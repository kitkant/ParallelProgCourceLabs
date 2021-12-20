#include <iostream>
#include <stdlib.h>
#include <cmath>
#include "kmeans.h"

#define MAX_BLOCK_SZ 128

template <int blockSize>
__device__ void warpReduce(volatile int* sdata,unsigned int tid) {
    if (blockSize >=  64) sdata[tid] += sdata[tid + 32];
    if (blockSize >=  32) sdata[tid] += sdata[tid + 16];
    if (blockSize >=  16) sdata[tid] += sdata[tid + 8];
    if (blockSize >=  8) sdata[tid] += sdata[tid + 4];
    if (blockSize >=  4) sdata[tid] += sdata[tid + 2];
    if (blockSize >=  2) sdata[tid] += sdata[tid + 1];
}

template <int blockSize>
__global__ void sum(int* g_odata, int* g_idata, int n) {
    extern __shared__ int sdata[];
    unsigned int tid = threadIdx.x;
    int i = blockIdx.x*(blockSize*2) + tid;
    int gridSize = blockSize*2*gridDim.x;
    sdata[tid] = 0;
    while (i < n){
        sdata[tid] += g_idata[i];
        g_idata[i] = 0;
        if (i + blockSize < n) {
            sdata[tid] += g_idata[i+blockSize];  
            g_idata[i+blockSize] = 0;
        }
        i += gridSize;  
    }
    __syncthreads();
    if (blockSize >= 512) { if (tid < 256) { sdata[tid] += sdata[tid + 256]; } __syncthreads(); }
    if (blockSize >= 256) { if (tid < 128) { sdata[tid] += sdata[tid + 128]; } __syncthreads(); }
    if (blockSize >= 128) { if (tid <  64) { sdata[tid] += sdata[tid +  64]; } __syncthreads(); }
    if (tid < 32) warpReduce<blockSize>(sdata, tid);
    if (tid == 0){
        g_odata[blockIdx.x] = sdata[0];
    } 
}

__device__ inline static float euclid_distance(int    numCoords,
                    int    numObjs,
                    int    numClusters,
                    int    tid,
                    int    clusterId,
                    float *objects,
                    float *clusters
                )
{
    float ans=0.0;
    for (int i = 0; i < numCoords; i++) {
        ans += (objects[3*tid+i] - clusters[i + clusterId*3]) *
               (objects[3*tid+i] - clusters[i + clusterId*3]);
    }

    return(ans);
}

__global__ static void find_nearest_cluster(int numCoords,
                          int numObjs,
                          int numClusters,
                          float *objects,    
                          float *deviceClusters,
                          int *membership,
                          int *changedmembership
)
{
    extern __shared__ float sharedMem[];
    float *sh_Clusters = sharedMem;
    float *sh_Objects = (float*)&sh_Clusters[numClusters * 3];

    for(int i = 0; i < numCoords * numClusters; i++) {
        sh_Clusters[i] = deviceClusters[i];
    }
    __syncthreads();

    unsigned int tid = threadIdx.x;
    int objectId = blockDim.x * blockIdx.x + threadIdx.x;

    while (objectId < numObjs) {
        int   index, i;
        float dist, min_dist;
        
        for(int i = 0; i < numCoords; i++) { 
            sh_Objects[3*tid+i] = objects[3*objectId+i];
        }

        index = 0;
        min_dist = euclid_distance(numCoords, numObjs, numClusters, tid,
             0, sh_Objects, sh_Clusters);

        for (i=1; i<numClusters; i++) {
            dist = euclid_distance(numCoords, numObjs, numClusters, tid,
                i, sh_Objects, sh_Clusters);
            if (dist < min_dist) {
                min_dist = dist;
                index    = i;
            }
        }
        if (membership[objectId] != index)
        {
            changedmembership[objectId] = 1;
            membership[objectId] = index;
            
        }
        objectId += blockDim.x * gridDim.x;
    }
}

float** cuda_kmeans(float **objects,      /* in: [numObjs][numCoords] */
    int     numCoords,    
    int     numObjs,     
    int     numClusters, 
    int    *membership  
)
{
#pragma region declaration

    int      i, j, index, loop=0;
    int total_sum = 0;

    float error = 0.001;
    float delta;              /* % of objects change their clusters */
    int GRID_SZ = (numObjs+MAX_BLOCK_SZ-1)/ MAX_BLOCK_SZ;
    int *newClusterSize; /* objects assigned in each new cluster */
    float  **loopClusters;   /* [numClusters][numCoords] */
    float  **newClusters;  /* [numClusters][numCoords] */  

    /*DEVICE*/
    int* d_block_sums;
    int* d_total_sum;

    int *d_Membership;
    int *d_Changedmembership;
    float *d_Objects;
    float *d_Clusters;

#pragma endregion

#pragma region init

    gpuErrchk(cudaSetDevice(0));
    
    /* initialize membership[] */
    for (i=0; i<numObjs; i++) membership[i] = -1;

    /* pick first numClusters elements of objects[] as initial cluster centers*/
    malloc2D(loopClusters, numClusters ,numCoords , float);
    for (i = 0; i < numClusters; i++) {
        for (j = 0; j < numCoords; j++) {
            loopClusters[i][j] = objects[i][j];
        }
    }

    newClusterSize = (int*) malloc(numClusters* sizeof(int));
    assert(newClusterSize != NULL);

    malloc2D(newClusters, numClusters,numCoords, float);
    memset(newClusters[0], 0, (numCoords * numClusters) * sizeof(float));
    memset(newClusterSize, 0, numClusters * sizeof(int));

    gpuErrchk(cudaMalloc(&d_Objects, numObjs*numCoords*sizeof(float)));
    gpuErrchk(cudaMalloc(&d_Clusters, numClusters*numCoords*sizeof(float)));
    gpuErrchk(cudaMalloc(&d_Membership, numObjs*sizeof(int)));
    gpuErrchk(cudaMalloc(&d_Changedmembership, numObjs*sizeof(int)));
    gpuErrchk(cudaMalloc(&d_block_sums, sizeof(int) * GRID_SZ));
    gpuErrchk(cudaMalloc(&d_total_sum, sizeof(int)));

    gpuErrchk(cudaMemset(d_total_sum, 0, sizeof(int)));
    gpuErrchk(cudaMemset(d_block_sums, 0, sizeof(int) * GRID_SZ));
    gpuErrchk(cudaMemset(d_Changedmembership,0, numObjs*sizeof(int)));
    
    gpuErrchk(cudaMemcpy(d_Objects, objects[0], numObjs*numCoords*sizeof(float), cudaMemcpyHostToDevice));
    gpuErrchk(cudaMemcpy(d_Membership, membership, numObjs*sizeof(int), cudaMemcpyHostToDevice));

#pragma endregion

do {

    int tot_cor = 0;
    gpuErrchk(cudaMemcpy(d_Clusters, loopClusters[0], numClusters*numCoords*sizeof(float), cudaMemcpyHostToDevice));

    find_nearest_cluster
        <<<GRID_SZ, MAX_BLOCK_SZ,sizeof(float) * numCoords * numClusters + sizeof(float) * numCoords * MAX_BLOCK_SZ>>> //assume here that 1 object per 1 thread in a block
        (numCoords, numObjs, numClusters, d_Objects,d_Clusters, d_Membership, d_Changedmembership);

    sum<MAX_BLOCK_SZ><<<GRID_SZ, MAX_BLOCK_SZ, sizeof(int) * MAX_BLOCK_SZ>>>(d_block_sums, d_Changedmembership, numObjs);

    sum<MAX_BLOCK_SZ><<<1, MAX_BLOCK_SZ, sizeof(int) * MAX_BLOCK_SZ>>>(d_total_sum, d_block_sums, GRID_SZ);

    gpuErrchk(cudaMemcpy(&total_sum, d_total_sum, sizeof(int), cudaMemcpyDeviceToHost));

    delta = (float)total_sum/(float)numObjs;

    gpuErrchk(cudaMemcpy(membership, d_Membership, numObjs*sizeof(int), cudaMemcpyDeviceToHost));
   
    for (i=0; i<numObjs; i++) {
        /* find the array index of nestest cluster center */
        index = membership[i];
      
        /* update new cluster centers : sum of objects located within */
        newClusterSize[index] += 1;
        for (j=0; j<numCoords; j++)
            {
                newClusters[index][j] += objects[i][j];
            }
    }
    
    /*set new cluster centers*/
    for (i=0; i<numClusters; i++) {
        for (j=0; j<numCoords; j++) {
            if (newClusterSize[i] > 0)
            {             
                loopClusters[i][j] = newClusters[i][j] / newClusterSize[i];
            }
            newClusters[i][j] = 0.0;   /* set back to 0 */
        }
        tot_cor += newClusterSize[i];
        newClusterSize[i] = 0;   /* set back to 0 */
    }

    if (tot_cor != numObjs) {
        printf("Sum error \n");
        exit(-1);
    }

    } while (delta > error && loop++ < 500);

#pragma region free
    gpuErrchk(cudaFree(d_Membership));
    gpuErrchk(cudaFree(d_Changedmembership));
    gpuErrchk(cudaFree(d_Objects));
    gpuErrchk(cudaFree(d_Clusters));
    gpuErrchk(cudaFree(d_total_sum));
	gpuErrchk(cudaFree(d_block_sums));

    free(newClusters[0]);
    free(newClusters);
    free(newClusterSize);
#pragma endregion

    return loopClusters;
}
#include <iostream>
#include <sys/time.h>
#include <mpi.h>
#include <cstdlib>

using namespace std;

void swap(int *data, int i, int j) {
  int temp = data[i];
  data[i] = data[j];
  data[j] = temp;
}

int partition(int *data, int start, int end) {
  if (start >= end) return 0;

  int pivotValue = data[start];
  int low = start;
  int high = end - 1;
  while (low < high) {
    while (data[low] <= pivotValue && low < end) low++;
    while (data[high] > pivotValue && high > start) high--;
    if (low < high) swap(data, low, high);
  }
  swap(data, start, high);

  return high;
}

void quicksort(int *data, int start, int end) {
  if  (end-start+1 < 2) return;

  int pivot = partition(data, start, end);

  quicksort(data, start, pivot);
  quicksort(data, pivot+1, end);
}

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);
  int rank, size;

  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Comm_size (MPI_COMM_WORLD, &size);

  if (argc < 2) {
    if (rank == 0)
      cout << "Usage: mpiqsort num_of_numbers" << endl;
    exit(0);
  }

  int length = atoi(argv[1]);
  srand(time(0));
  int *data = new int[length];	
  int i;
  for (i=0; i<length/size; i++)
    data[i] = rand();
  
  MPI_Status status;
  if (rank == 0) {
    for (i=1; i<size; i++)
      MPI_Recv(data+i*length/size, length/size, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  }
  else {
    MPI_Send(data, length/size, MPI_INT, 0, 0, MPI_COMM_WORLD);
  }
  timeval start, end;
  gettimeofday(&start, 0);
  int s;
  int localDataSize =  length;
  int pivot;
  for (s=size; s > 1; s /= 2) {
    if (rank % s == 0) {
      pivot = partition(data, 0, localDataSize);
      MPI_Send(data+pivot, localDataSize - pivot, MPI_INT, rank + s/2, 0, MPI_COMM_WORLD);
      localDataSize = pivot;
    }
    else if (rank % s == s/2) {
      MPI_Recv(data, length, MPI_INT, rank - s/2, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      MPI_Get_count(&status, MPI_INT, &localDataSize);
    }
  }
  quicksort(data, 0, localDataSize);
  gettimeofday(&end, 0);
  if (rank == 0)
    cout << "Elapsed time for sorting = " << (end.tv_sec - start.tv_sec + .000001*(end.tv_usec - start
 .tv_usec)) << " seconds" << endl;

  MPI_Finalize();
}
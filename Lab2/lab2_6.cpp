#include <mpi.h>
#include <time.h>
#include <cstdlib>
#include <iostream>
using namespace std;
int main(int argc, char* argv[])
{
int id, np;
double x, y, eTime,sTime,pTime;
long int lhit, hit=0, i,lN, N=10000000000;
MPI_Init(&argc, &argv);
MPI_Comm_rank(MPI_COMM_WORLD, &id);
MPI_Comm_size(MPI_COMM_WORLD, &np);
MPI_Barrier(MPI_COMM_WORLD);
sTime = MPI_Wtime();
lhit = 0;
srand((unsigned)(time(0)));
lN = N/np;
for(i = 1; i<=lN;i++)
{
x = ((double)rand())/((double)RAND_MAX);
y = ((double)rand())/((double)RAND_MAX);
if (((x*x) + (y*y)) <= 1) lhit++;
}
MPI_Reduce(&lhit,&hit,1,MPI_DOUBLE, MPI_SUM,0,MPI_COMM_WORLD);
double est;
est = ((double)hit*4)/(double)N;
MPI_Barrier(MPI_COMM_WORLD);
eTime = MPI_Wtime();
pTime =eTime - sTime;
if (id == 0)
{
cout << "Number of drops: " << double_N << endl;
cout << "Pi score: " << est << endl;
cout << "Duration: " << pTime << endl;
}
MPI_Finalize();
return 0;
}

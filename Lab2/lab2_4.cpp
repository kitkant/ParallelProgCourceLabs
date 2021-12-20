#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

int numTasks, rank;

int min(int a, int b) {
  if (a <= b) return a;
  return b;
}

int max(int a, int b) {
  if (a >= b) return a;
  return b;
}


int *createNumbers(int howMany) {
  int * numbers = (int *) malloc(sizeof(int) * howMany);

  if (numbers == NULL) {
    printf("Error: malloc failed.\n");
    return NULL;
  }

  srand(rank+3);

  for(int i=0; i < howMany; i++) 
    numbers[i] = rand();

  return numbers;
}

void printNumbers(int * numbers, int howMany) {
  for(int i=0; i < howMany; i++)
    printf("%d\n", numbers[i]);
}

void merge(int *A, int iLeft, int iRight, int iEnd, int *B)
{
  int i0 = iLeft;
  int i1 = iRight;
  int j;
  for (j = iLeft; j < iEnd; j++) {
    if (i0 < iRight && (i1 >= iEnd || A[i0] <= A[i1])) {
      B[j] = A[i0];
      i0 = i0 + 1;
    }
    else {
      B[j] = A[i1];
      i1 = i1 + 1;
    }
  }
}
void mergeSort(int *list, int n, int startWidth)
{
  int *A = list;
  int *B = (int *)malloc(sizeof(int)*n);
  if (B == NULL) {
    printf("Error: malloc failed.\n");
    return;
  }
  int width;
  for (width = startWidth; width < n; width = 2 * width) {
    int i;
    for (i = 0; i < n; i = i + 2 * width)  {
		merge(A, i, min(i+width, n), min(i+2*width, n), B);
    }
    int *temp = B; B = A; A = temp;
  }
  if (A == list)
    free(B);
  else {
    for(int i=0; i < n; i++)
      list[i] = A[i];
    free(A);
  }
}
int isSorted(int *numbers, int howMany) {
  for(int i=1; i<howMany; i++) {
    if (numbers[i] < numbers[i-1]) return 0;
  }
  return 1;
}
int * mergeAll(int *list, int howMany) {
  int * A = NULL;
  int n = howMany * numTasks;
  if (rank == 0)
    A = (int *) malloc(n * sizeof(int));
  MPI_Gather(list, howMany, MPI_INT, 
	     A, howMany, MPI_INT, 
	     0, MPI_COMM_WORLD);
  if (rank != 0) return NULL;
  mergeSort(A, n, howMany);
  return A;
}
int main(int argc, char *argv[]) {
  double timeStart, timeEnd;
  int howMany;
  if (argc < 2) {
    printf("  Usage: ./a.out howMany.\n");
  }
  howMany = atoi(argv[1]);
  long int returnVal;
  int len;
  char hostname[MPI_MAX_PROCESSOR_NAME];
  returnVal = MPI_Init(&argc, &argv);
  if (returnVal != MPI_SUCCESS) {
    printf("Error starting MPI program.  Terminating.\n");
    MPI_Abort(MPI_COMM_WORLD, returnVal);
  }
  MPI_Comm_size(MPI_COMM_WORLD, &numTasks);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Get_processor_name(hostname, &len);
  timeStart = MPI_Wtime();
 int * numbers = createNumbers(howMany);
  mergeSort(numbers, howMany, 1);
  int *allNumbers = mergeAll(numbers, howMany);
  if (rank == 0) {
    if (isSorted(allNumbers, howMany*numTasks))
      printf("Numbers are sorted now!\n");
    else
      printf("Problem - numbers are not sorted!\n");
    free(allNumbers);
    timeEnd = MPI_Wtime();
    printf("Total # seconds elapsed is %f.\n", timeEnd-timeStart);
  }
  
  free(numbers);


  MPI_Finalize();
  return 0;
}
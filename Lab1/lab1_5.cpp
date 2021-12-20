#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct TASK
{
	int low;
	int high;
	int busy;
	int* a;
};

void merge(int* a, int low, int mid, int high)
{

	int n1 = mid - low + 1;
	int n2 = high - mid;

	int* left = (int*)malloc(n1 * sizeof(int));
	int* right = (int*)malloc(n2 * sizeof(int));

	int i;
	int j;
	for (i = 0; i < n1; i++)
		left[i] = a[i + low];
	for (i = 0; i < n2; i++)
		right[i] = a[i + mid + 1];

	int k = low;

	i = j = 0;
	while (i < n1 && j < n2)
	{
		if (left[i] <= right[j])
			a[k++] = left[i++];
		else
			a[k++] = right[j++];
	}
	while (i < n1)
		a[k++] = left[i++];
	while (j < n2)
		a[k++] = right[j++];

	free(left);
	free(right);
}

void merge_sort(int* a, int low, int high)
{
	int mid = low + (high - low) / 2;

	if (low < high)
	{
		merge_sort(a, low, mid);
		merge_sort(a, mid + 1, high);
		merge(a, low, mid, high);
	}
}

void* merge_sort_thread(void* arg)
{
	TASK* task = (TASK*)arg;
	int low;
	int high;
	low = task->low;
	high = task->high;
	int mid = low + (high - low) / 2;

	if (low < high)
	{
		merge_sort(task->a, low, mid);
		merge_sort(task->a, mid + 1, high);
		merge(task->a, low, mid, high);
	}
	task->busy = 0;
	return 0;
}
int main(int argc, char** argv)
{
	char* sz;

	int MAX_ARRAY_ELEMENTS = 2000;
	int MAX_THREADS = 1;
	for (--argc, ++argv; argc > 0; --argc, ++argv)
	{
		sz = *argv;
		if (*sz != '-')
			break;

		switch (sz[1])
		{
		case 'A': 
			MAX_ARRAY_ELEMENTS = atoi(sz + 2);
			break;

		case 'T': 
			MAX_THREADS = atoi(sz + 2);
			break;
		}
	}

	printf("\n\nArray[%d]\nThreads[%d]", MAX_ARRAY_ELEMENTS, MAX_THREADS);
	int* array = (int*)malloc(sizeof(int) * MAX_ARRAY_ELEMENTS);
	srand(clock());
	for (int i = 0; i < MAX_ARRAY_ELEMENTS; i++)
		array[i] = rand();

	printf("\n\nArray Randomized");

	pthread_t * threads = (pthread_t*)malloc(sizeof(pthread_t) * MAX_THREADS);
	TASK * tasklist = (TASK*)malloc(sizeof(TASK) * MAX_THREADS);

	int len = MAX_ARRAY_ELEMENTS / MAX_THREADS;

	TASK * task;
	int low = 0;

	clock_t time = clock();

	for (int i = 0; i < MAX_THREADS; i++, low += len)
	{
		task = &tasklist[i];
		task->low = low;
		task->high = low + len - 1;
		if (i == (MAX_THREADS - 1))
			task->high = MAX_ARRAY_ELEMENTS - 1;
	}
	for (int i = 0; i < MAX_THREADS; i++)
	{
		task = &tasklist[i];
		task->a = array;
		task->busy = 1;
		pthread_create(&threads[i], 0, merge_sort_thread, task);
	}
	for (int i = 0; i < MAX_THREADS; i++)
		while (tasklist[i].busy)
			sleep(50);


	TASK * taskm = &tasklist[0];
	for (int i = 1; i < MAX_THREADS; i++)
	{
		TASK* task = &tasklist[i];
		merge(taskm->a, taskm->low, task->low - 1, task->high);
	}

	printf("\n\nSorted in %f Seconds", (clock() - time) / 1000.0L);

	int last = 0;
	for (int i = 0; i < MAX_ARRAY_ELEMENTS; i++)
	{
		if (array[i] < last)
		{
			printf("\n\nArray Not Sorted");
			return 0;
		}
		last = array[i];
	}

	printf("\n\nArray Sorted");
	if (MAX_ARRAY_ELEMENTS < 50)
		for (int i = 0; i < MAX_ARRAY_ELEMENTS; i++)
			printf(" %d", array[i]);
	printf("\n");

	free(tasklist);
	free(threads);

	return 0;
}

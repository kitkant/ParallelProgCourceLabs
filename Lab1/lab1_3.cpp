#include <iostream>
#include <time.h>
#include <thread>
using namespace std;

class QS
{
public:
	int* c_array;
	int c_size;
	int c_thread_id;
	QS** c_threads;

	QS(int* array, int size, QS** threads, int thread_id)
	{
		c_array = array;
		c_size = size;
		c_threads = threads;
		c_thread_id = thread_id;
		threads[thread_id] = this;
	}

	~QS()
	{
		c_threads[c_thread_id] = nullptr;
	}
};

void qsort(int* array, const unsigned int size)
{
	if (size <= 30)
	{
		for (unsigned int i = 1; i < size; i++) {
			int temp = array[i];
			unsigned int j = i;
			while (j > 0 && temp < array[j - 1]) {
				array[j] = array[j - 1];
				j--;
			}
			array[j] = temp;
		}
	}
	else
	{
		int pivot = array[size / 2];
		int* left = array;
		int* right = array + size - 1;
		while (true) {
			while ((left <= right) && (*left < pivot)) left++;
			while ((left <= right) && (*right > pivot)) right--;
			if (left > right) break;
			int temp = *left;
			*left++ = *right;
			*right-- = temp;
		}
		qsort(array, right - array + 1);
		qsort(left, array + size - left);
	}
}

void* qsort_thread(void* obj)
{
	qsort(((QS*)obj)->c_array, ((QS*)obj)->c_size);
	delete ((QS*)obj);
	return nullptr;
}


struct RAND
{
	int* s_array;
	int s_size;

	RAND(int* array, int size)
	{
		s_array = array;
		s_size = size;
	}
};

int RANDOM_THREAD_COUNT = 0;

void* randomize_thread(void* obj)
{
	RAND* r = (RAND*)obj;
	for (int i = 0; i < r->s_size; i++)
		r->s_array[i] = rand();
	delete (RAND*)r;
	RANDOM_THREAD_COUNT--;
	return nullptr;
}

int main(int argc, char** argv)
{
	char* sz;

	int MAX_ARRAY_ELEMENTS = 2000;
	int MAX_THREADS = thread::hardware_concurrency();

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

	if (MAX_THREADS < 1)
		MAX_THREADS = 1;

	cout << "\n\nArray[" << MAX_ARRAY_ELEMENTS << "]\nThreads[" << MAX_THREADS << "]";

	int* g_array = new int[MAX_ARRAY_ELEMENTS];

	int len = MAX_ARRAY_ELEMENTS / MAX_THREADS;

	srand(clock());

	for (int i = 0, ai = 0; i < MAX_THREADS; i++, ai += len)
	{
		RANDOM_THREAD_COUNT++;
		pthread_t t;
		int size = len + (i == (MAX_THREADS - 1) ? (MAX_ARRAY_ELEMENTS % MAX_THREADS) : 0);
		pthread_create(&t, 0, randomize_thread, new RAND(&g_array[ai], size));
	}
	while (RANDOM_THREAD_COUNT)
		sleep(10);
	cout << "\n\nArray Randomized";
	QS * *threads = new QS * [MAX_THREADS];

	clock_t time = clock();
	for (int i = 0, ai = 0; i < MAX_THREADS; i++, ai += len)
	{
		threads[i] = nullptr;
		pthread_t t;
		int size = len + (i == (MAX_THREADS - 1) ? (MAX_ARRAY_ELEMENTS % MAX_THREADS) : 0);
		pthread_create(&t, 0, qsort_thread, new QS(&g_array[ai], size, threads, i));
	}
	for (int i = 0; i < MAX_THREADS;)
	{
		sleep(10);
		if (threads[i])
			continue;
		i++;
	}
	if (MAX_THREADS > 1)
		qsort(g_array, MAX_ARRAY_ELEMENTS);

	cout << "\n\nSorted in " << ((clock() - time) / 1000.0L) << " Seconds";

	int last = 0;
	for (int i = 0; i < MAX_ARRAY_ELEMENTS; i++)
	{
		if (g_array[i] < last)
		{
			cout << "\n\nArray Not Sorted";
			return 0;
		}
		last = g_array[i];
	}
	cout << "\n\nArray Sorted";
	if (MAX_ARRAY_ELEMENTS < 50)
		for (int i = 0; i < MAX_ARRAY_ELEMENTS; i++)
			cout << " " << g_array[i];
	cout << "\n";
	delete[]g_array;
	delete[]threads;
}

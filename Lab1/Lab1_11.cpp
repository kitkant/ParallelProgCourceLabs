#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<errno.h>
#include<semaphore.h>

#define N 5

sem_t mutex,cus,bar;
int count=0;

void* barber(void *arg);
void* customer(void *arg);

int main(int argc,char *argv[])
{
	pthread_t ida,idb;
	int st=0;

	sem_init(&mutex,0,1);
	sem_init(&cus,0,0);
	sem_init(&bar,0,1);

	st=pthread_create(&ida,NULL,barber,NULL);
	st=pthread_create(&idb,NULL,customer,NULL);


	pthread_join(idb,NULL);
	pthread_join(ida,NULL);

	exit(0);
}

void* barber(void *arg)
{
	while(1)
	{
		sem_wait(&cus);           
		sem_wait(&mutex);
		count--;
		printf("Barber is working \tcount is:%d.\n",count);
		sem_post(&mutex);
		sem_post(&bar);
		sleep(3);       
	}
}

void* customer(void *arg)
{
	while(1)
	{
		sem_wait(&mutex);
		if(count<N)
		{
			count++;
			printf("Customer Arrived \tcount is:%d\n",count);
			sem_post(&mutex);
			sem_post(&cus);
			sem_wait(&bar);
		}
		else
			sem_post(&mutex);
		sleep(1);
	}
}
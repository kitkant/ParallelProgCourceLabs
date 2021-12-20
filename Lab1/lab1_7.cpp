#include <iostream>
#include <cstdlib>
#include "omp.h"
#include <vector>
#include <chrono>
using namespace std;
using namespace std::chrono;


#define TMAX   16  
#define ITERATIONS 4 

static long num_steps = 100000;
double step = 1.0/(double)num_steps;

void operation(int num_t)
{
	double x, pi, sum = 0.0, psum=0.0;
	int i;
	omp_set_num_threads(num_t);
	#pragma omp parallel private(psum)
	{
		psum = 0.0;
		#pragma omp for
		for (i=0; i<num_steps; i++) 
		{
			x = (i+0.5)*step;
			
			psum += 4.0/(1.0+x*x);
		}
		#pragma omp atomic
			sum += psum;
	}
	pi = step * sum;
	cout<<pi<<endl;
}

int calcAvgTime(int n)
{
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	operation(n);

	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>( t2 - t1 ).count();

	return (int)duration;
}

int main () {

   int avg_times[TMAX+1];


   for (int i = 1; i <= 1; ++i)
   {
      avg_times[i] = calcAvgTime(i);
   }
   for (int i = 1; i <= TMAX; ++i)
   {
      avg_times[i] = 0;
   }

   for (int x = 0; x < ITERATIONS; ++x)
   {
      for (int i = 1; i <= TMAX; ++i)
      {
         avg_times[i] += calcAvgTime(i);
      }
   }

   for (int i = 1; i <= TMAX; ++i)
   {
      avg_times[i] /= ITERATIONS;
   }
   cout<<"Num_t\tExec Time (micro-s)"<<endl;
   for (int i = 1; i <= TMAX; ++i)
   {
      cout<<i<<"\t"<<avg_times[i]<<endl;
   }

   return 0;
}
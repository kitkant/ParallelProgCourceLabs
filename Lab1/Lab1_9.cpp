#include <iostream>
#include <mutex>
#include <thread>
#include <ctime>
#include <chrono>
#include <cstdlib>


const int NrOfPhil = 5;
std::thread philosphers[NrOfPhil];
std::mutex cout_mutex;
int counter = 30;

bool chopsticks[NrOfPhil] = {};




void log(std::string str){
    cout_mutex.lock();
    std::cout << str << std::endl;
    cout_mutex.unlock();
}


void think(int id){
    std::this_thread::sleep_for(std::chrono::milliseconds(2500+(std::rand() % 1000)));
    log("Philosopher " + std::to_string(id) + " is thinking.");
}

void eat(int id){
    std::this_thread::sleep_for(std::chrono::milliseconds(2500+(std::rand() % 1000)));
    log("Philosopher " + std::to_string(id) + " is eating.");
}


void dine(int id){
    while (counter-- > 0)
    {

        if (chopsticks[id] == true)
        {

            chopsticks[id] = false;

            eat(id);

            chopsticks[id] = true;

            think(id);
        }
    }
}

void start(){

    for (int i = 0; i < NrOfPhil; ++i) philosphers[i] = std::thread(dine, i);

    for (int i = 0; i < NrOfPhil; ++i) philosphers[i].join();

}

int main()
{
    for (int i=0; i<NrOfPhil; i++)
    {
    chopsticks[i] = true;
    }

    srand(time(NULL));
    start();

}
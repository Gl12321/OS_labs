#include <iostream>
#include <thread>
#include <mutex>

int counter = 0;

std::mutex m;

void inc() {
    for(int i = 0; i < 100000; i++) {
        std::lock_guard<std::mutex> lock(m);
        counter++;
    }
}

int main() {
    std::thread t1(inc);
    std::thread t2(inc);
    std::thread t3(inc);
    std::thread t4(inc);

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    std::cout << counter << std::endl;

    return 0;
}
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <cassert>
#include "buffered_channel.h"


int main() {
    {
        BufferedChannel<int> ch(3);
        
        ch.Send(10);
        ch.Send(20);
        ch.Send(30);
        ch.Close(); 

        std::pair<int, bool> res;
        
        res = ch.Recv();
        assert(res.first == 10 && res.second == true);

        res = ch.Recv();
        assert(res.first == 20 && res.second == true);

        res = ch.Recv();
        assert(res.first == 30 && res.second == true);

        res = ch.Recv();
        assert(res.second == false);
        std::cout << "OK\n";

        bool caught = false; 
        try {
            ch.Send(40);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        assert(caught);
        std::cout << "OK\n";
    }

    {        
        const int buf_size = 5;
        const int producers_cnt = 4;
        const int consumers_cnt = 4;
        const int items_per_thread = 10000;

        BufferedChannel<int> ch(buf_size);
        std::atomic<long long> total_sum(0);
        
        std::vector<std::thread> producers;
        std::vector<std::thread> consumers;

        for (int i = 0; i < consumers_cnt; ++i) {
            consumers.emplace_back([&]() {
                while (true) {
                    auto res = ch.Recv();
                    if (!res.second) break; 
                    total_sum += res.first;
                }
            });
        }

        for (int i = 0; i < producers_cnt; ++i) {
            producers.emplace_back([&]() {
                for (int j = 1; j <= items_per_thread; ++j) {
                    ch.Send(j);
                }
            });
        }

        for (auto& t : producers) {
            t.join();
        }

        ch.Close();

        for (auto& t : consumers) {
            t.join();
        }

        long long sum_per_thread = (long long)items_per_thread * (items_per_thread + 1) / 2;
        long long expected_total = sum_per_thread * producers_cnt;

        if (total_sum == expected_total) {
            std::cout << "OK/n " << total_sum << ")\n";
        } else {
            std::cout << "jopa/n";
            return 1;
        }
    }

    return 0;
}
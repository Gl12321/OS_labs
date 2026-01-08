#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <sstream>

const int N = 19;

void worker_M() { 
    long long x;
    while (std::cin >> x) std::cout << (x * 7) << " ";
}

void worker_A() {
    long long x;
    while (std::cin >> x) std::cout << (x + N) << " ";
}

void worker_P() { 
    long long x;
    while (std::cin >> x) std::cout << (x * x * x) << " ";
}

void worker_S() { 
    long long sum = 0, x;
    while (std::cin >> x) sum += x;
    std::cout << sum << std::endl;
}

int main() {
    int start_pipe[2];

    std::string input_data = "1 2 3";
    write(start_pipe[1], input_data.c_str(), input_data.size());
    close(start_pipe[1]); 

    int prev_read_fd = start_pipe[0];

    typedef void (*WorkerFunc)();
    struct Task { WorkerFunc func; bool is_last; };
    
    std::vector<Task> chain = {
        {worker_M, false},
        {worker_A, false},
        {worker_P, false},
        {worker_S, true} 
    };

    int final_result_fd = -1;

    for (const auto& task : chain) {
        int next_pipe[2];
        if (pipe(next_pipe) == -1) return 1;

        if (fork() == 0) {
            dup2(prev_read_fd, STDIN_FILENO);
            close(prev_read_fd);

            dup2(next_pipe[1], STDOUT_FILENO);
            close(next_pipe[1]);
            close(next_pipe[0]);

            task.func();
            
            exit(0); 
        }

        close(prev_read_fd);
        close(next_pipe[1]);

        if (task.is_last) {
            final_result_fd = next_pipe[0];
        } else {
            prev_read_fd = next_pipe[0];
        }
    }

    char buffer[128];
    ssize_t bytes = read(final_result_fd, buffer, sizeof(buffer) - 1);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        std::cout << "результат: " << buffer; 
    }
    close(final_result_fd);

    for (size_t i = 0; i < chain.size(); ++i) wait(nullptr);

    return 0;
}
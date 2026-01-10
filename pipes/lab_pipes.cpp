#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <string>

using namespace std;

void run_worker(char type) {
    if (type == 'M') {
        long long x;
        while (cin >> x) { cout << (x * 7) << " "; cout.flush(); }
    } else if (type == 'A') {
        long long x;
        while (cin >> x) { cout << (x + 19) << " "; cout.flush(); }
    } else if (type == 'P') {
        long long x;
        while (cin >> x) { cout << (x * x * x) << " "; cout.flush(); }
    } else if (type == 'S') {
        long long sum = 0, x;
        while (cin >> x) sum += x;
        cout << sum << endl;
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        run_worker(argv[1][0]);
        return 0;
    }

    int start_pipe[2];
    if (pipe(start_pipe) == -1) return 1;

    string input_data = "1 2 3 ";
    write(start_pipe[1], input_data.c_str(), input_data.size());
    close(start_pipe[1]);

    int prev_read_fd = start_pipe[0];
    vector<string> workers = {"M", "A", "P", "S"};
    int final_result_fd = -1;

    char path[1024];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    path[len] = '\0';

    for (size_t i = 0; i < workers.size(); ++i) {
        int next_pipe[2];
        pipe(next_pipe);

        if (fork() == 0) {
            dup2(prev_read_fd, STDIN_FILENO);
            dup2(next_pipe[1], STDOUT_FILENO);
            close(prev_read_fd);
            close(next_pipe[0]);
            close(next_pipe[1]);

            execl(path, path, workers[i].c_str(), nullptr);
            exit(1);
        }

        close(prev_read_fd);
        close(next_pipe[1]);
        
        if (i == workers.size() - 1) final_result_fd = next_pipe[0];
        else prev_read_fd = next_pipe[0];
    }

    char buffer[128];
    ssize_t bytes = read(final_result_fd, buffer, sizeof(buffer) - 1);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        cout << "результат: " << buffer;
    }
    close(final_result_fd);

    for (size_t i = 0; i < workers.size(); ++i) wait(nullptr);

    return 0;
}
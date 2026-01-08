#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <thread>
#include <chrono>

using namespace std;

pid_t spawn() {
    pid_t pid = fork();
    if (pid == 0) {
        execlp("sleep", "sleep", "10", NULL);
        _exit(1);
    }
    return pid;
}

void verify(pid_t pid) {
    this_thread::sleep_for(chrono::milliseconds(100));
    if (kill(pid, 0) == -1) {
        cout << " [OK]" << endl;
    } else {
        cout << " [FAIL]" << endl;
        kill(pid, 9);
    }
    waitpid(pid, nullptr, WNOHANG);
}

int main() {
    pid_t p;

    p = spawn();
    cout << "ID:";
    system(("./killer --id " + to_string(p)).c_str());
    verify(p);

    p = spawn();
    cout << "NAME:";
    system("./killer --name sleep");
    verify(p);

    p = spawn();
    cout << "ENV:";
    setenv("PROC_TO_KILL", "sleep", 1);
    system("./killer");
    verify(p);
    unsetenv("PROC_TO_KILL");

    return 0;
}
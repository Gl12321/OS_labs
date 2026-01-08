#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <cstring>

using namespace std;

void killPid(pid_t pid) {
    if (kill(pid, SIGKILL) == 0) {
        cout << "Killed PID: " << pid << endl;
    } else {
        perror("Failed to kill");
    }
}

void killByName(const std::string& targetName) {
    DIR* dir = opendir("/proc");

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (!isdigit(*entry->d_name)) continue;

        pid_t pid = stoi(entry->d_name);
        
        string commPath = std::string("/proc/") + entry->d_name + "/comm";
        ifstream commFile(commPath);
        string procName;
        
        if (commFile >> procName) {
            if (procName == targetName) {
                if (pid == getpid()) continue;
                
                cout << "Found" << targetName << "' at PID " << pid << std::endl;
                killPid(pid);
            }
        }
    }
    closedir(dir);
}

void checkEnv() {
    const char* envVal = getenv("PROC_TO_KILL");
    if (envVal) {
        std::string raw(envVal);
        std::cout << "[ENV] PROC_TO_KILL found: " << raw << std::endl;
        
        std::stringstream ss(raw);
        std::string name;
        while (std::getline(ss, name, ',')) {
            name.erase(0, name.find_first_not_of(" \t\n\r"));
            name.erase(name.find_last_not_of(" \t\n\r") + 1);
            
            if (!name.empty()) killByName(name);
        }
    }
}

int main(int argc, char* argv[]) {
    checkEnv();

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--id") {
            if (i + 1 < argc) {
                killPid(std::stoi(argv[++i]));
            }
        } else if (arg == "--name") {
            if (i + 1 < argc) {
                killByName(argv[++i]);
            }
        }
    }
    return 0;
}
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#include <thread>
#include <chrono>

// Запуск жертвы (фоновый процесс sleep)
pid_t spawnVictim() {
    pid_t pid = fork();
    if (pid == 0) {
        // Дочерний процесс: заменяем себя на sleep 100
        execlp("sleep", "sleep", "100", NULL);
        exit(1); // Если execlp не сработал
    }
    // Родитель возвращает PID ребенка
    return pid;
}

bool isAlive(pid_t pid) {
    return kill(pid, 0) == 0;
}

int main() {
    std::string killerBin = "./killer";

    std::cout << "=== Linux User Test ===\n";

    std::cout << "\n>>> Test 1: Kill by ID\n";
    pid_t v1 = spawnVictim();
    std::cout << "Spawned victim (sleep) PID: " << v1 << std::endl;
    
    std::string cmd1 = killerBin + " --id " + std::to_string(v1);
    system(cmd1.c_str());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (!isAlive(v1)) std::cout << "SUCCESS: Victim is dead.\n";
    else std::cout << "FAIL: Victim is still alive.\n";
    waitpid(v1, nullptr, WNOHANG); // Очистка зомби

    std::cout << "\n>>> Test 2: Kill by Name\n";
    pid_t v2 = spawnVictim();
    std::cout << "Spawned victim PID: " << v2 << std::endl;

    std::string cmd2 = killerBin + " --name sleep";
    system(cmd2.c_str());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (!isAlive(v2)) std::cout << "SUCCESS: Victim is dead.\n";
    else std::cout << "FAIL: Victim is still alive.\n";
    waitpid(v2, nullptr, WNOHANG);

    // --- ТЕСТ 3: Переменная окружения ---
    std::cout << "\n>>> Test 3: Kill by ENV\n";
    pid_t v3 = spawnVictim();
    std::cout << "Spawned victim PID: " << v3 << std::endl;

    // Установка переменной (1 - перезаписать если есть)
    setenv("PROC_TO_KILL", "vlc,sleep,firefox", 1);
    
    // Запуск киллера без аргументов
    system(killerBin.c_str());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (!isAlive(v3)) std::cout << "SUCCESS: Victim is dead.\n";
    else std::cout << "FAIL: Victim is still alive.\n";
    waitpid(v3, nullptr, WNOHANG);

    // Удаление переменной
    unsetenv("PROC_TO_KILL");
    std::cout << "ENV var removed.\n";

    return 0;
}
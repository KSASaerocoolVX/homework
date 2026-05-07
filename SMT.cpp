#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <algorithm>
#ifdef _WIN32
    #include <windows.h>
#endif

std::mutex cout_mtx;

class TaskQueue {
private:
    std::queue<int> tasks;
    std::mutex mtx;
    std::condition_variable cv;
    bool stop = false;

public:
    void push(const int task) {
        std::lock_guard<std::mutex> lock(mtx);
        tasks.push(task);
        cv.notify_one();
    }

    bool pop(int& task) {
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock, [this] { return !tasks.empty() || stop; });

        if (stop && tasks.empty()) {
            return false;
        }

        task = tasks.front();
        tasks.pop();
        return true;
    }

    void stopQueue() {
        std::lock_guard<std::mutex> lock(mtx);
        stop = true;
        cv.notify_all();
    }
};

void worker(unsigned int id, TaskQueue& q) {
    int task;
    while (q.pop(task)) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::lock_guard<std::mutex> lock(cout_mtx);
        std::cout << "[Worker-" << id << "] обработал задачу " << task << "\n";
    }

}

int main() {
    #ifdef _WIN32
        SetConsoleOutputCP(65001); // Установка кодовой страницы UTF-8 для корректного отображения символов в терминале
        SetConsoleCP(65001);
    #endif
    TaskQueue queue;
    const unsigned int NUM_WORKERS = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> threads;

    for (int i = 1; i <= 20; ++i) {
        queue.push(i);
    }

    for (unsigned int i = 1; i <= NUM_WORKERS; ++i) {
        threads.emplace_back(worker, i, std::ref(queue));
    }


    queue.stopQueue();

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    std::cout << "Все задачи выполнены успешно.\n";
    return 0;
}
#include "../include/searchDirectory.h"
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <tuple>

class ThreadPool {
public:
    ThreadPool(size_t numThreads) : stop(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        unique_lock<mutex> lock(queueMutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) {
                            return;
                        }
                        task = move(tasks.front());
                        tasks.pop_front();
                    }
                    task();
                }
            });
        }
    }

    template <class F, class... Args>
    void enqueue(F&& f, Args&&... args) {
        {
            unique_lock<mutex> lock(queueMutex);
            tasks.emplace_back([f = std::forward<F>(f), args = std::make_tuple(std::forward<Args>(args)...)] {
                std::apply(f, args);
            });
        }
        condition.notify_one();
    }

    void stopExecution() {
        stop = true;
        condition.notify_all();
    }

    ~ThreadPool() {
        {
            unique_lock<mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (thread& worker : workers) {
            worker.join();
        }
    }

private:
    vector<thread> workers;
    deque<std::function<void()>> tasks;

    mutex queueMutex;
    condition_variable condition;
    bool stop;
};

bool recursion(ThreadPool& pool, string filePath, string fileName) {
    vector<string> filePaths = findFileInDirectory(filePath, fileName);
    if (filePaths.size() != 0) {
        if (filePaths[1] == fileName) {
            cout << filePaths[0] + filePaths[2] << endl;
            pool.stopExecution();
            return true;
        } else {
            for (int i = 1; i < filePaths.size(); i++) {
                if (recursion(pool, filePaths[0] + filePaths[i], fileName))
                    return true;
            }
        }
    }
    return false;
}

int main() {
    string fileName = "";
    cout << "Enter Filename: ";
    cin >> fileName;

    cout << "Looking for file with name: " << fileName << endl;

    int numThreads = thread::hardware_concurrency();
    ThreadPool pool(numThreads > 8 ? 8 : numThreads);

    if(!recursion(pool, "C:\\", fileName)){
        cout << "Nothing found" << endl;
    }

    system("pause");

    return 0;
}
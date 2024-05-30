#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <algorithm>
#include <sstream>
#include <queue>
#include <string>
#include <cstring> // For strcpy_s

using namespace std;

class Process {
public:
    int id;
    bool isForeground;
    string command;
    bool promoted = false;
    int remainingTime = 0; // for wait queue

    Process(int id, bool isForeground, const string& command)
        : id(id), isForeground(isForeground), command(command), promoted(false) {}

    string toString() const {
        stringstream ss;
        ss << id << (isForeground ? "F" : "B");
        if (promoted) ss << "*";
        return ss.str();
    }
};

class DynamicQueue {
private:
    mutex mtx;
    condition_variable cv;
    list<shared_ptr<Process>> fgProcesses;
    list<shared_ptr<Process>> bgProcesses;
    map<int, shared_ptr<Process>> waitQueue;
    atomic<int> processCount{ 0 };
    atomic<int> bgCount{ 0 };

public:
    void enqueue(shared_ptr<Process> process) {
        lock_guard<mutex> lock(mtx);
        if (process->isForeground) {
            fgProcesses.push_back(process);
        }
        else {
            bgProcesses.push_back(process);
            bgCount++;
        }
        processCount++;
    }

    void simulateSleep(int pid, int seconds) {
        lock_guard<mutex> lock(mtx);
        auto proc = find_if(fgProcesses.begin(), fgProcesses.end(), [&pid](const auto& p) { return p->id == pid; });
        if (proc != fgProcesses.end()) {
            waitQueue[pid] = *proc;
            fgProcesses.erase(proc);
        }
        else {
            proc = find_if(bgProcesses.begin(), bgProcesses.end(), [&pid](const auto& p) { return p->id == pid; });
            if (proc != bgProcesses.end()) {
                waitQueue[pid] = *proc;
                bgProcesses.erase(proc);
                bgCount--;
            }
        }
        waitQueue[pid]->remainingTime = seconds;
    }

    void wakeUpProcesses() {
        lock_guard<mutex> lock(mtx);
        for (auto it = waitQueue.begin(); it != waitQueue.end();) {
            if (--it->second->remainingTime <= 0) {
                if (it->second->isForeground) {
                    fgProcesses.push_back(it->second);
                }
                else {
                    bgProcesses.push_back(it->second);
                    bgCount++;
                }
                it = waitQueue.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void printQueue() {
        lock_guard<mutex> lock(mtx);
        cout << "Running: [" << bgCount.load() << "B]\n";
        cout << "---------------------------\n";
        cout << "DQ: (bottom) ";
        for (const auto& proc : bgProcesses) {
            cout << "[" << proc->toString() << "] ";
        }
        cout << "\nP => ";
        for (const auto& proc : fgProcesses) {
            cout << "[" << proc->toString() << "] ";
        }
        cout << "(top)\n";
        cout << "---------------------------\n";
        cout << "WQ: ";
        for (const auto& wp : waitQueue) {
            cout << "[" << wp.second->toString() << ": " << wp.second->remainingTime << "s] ";
        }
        cout << "\n...\n";
    }
};

void shellProcess(DynamicQueue& dq, int interval) {
    vector<string> commands = { "alarm clock", "todo list", "email check", "music player", "video player", "web browser" };
    int processId = 2; // Starting from 2, since 0 and 1 are taken by shell and monitor
    for (const string& line : commands) {
        bool isForeground = (processId % 2 == 0);
        shared_ptr<Process> proc = make_shared<Process>(processId++, isForeground, line);
        dq.enqueue(proc);
        if (processId % 3 == 0) {
            dq.simulateSleep(proc->id, interval * 2); // 일부 프로세스를 대기 큐로 보냄
        }
        this_thread::sleep_for(chrono::seconds(interval));
        dq.printQueue();
    }
}

void monitorProcess(DynamicQueue& dq, int interval) {
    while (true) {
        this_thread::sleep_for(chrono::seconds(interval));
        dq.wakeUpProcesses();
        dq.printQueue();
    }
}

char** parse(const char* command) {
    vector<string> tokens;
    stringstream ss(command);
    string token;
    while (ss >> token) {
        tokens.push_back(token);
    }

    char** args = new char* [tokens.size() + 1];
    for (size_t i = 0; i < tokens.size(); ++i) {
        args[i] = new char[tokens[i].size() + 1];
        strcpy_s(args[i], tokens[i].size() + 1, tokens[i].c_str());
    }
    args[tokens.size()] = nullptr;

    return args;
}

void exec(char** args) {
    // Simulate command execution (replace with actual system call)
    cout << "Executing command: ";
    for (int i = 0; args[i] != nullptr; ++i) {
        cout << args[i] << " ";
    }
    cout << endl;

    // Free memory
    for (int i = 0; args[i] != nullptr; ++i) {
        delete[] args[i];
    }
    delete[] args;
}

int main() {
    DynamicQueue dq;
    shared_ptr<Process> shellProc = make_shared<Process>(0, true, "shell");
    shared_ptr<Process> monitorProc = make_shared<Process>(1, false, "monitor");

    dq.enqueue(shellProc);
    dq.enqueue(monitorProc);

    thread shellThread(shellProcess, ref(dq), 5); // Simulate shell processing
    thread monitorThread(monitorProcess, ref(dq), 10);

    shellThread.join();
    monitorThread.detach();

    // Parse and execute example command
    const char* command = "example command";
    char** args = parse(command);
    exec(args);

    return 0;
}

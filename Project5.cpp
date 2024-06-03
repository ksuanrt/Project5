//2-1 Dynamic Queueing
//#include <iostream>
//#include <queue>
//#include <list>
//#include <thread>
//#include <mutex>
//#include <condition_variable>
//#include <atomic>
//#include <chrono>
//#include <vector>
//#include <cmath>
//#include <unordered_map>
//
//using namespace std;
//
//class Process {
//public:
//    int id;
//    bool isForeground;
//
//    Process(int id, bool isForeground) : id(id), isForeground(isForeground) {}
//};
//
//class DynamicQueue {
//private:
//    vector<list<Process>> queues;
//    list<Process> waitQueue;
//    mutex mtx;
//    condition_variable cv;
//    atomic<bool> running;
//    int threshold;
//    size_t pIndex; // Promote index
//
//public:
//    DynamicQueue(int numQueues, int threshold) : running(true), threshold(threshold), pIndex(0) {
//        queues.resize(numQueues);
//    }
//
//    void enqueue(Process process) {
//        unique_lock<mutex> lock(mtx);
//        if (process.isForeground) {
//            queues.back().push_back(process);
//        }
//        else {
//            queues.front().push_back(process);
//        }
//        split_and_merge();
//        cv.notify_all();
//    }
//
//    Process dequeue() {
//        unique_lock<mutex> lock(mtx);
//        cv.wait(lock, [this] { return !queues.back().empty() || !queues.front().empty(); });
//
//        Process process(-1, false);
//        if (!queues.back().empty()) {
//            process = queues.back().front();
//            queues.back().pop_front();
//            if (queues.back().empty() && queues.size() > 1) {
//                queues.pop_back();
//            }
//        }
//        else if (!queues.front().empty()) {
//            process = queues.front().front();
//            queues.front().pop_front();
//        }
//
//        promote();
//        return process;
//    }
//
//    void promote() {
//        if (queues[pIndex].empty()) {
//            pIndex = (pIndex + 1) % queues.size();
//            return;
//        }
//
//        Process process = queues[pIndex].front();
//        queues[pIndex].pop_front();
//
//        size_t nextIndex = (pIndex + 1) % queues.size();
//        queues[nextIndex].push_back(process);
//
//        if (queues[pIndex].empty() && queues.size() > 1) {
//            queues.erase(queues.begin() + pIndex);
//        }
//
//        pIndex = (pIndex + 1) % queues.size();
//    }
//
//    void split_and_merge() {
//        for (size_t i = 0; i < queues.size(); ++i) {
//            if (queues[i].size() > threshold) {
//                auto it = queues[i].begin();
//                advance(it, queues[i].size() / 2);
//
//                list<Process> newList;
//                newList.splice(newList.begin(), queues[i], queues[i].begin(), it);
//                if (i + 1 < queues.size()) {
//                    queues[i + 1].splice(queues[i + 1].end(), newList);
//                }
//                else {
//                    queues.push_back(newList);
//                }
//            }
//        }
//    }
//
//    void print_status() {
//        unique_lock<mutex> lock(mtx);
//        cout << "Running: [1B]" << endl;  // Always print "Running: [1B]"
//        cout << "---------------------------" << endl;
//        cout << "DQ: ";
//        for (auto& process : queues.front()) {
//            cout << process.id << (process.isForeground ? "F " : "B ");
//        }
//        cout << "(bottom)";
//        cout << endl;
//
//        cout << "P => ";
//        for (auto& process : queues.back()) {
//            cout << process.id << (process.isForeground ? "F " : "B ");
//        }
//        cout << "(top)";
//        cout << endl;
//
//        cout << "WQ: ";
//        for (auto& process : waitQueue) {
//            cout << process.id << (process.isForeground ? "F" : "B") << " ";
//        }
//        cout << endl;
//        cout << "---------------------------" << endl;
//    }
//
//    void add_to_wait_queue(Process process) {
//        unique_lock<mutex> lock(mtx);
//        waitQueue.push_back(process);
//    }
//
//    void remove_from_wait_queue(Process process) {
//        unique_lock<mutex> lock(mtx);
//        waitQueue.remove_if([&process](const Process& p) { return p.id == process.id; });
//    }
//
//    bool is_running() const {
//        return running;
//    }
//
//    void stop() {
//        running = false;
//        cv.notify_all();
//    }
//};
//
//void fg_process(DynamicQueue& dq) {
//    for (int i = 0; i < 10; ++i) { // Simulating 10 FG processes
//        Process p(i, true);
//        dq.enqueue(p);
//        this_thread::sleep_for(chrono::seconds(1));
//        dq.add_to_wait_queue(p);  // Simulate process moving to wait queue
//    }
//    dq.stop();
//}
//
//void bg_monitor(DynamicQueue& dq) {
//    while (dq.is_running()) {
//        dq.print_status();
//        this_thread::sleep_for(chrono::seconds(1));
//    }
//}
//
//int main() {
//    const int numQueues = 2;
//    const int threshold = 3;
//    DynamicQueue dq(numQueues, threshold);
//
//    thread fg_thread(fg_process, ref(dq));
//    thread bg_thread(bg_monitor, ref(dq));
//
//    fg_thread.join();
//    bg_thread.join();
//
//    return 0;
//}

////2-2 Alarm Clock
//#include <iostream>
//#include <list>
//#include <memory>
//#include <mutex>
//#include <thread>
//#include <vector>
//#include <atomic>
//#include <chrono>
//#include <condition_variable>
//#include <map>
//#include <algorithm>
//#include <sstream>
//
//using namespace std;
//
//class Process {
//public:
//    int id;
//    bool isForeground;
//    string command;
//    bool promoted = false;
//    int remainingTime = 0; // for wait queue
//
//    Process(int id, bool isForeground, const string& command)
//        : id(id), isForeground(isForeground), command(command), promoted(false) {}
//
//    string toString() const {
//        stringstream ss;
//        ss << "[" << id << (isForeground ? "F" : "B");
//        if (promoted) ss << "*";
//        ss << "]";
//        return ss.str();
//    }
//
//    string toStringForWaitQueue() const {
//        stringstream ss;
//        ss << id << (isForeground ? "F" : "B") << ":" << remainingTime;
//        return ss.str();
//    }
//};
//
//class DynamicQueue {
//private:
//    mutex mtx;
//    condition_variable cv;
//    list<shared_ptr<Process>> fgProcesses;
//    list<shared_ptr<Process>> bgProcesses;
//    map<int, shared_ptr<Process>> waitQueue;
//    atomic<int> processCount{ 0 };
//    atomic<int> bgCount{ 0 };
//
//public:
//    void enqueue(shared_ptr<Process> process) {
//        lock_guard<mutex> lock(mtx);
//        if (process->isForeground) {
//            fgProcesses.push_back(process);
//        }
//        else {
//            bgProcesses.push_back(process);
//            bgCount++;
//        }
//        processCount++;
//    }
//
//    void simulateSleep(int pid, int seconds) {
//        lock_guard<mutex> lock(mtx);
//        auto it = find_if(fgProcesses.begin(), fgProcesses.end(), [&pid](const auto& p) { return p->id == pid; });
//        if (it != fgProcesses.end()) {
//            waitQueue[pid] = *it;
//            fgProcesses.erase(it);
//        }
//        else {
//            it = find_if(bgProcesses.begin(), bgProcesses.end(), [&pid](const auto& p) { return p->id == pid; });
//            if (it != bgProcesses.end()) {
//                waitQueue[pid] = *it;
//                bgProcesses.erase(it);
//                bgCount--;
//            }
//        }
//        waitQueue[pid]->remainingTime = seconds;
//    }
//
//    void wakeUpProcesses() {
//        lock_guard<mutex> lock(mtx);
//        for (auto it = waitQueue.begin(); it != waitQueue.end();) {
//            if (--it->second->remainingTime <= 0) {
//                if (it->second->isForeground) {
//                    fgProcesses.push_back(it->second);
//                }
//                else {
//                    bgProcesses.push_back(it->second);
//                    bgCount++;
//                }
//                it = waitQueue.erase(it);
//            }
//            else {
//                ++it;
//            }
//        }
//    }
//
//    void printQueue() {
//        lock_guard<mutex> lock(mtx);
//        cout << "Running: [" << bgCount.load() << "B]\n";
//        cout << "---------------------------\n";
//        cout << "DQ: ";
//        if (!bgProcesses.empty()) {
//            cout << "(bottom) ";
//            for (const auto& proc : bgProcesses) {
//                cout << proc->toString() << " ";
//            }
//            cout << "\nP => ";
//        }
//        if (!fgProcesses.empty()) {
//            for (const auto& proc : fgProcesses) {
//                cout << proc->toString() << " ";
//            }
//            cout << "(top)\n";
//        }
//        else {
//            cout << "(bottom/top)\n";
//        }
//        cout << "---------------------------\n";
//        cout << "WQ: [";
//        for (const auto& wp : waitQueue) {
//            cout << wp.second->toStringForWaitQueue() << " ";
//        }
//        cout << "]\n...\n";
//    }
//};
//
//void shellProcess(DynamicQueue& dq, int interval) {
//    vector<string> commands = { "alarm clock", "todo list", "email check" };
//    int processId = 2; // Starting from 2, since 0 and 1 are taken by shell and monitor
//    for (const string& line : commands) {
//        shared_ptr<Process> proc = make_shared<Process>(processId++, true, line);
//        dq.enqueue(proc);
//        this_thread::sleep_for(chrono::seconds(interval));
//        if (processId % 3 == 0) dq.simulateSleep(processId - 1, 5);
//        dq.printQueue();
//    }
//}
//
//void monitorProcess(DynamicQueue& dq, int interval) {
//    while (true) {
//        this_thread::sleep_for(chrono::seconds(interval));
//        dq.wakeUpProcesses();
//        dq.printQueue();
//    }
//}
//
//int main() {
//    DynamicQueue dq;
//    shared_ptr<Process> shellProc = make_shared<Process>(0, true, "shell");
//    shared_ptr<Process> monitorProc = make_shared<Process>(1, false, "monitor");
//
//    dq.enqueue(shellProc);
//    dq.enqueue(monitorProc);
//
//    thread shellThread(shellProcess, ref(dq), 1); // Simulate shell processing
//    thread monitorThread(monitorProcess, ref(dq), 2); // Monitor background tasks
//
//    shellThread.join();
//    monitorThread.detach();
//
//    return 0;
//}
// 2-3 CLI
//#include <iostream>
//#include <fstream>
//#include <sstream>
//#include <thread>
//#include <mutex>
//#include <condition_variable>
//#include <vector>
//#include <queue>
//#include <functional>
//#include <chrono>
//#include <atomic>
//
//using namespace std;
//
//mutex coutMutex;
//condition_variable cv;
//atomic<bool> isRunning(true);
//
//class Process {
//public:
//    string command;
//    bool isForeground;
//    int period;
//    int duration;
//
//    Process(string cmd, bool fg, int per, int dur)
//        : command(cmd), isForeground(fg), period(per), duration(dur) {}
//
//    void execute() {
//        lock_guard<mutex> lock(coutMutex);
//        cout << command.substr(5) << endl; // 실행 로직, 여기서는 간단히 echo만 처리
//    }
//};
//
//void worker(queue<shared_ptr<Process>>& workQueue) {
//    while (isRunning && !workQueue.empty()) {
//        auto process = workQueue.front();
//        workQueue.pop();
//        if (process) {
//            process->execute();
//            this_thread::sleep_for(chrono::seconds(process->period)); // 주어진 주기만큼 대기
//        }
//    }
//}
//
//void parseAndExecute(const string& commandLine) {
//    istringstream stream(commandLine);
//    string command, segment;
//    queue<shared_ptr<Process>> workQueue;
//
//    while (getline(stream, segment, ';')) {
//        bool isForeground = segment.find('&') == string::npos;
//        string cmd = isForeground ? segment : segment.substr(1); // '&' 제거
//        int period = 1; // 기본 주기
//        int duration = 50; // 기본 지속 시간
//        auto process = make_shared<Process>(cmd, isForeground, period, duration);
//        workQueue.push(process);
//    }
//
//    thread th([&workQueue] {
//        while (!workQueue.empty()) {
//            auto process = workQueue.front();
//            workQueue.pop();
//            if (process && process->isForeground) {
//                process->execute();
//                this_thread::sleep_for(chrono::seconds(process->period));
//            }
//            else {
//                // 배경 작업 실행
//                thread bgThread(&Process::execute, process);
//                bgThread.detach();
//            }
//        }
//        });
//    th.join();
//}
//
//int main() {
//    string commandLine;
//    while (true) {
//        cout << "prompt> ";
//        getline(cin, commandLine);
//        if (commandLine == "exit") {
//            isRunning = false;
//            cv.notify_all();
//            break;
//        }
//        parseAndExecute(commandLine);
//    }
//    return 0;
//}
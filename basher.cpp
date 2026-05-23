#include "basher.h"
#include "http.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <chrono>
#include <curl/curl.h>

using namespace std;

static mutex printLock;
static mutex poolLock;
static mutex workLock;

struct WorkItem {
    string word;
    int retries;
};

static queue<WorkItem> workQueue;
static vector<string> foundPaths;
static queue<CURL*> connectionPool;

static atomic<int> totalWords{0};
static atomic<int> remaining{0};
static atomic<int> activeThreads{0};
static atomic<int> maxThreads{10};
static atomic<int> minThreads{2};
static atomic<int> errors429{0};
static atomic<bool> running{false};

static string globalCookie = "";
static string outputFile = "";
static vector<string> extensions;

static void redrawBar(int done, int total) {
    if (total == 0) return;
    constexpr int barWidth = 30;
    int percent = (done * 100) / total;
    int filled = (done * barWidth) / total;
    string bar = "[";
    for (int i = 0; i < barWidth; i++) {
        if (i < filled) bar += "=";
        else if (i == filled) bar += ">";
        else bar += " ";
    }
    bar += "] " + to_string(percent) + "% | " + to_string(done) + "/" + to_string(total);
    bar += " | Threads: " + to_string(activeThreads.load()) + "                    ";
    cout << "\033[s\033[9999;0H\r" << bar << "\033[u" << flush;
}

static void printFound(const string& entry, int done, int total) {
    cout << "\033[s\033[9999;0H\033[2K\033[u" << entry << "\n" << flush;
    redrawBar(done, total);
}

static CURL* getConnection() {
    lock_guard<mutex> lock(poolLock);
    if (!connectionPool.empty()) {
        CURL* conn = connectionPool.front();
        connectionPool.pop();
        return conn;
    }
    return createConnection(globalCookie);
}

static void returnConnection(CURL* conn) {
    if (!conn) return;
    lock_guard<mutex> lock(poolLock);
    connectionPool.push(conn);
}

static void adjustThreads() {
    int e = errors429.load();
    int cur = maxThreads.load();
    int mn = minThreads.load();
    if (e > 50) {
        maxThreads.store(max(mn, cur / 2));
        errors429.store(0);
    } else if (e == 0 && cur < 50) {
        maxThreads.store(min(50, cur + 2));
    }
}

static vector<string> buildWordList(const vector<string>& base, const vector<string>& exts) {
    vector<string> result;
    result.reserve(base.size() * max((size_t)1, exts.size() + 1));
    for (const string& word : base) {
        result.push_back(word);
        for (const string& ext : exts) {
            result.push_back(word + ext);
        }
    }
    return result;
}

static void scanWorker(const string& baseUrl, int) {
    activeThreads.fetch_add(1);
    CURL* curl = getConnection();
    if (!curl) {
        activeThreads.fetch_sub(1);
        return;
    }

    while (running.load()) {
        WorkItem item;
        {
            lock_guard<mutex> lock(workLock);
            if (workQueue.empty()) break;
            item = workQueue.front();
            workQueue.pop();
        }

        string fullUrl = baseUrl + "/" + item.word;
        HttpResponse result = sendGet(curl, fullUrl);

        if (result.statusCode == 429) {
            errors429.fetch_add(1);
            adjustThreads();
            if (item.retries < 3) {
                item.retries++;
                lock_guard<mutex> lock(workLock);
                workQueue.push(item);
            } else {
                int done = totalWords.load() - remaining.fetch_sub(1) + 1;
                lock_guard<mutex> lock(printLock);
                redrawBar(done, totalWords.load());
            }
            this_thread::sleep_for(chrono::milliseconds(500));
            continue;
        }

        int done = totalWords.load() - remaining.fetch_sub(1) + 1;

        if (result.statusCode == 200 || result.statusCode == 301 ||
            result.statusCode == 302 || result.statusCode == 403) {

            string msg;
            switch (result.statusCode) {
                case 200: msg = "200 OK"; break;
                case 301: msg = "301 Moved"; break;
                case 302: msg = "302 Redirect"; break;
                case 403: msg = "403 Forbidden"; break;
            }

            string entry = "[+] /" + item.word + " [" + msg + "]";
            lock_guard<mutex> lock(printLock);
            foundPaths.push_back("/" + item.word + " [" + msg + "]");
            printFound(entry, done, totalWords.load());
        } else {
            lock_guard<mutex> lock(printLock);
            redrawBar(done, totalWords.load());
        }
    }

    returnConnection(curl);
    activeThreads.fetch_sub(1);
}

void startBruteForce(Args config) {
    cout << "Target : " << config.url << "\n";
    cout << "Threads: " << config.threads << "\n";

    {
        lock_guard<mutex> lp(poolLock);
        while (!connectionPool.empty()) connectionPool.pop();
    }
    {
        lock_guard<mutex> lw(workLock);
        while (!workQueue.empty()) workQueue.pop();
    }
    foundPaths.clear();
    errors429.store(0);
    running.store(true);

    int threads = max(1, config.threads);
    maxThreads.store(threads);
    minThreads.store(max(1, threads / 5));

    globalCookie = config.cookie;
    outputFile = config.output;
    extensions = config.extensions;

    {
        ifstream file(config.wordlist);
        if (!file.is_open()) {
            cout << "[-] Error: Cannot open " << config.wordlist << "\n";
            return;
        }
        string word;
        vector<string> base;
        while (getline(file, word)) {
            if (!word.empty()) base.push_back(word);
        }
        vector<string> all = buildWordList(base, extensions);
        totalWords.store((int)all.size());
        remaining.store((int)all.size());
        lock_guard<mutex> lw(workLock);
        for (const string& w : all) workQueue.push({w, 0});
    }

    cout << "Words  : " << totalWords.load() << "\n";
    cout << "\n";

    {
        lock_guard<mutex> lp(poolLock);
        for (int i = 0; i < threads * 2; i++) {
            CURL* c = createConnection(globalCookie);
            if (c) connectionPool.push(c);
        }
    }

    vector<thread> workers;
    workers.reserve(threads);
    for (int i = 0; i < threads; i++) {
        workers.emplace_back(scanWorker, config.url, i);
    }
    for (auto& t : workers) t.join();

    running.store(false);

    {
        lock_guard<mutex> lock(printLock);
        redrawBar(totalWords.load(), totalWords.load());
        cout << "\n";
    }

    if (!foundPaths.empty()) {
        cout << "\nFound " << foundPaths.size() << " paths:\n";
        for (const string& path : foundPaths) {
            cout << "  " << path << "\n";
        }
    } else {
        cout << "\nNo paths found.\n";
    }

    if (!outputFile.empty() && !foundPaths.empty()) {
        ofstream outFile(outputFile);
        if (outFile.is_open()) {
            for (const string& path : foundPaths) {
                outFile << path << "\n";
            }
            cout << "[+] Results saved to: " << outputFile << "\n";
        } else {
            cout << "[-] Error: Cannot write to " << outputFile << "\n";
        }
    }

    {
        lock_guard<mutex> lp(poolLock);
        while (!connectionPool.empty()) {
            destroyConnection(connectionPool.front());
            connectionPool.pop();
        }
    }
}

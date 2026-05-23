#include "basher.h"
#include "http.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <curl/curl.h>

using namespace std;

mutex queueLock;
mutex printLock;
mutex poolLock;
vector<string> wordQueue;
vector<string> foundPaths;
vector<string> extensions;
queue<CURL*> connectionPool;
int currentIndex = 0;
int totalWords = 0;
int attempts = 0;
int activeThreads = 0;
int maxThreads = 10;
int minThreads = 2;
int errors429 = 0;
bool running = true;
string globalCookie = "";
string outputFile = "";

void printBar(int current, int total) {
    int barWidth = 30;
    int percent = (current * 100) / total;
    int filled = (current * barWidth) / total;
    
    string bar = "\r[";
    for (int i = 0; i < barWidth; i++) {
        if (i < filled) bar += "=";
        else if (i == filled) bar += ">";
        else bar += " ";
    }
    bar += "] " + to_string(percent) + "% | " + to_string(current) + "/" + to_string(total);
    bar += " | Threads: " + to_string(activeThreads);
    
    cout << bar << flush;
}

CURL* getConnection() {
    lock_guard<mutex> lock(poolLock);
    if (!connectionPool.empty()) {
        CURL* conn = connectionPool.front();
        connectionPool.pop();
        return conn;
    }
    return createConnection(globalCookie);
}

void returnConnection(CURL* conn) {
    lock_guard<mutex> lock(poolLock);
    connectionPool.push(conn);
}

void adjustThreads() {
    if (errors429 > 50) {
        maxThreads = max(minThreads, maxThreads / 2);
        errors429 = 0;
    }
    else if (errors429 == 0 && maxThreads < 50) {
        maxThreads = min(50, maxThreads + 2);
    }
}

vector<string> getWords() {
    vector<string> result;
    
    for (string word : wordQueue) {
        if (!extensions.empty()) {
            result.push_back(word);
            for (string ext : extensions) {
                result.push_back(word + ext);
            }
        }
        else {
            result.push_back(word);
        }
    }
    
    return result;
}

void scanWorker(string baseUrl, int threadId) {
    CURL* curl = getConnection();
    if (!curl) return;
    
    while (running) {
        string word;
        
        {
            lock_guard<mutex> lock(queueLock);
            if (currentIndex >= totalWords) break;
            word = wordQueue[currentIndex];
            currentIndex++;
        }
        
        string fullUrl = baseUrl + "/" + word;
        HttpResponse result = sendGet(curl, fullUrl);
        
        {
            lock_guard<mutex> lock(printLock);
            attempts++;
            
            if (result.statusCode == 429) {
                errors429++;
                adjustThreads();
            }
            
            if (result.statusCode == 200 || result.statusCode == 301 || 
                result.statusCode == 302 || result.statusCode == 403) {
                
                string msg;
                if (result.statusCode == 200) msg = "200 OK";
                else if (result.statusCode == 301) msg = "301 Moved";
                else if (result.statusCode == 302) msg = "302 Redirect";
                else if (result.statusCode == 403) msg = "403 Forbidden";
                
                foundPaths.push_back("/" + word + " [" + msg + "]");
                cout << "\r\033[K[+] /" << word << " [" << msg << "]" << endl;
            }
            
            if (attempts % 10 == 0 || attempts == totalWords) {
                printBar(attempts, totalWords);
            }
        }
    }
    
    returnConnection(curl);
}

void startBruteForce(Args config) {
    cout << "====================================" << endl;
    cout << "  Basher - Directory Brute Forcer" << endl;
    cout << "====================================" << endl;
    cout << "Target : " << config.url << endl;
    cout << "Threads: " << config.threads << endl;
    
    maxThreads = config.threads;
    minThreads = config.threads / 5;
    if (minThreads < 1) minThreads = 1;
    
    ifstream file(config.wordlist);
    if (!file.is_open()) {
        cout << "[-] Error: Cannot open " << config.wordlist << endl;
        return;
    }
    
    string word;
    while (getline(file, word)) {
        if (!word.empty()) wordQueue.push_back(word);
    }
    file.close();
    
    globalCookie = config.cookie;
    outputFile = config.output;
    extensions = config.extensions;
    
    wordQueue = getWords();
    totalWords = wordQueue.size();
    currentIndex = 0;
    attempts = 0;
    errors429 = 0;
    running = true;
    
    cout << "Words  : " << totalWords << endl;
    cout << "------------------------------------" << endl;
    
    for (int i = 0; i < maxThreads * 2; i++) {
        connectionPool.push(createConnection(globalCookie));
    }
    
    vector<thread> workers;
    activeThreads = maxThreads;
    
    for (int i = 0; i < maxThreads; i++) {
        workers.push_back(thread(scanWorker, config.url, i));
    }
    
    for (auto& t : workers) {
        t.join();
    }
    
    running = false;
    
    cout << "\r\033[KProgress: [";
    for (int i = 0; i < 30; i++) cout << "=";
    cout << "] 100% | " << totalWords << "/" << totalWords << endl;
    
    cout << "\n====================================" << endl;
    if (!foundPaths.empty()) {
        cout << "Found " << foundPaths.size() << " paths:" << endl;
        for (string path : foundPaths) {
            cout << "  " << path << endl;
        }
    }
    else {
        cout << "No paths found." << endl;
    }
    cout << "====================================" << endl;
    
    if (!outputFile.empty() && !foundPaths.empty()) {
        ofstream outFile(outputFile);
        if (outFile.is_open()) {
            for (string path : foundPaths) {
                outFile << path << endl;
            }
            outFile.close();
            cout << "\n[+] Results saved to: " << outputFile << endl;
        }
        else {
            cout << "\n[-] Error: Cannot write to " << outputFile << endl;
        }
    }

    while (!connectionPool.empty()) {
        destroyConnection(connectionPool.front());
        connectionPool.pop();
    }
}
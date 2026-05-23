#ifndef ARGS_H
#define ARGS_H
#include <string>
#include <vector>
using namespace std;

struct Args {
    string url;
    string wordlist;
    int threads;
    string cookie;
    string output;
    vector<string> extensions;
};
Args parseArgs(int argc, char* argv[]);

#endif
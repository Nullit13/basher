#include "args.h"
#include <iostream>
#include <string>
#include <sstream>

Args parseArgs(int argc, char* argv[]) {
    Args config;
    config.url = "";
    config.wordlist = "";
    config.threads = 1;
    config.cookie = "";
    config.output = "";
    
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-u" && i + 1 < argc) {
            config.url = argv[++i];
        }
        else if (arg == "-w" && i + 1 < argc) {
            config.wordlist = argv[++i];
        }
        else if (arg == "-T" && i + 1 < argc) {
            config.threads = stoi(argv[++i]);
        }
        else if (arg == "-c" && i + 1 < argc) {
            config.cookie = argv[++i];
        }
        else if (arg == "-o" && i + 1 < argc) {
            config.output = argv[++i];
        }
        else if (arg == "-x" && i + 1 < argc) {
            string exts = argv[++i];
            stringstream ss(exts);
            string ext;
            while (getline(ss, ext, ',')) {
                if (!ext.empty()) {
                    if (ext[0] != '.') ext = "." + ext;
                    config.extensions.push_back(ext);
                }
            }
        }
    }
    return config;
}
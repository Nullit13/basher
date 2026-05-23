#include <iostream>
#include "args.h"
#include "basher.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Basher - Directory Brute Forcer\n";
        cout << "===============================\n\n";
        cout << "Usage: basher -u <url> -w <wordlist> [-T threads]\n\n";
        cout << "Required:\n";
        cout << "  -u   Target URL\n";
        cout << "  -w   Wordlist path\n\n";
        cout << "Optional:\n";
        cout << "  -T   Number of threads (default: 1)\n\n";
        cout << "  -c   Cookie (e.g. \"PHPSESSID=abc123\")\n\n";
        cout << "  -o   Output file (save results)\n\n";
        cout << "  -x   Extensions (e.g. php,html,bak)\n\n";
        cout << "Example:\n";
        cout << "  basher -u https://example.com -w dirs.txt -T 10\n";
        return 0;
    }
    
    Args config = parseArgs(argc, argv);
    startBruteForce(config);
    return 0;
}
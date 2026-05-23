# ⚡ Basher - Blazing Fast Directory Brute Forcer

A high-performance web directory scanner built in C++. Faster than Gobuster.

![Version](https://img.shields.io/badge/version-1.0-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B-red)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20Mac-lightgrey)

## 🚀 Features

- **Blazing Fast** - HTTP/2, connection pooling, pipelining
- **Multi-threaded** - Configurable threads with auto-calibration
- **Smart Rate Limiting** - Auto-adjusts speed to avoid blocks
- **Extensions** - Brute force multiple file types
- **Cookie Support** - Scan authenticated pages
- **Real-time Progress Bar** - See progress at a glance
- **Output to File** - Save results for later

## 📦 Installation

### Prerequisites
- g++ (MinGW on Windows)
- libcurl

### Build
```bash
git clone https://github.com/Nullit13/basher.git
cd basher
g++ main.cpp args.cpp http.cpp basher.cpp -o basher -lcurl
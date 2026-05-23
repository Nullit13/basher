#ifndef HTTP_H
#define HTTP_H
#include <string>

#define byte windows_byte
#include <curl/curl.h>
#undef byte

using namespace std;

struct HttpResponse {
    int statusCode;
};

HttpResponse sendGet(CURL* curl, string url);
CURL* createConnection(string cookie);
void destroyConnection(CURL* curl);

#endif
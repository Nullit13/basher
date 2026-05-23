#include "http.h"

HttpResponse sendGet(CURL* curl, string url) {
    HttpResponse response;
    response.statusCode = 0;
    
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.statusCode);
    }
    
    return response;
}

CURL* createConnection(string cookie) {
    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, 60L);
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
        curl_easy_setopt(curl, CURLOPT_PIPEWAIT, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_FASTOPEN, 1L);
        
        if (!cookie.empty()) {                                      // NEW
            curl_easy_setopt(curl, CURLOPT_COOKIE, cookie.c_str());
        }
    }
    return curl;
}

void destroyConnection(CURL* curl) {
    if (curl) {
        curl_easy_cleanup(curl);
    }
}
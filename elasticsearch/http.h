/*
* Filename: http.h
* Language: C
* Purpose:  Provide simplified functions for making http
*           calls with common parameters using the libcurl
*           libraries. 
*/

#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED
#endif

#include <curl/curl.h>

/* structure to hold payload and size */
struct http_payload {
    char *payload;
    size_t size;
};

/* function to write the response into the struct */
size_t write_http_response (void *contents, 
    size_t size, 
    size_t nmemb, 
    void *userp);

/* function to initialize headers from a string */
void init_headers(CURL *handle, 
    char **headers, 
    int headers_count);

/* initialize the http payload */
void init_payload (struct http_payload *body);

/* make http request */
char *request_http(const char *url, 
    const char *action, 
    char **headers, 
    int headers_count, 
    const char *body);

/* call http get with json content headers */
char *get_http_json(const char *url);

/* call http delete */
char *delete_http_json(const char *url);

/* call http post with json content headers */
char *post_http_json(const char *url, const char *body);

/* call http put with json content headers */
char *put_http_json(const char *url, const char *body);
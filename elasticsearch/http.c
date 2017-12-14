/**
 * 
 * json-c - https://github.com/json-c/json-c
 * libcurl - http://curl.haxx.se/libcurl/c
 *
 * cc http.c -lcurl -ljson-c -o httptest
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <json-c/json.h>
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
void init_headers(CURL *handle, char **headers, int headers_count);

/* initialize the http payload */
void init_payload (struct http_payload *body);

/* fetch and return url body via curl */
char *request_http(const char *url, const char *action, char **headers, int headers_count, const char *body) {

    /* init curl handle */
    CURL *handle = curl_easy_init();

    struct http_payload curl_fetch;         /* curl fetch struct */
    struct http_payload *cf = &curl_fetch;  /* pointer to fetch struct */

    /* init payload */
    init_payload(cf);

    /* set url to fetch */
    curl_easy_setopt(handle, CURLOPT_URL, url);

    /* set curl headers */
    init_headers(handle, headers, headers_count);

    /* set request body */
    if (body != NULL)
    {
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, body);
    }

    /* set http action */
    curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, action);

    /* set calback function */
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_http_response);

    /* pass fetch struct pointer */
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *) cf);

    /* set timeout */
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, 5);

    /* enable location redirects */
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);

    /* perform http request */
    int curl_result = curl_easy_perform(handle);

    /* cleanup curl handle */
    curl_easy_cleanup(handle);

    /* make the http request */
    if (curl_result != CURLE_OK || cf->size < 1) {
        /* log error */
        fprintf(stderr, "ERROR: Failed to fetch url (%s) - curl said: %s",
            url, curl_easy_strerror(curl_result));
        /* return error */
        return "";
    }

    /* return */
    return cf->payload;
}

int main(int argc, char *argv[]) {

    json_object *json;                                      /* json post body */
    enum json_tokener_error jerr = json_tokener_success;    /* json parse error */
    char *content = NULL;

    char *get_headers[] = {};
    content = request_http("http://dimuthu.org", "GET", get_headers, 0, NULL);
    printf("%s", content);

    /* fetch page and capture return code */
    char *post_headers[] = {"Accept: application/json","Content-Type: application/json"};
    char *body = "{\"title\":\"testies\", \"body\":\"testies ... testies ... 1,2,3\", \"userId\":133}";
    content = request_http("http://jsonplaceholder.typicode.com/posts/", "POST", post_headers, 2, body);
    printf("%s", content);

    /* exit */
    return 0;
}

/* initialize the http payload structure */
void init_payload(struct http_payload *body) {
        /* init payload */
    body->payload = (char *) calloc(1, sizeof(body->payload));

    /* check payload */
    if (body->payload == NULL) {
        body->payload = "";
    }

    /* init size */
    body->size = 0;
}

/* set headers from string array */
void init_headers(CURL *handle, char **headers, int headers_count) {
    struct curl_slist *curl_headers = NULL;
    for(int i=0;i<headers_count;i++)
    {
        curl_headers = curl_slist_append(curl_headers, headers[i]);
    }
    if(curl_headers != NULL) 
    {
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, curl_headers);
    }
}

/* callback for curl fetch */
size_t write_http_response (void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;                             /* calculate buffer size */
    struct http_payload *p = (struct http_payload *) userp;   /* cast pointer to fetch struct */

    /* expand buffer */
    p->payload = (char *) realloc(p->payload, p->size + realsize + 1);

    /* check buffer */
    if (p->payload == NULL) {
      /* this isn't good */
      fprintf(stderr, "ERROR: Failed to expand buffer in curl_callback");
      /* free buffer */
      free(p->payload);
      /* return */
      return -1;
    }

    /* copy contents to buffer */
    memcpy(&(p->payload[p->size]), contents, realsize);

    /* set new buffer size */
    p->size += realsize;

    /* ensure null termination */
    p->payload[p->size] = 0;

    /* return size */
    return realsize;
}
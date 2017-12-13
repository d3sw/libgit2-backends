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

/* initialize the http payload */
void init_payload (struct http_payload *body);

/* fetch and return url body via curl */
char *request_http(const char *url, const char *action, struct http_payload *fetch) {

    /* init curl handle */
    CURL *handle = curl_easy_init();

    /* init payload */
    init_payload(fetch);

    /* set url to fetch */
    curl_easy_setopt(handle, CURLOPT_URL, url);

    /* set http action */
    curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, action);

    /* set calback function */
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_http_response);

    /* pass fetch struct pointer */
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *) fetch);

    /* set timeout */
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, 5);

    /* enable location redirects */
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);

    /* perform http request */
    int curl_result = curl_easy_perform(handle);

    /* cleanup curl handle */
    curl_easy_cleanup(handle);

    /* make the http request */
    if (curl_result != CURLE_OK || fetch->size < 1) {
        /* log error */
        fprintf(stderr, "ERROR: Failed to fetch url (%s) - curl said: %s",
            url, curl_easy_strerror(curl_result));
        /* return error */
        return "";
    }

    /* return */
    return fetch->payload;
}

int main(int argc, char *argv[]) {

    json_object *json;                                      /* json post body */
    enum json_tokener_error jerr = json_tokener_success;    /* json parse error */

    struct http_payload curl_fetch;                        /* curl fetch struct */
    struct http_payload *cf = &curl_fetch;                 /* pointer to fetch struct */
    struct curl_slist *headers = NULL;                      /* http headers to send with request */
    char *content = NULL;



    content = request_http("http://dimuthu.org", "GET", cf);
    printf("%s", content);

    /* set content type */
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");

    /* create json object for post */
    json = json_object_new_object();

    /* build post data */
    json_object_object_add(json, "title", json_object_new_string("testies"));
    json_object_object_add(json, "body", json_object_new_string("testies ... testies ... 1,2,3"));
    json_object_object_add(json, "userId", json_object_new_int(133));

    /* set curl options */
    //curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
    //curl_easy_setopt(handle, CURLOPT_POSTFIELDS, json_object_to_json_string(json));

    /* fetch page and capture return code */
    content = request_http("http://jsonplaceholder.typicode.com/posts/", "POST", cf);

    /* free headers */
    curl_slist_free_all(headers);

    /* free json object */
    json_object_put(json);

    /* check payload */
    if (cf->payload != NULL) {
        /* print result */
        printf("CURL Returned: \n%s\n", cf->payload);
        /* parse return */
        json = json_tokener_parse_verbose(cf->payload, &jerr);
        /* free payload */
        free(cf->payload);
    } else {
        /* error */
        fprintf(stderr, "ERROR: Failed to populate payload");
        /* free payload */
        free(cf->payload);
        /* return */
        return 3;
    }

    /* check error */
    if (jerr != json_tokener_success) {
        /* error */
        fprintf(stderr, "ERROR: Failed to parse json string");
        /* free json object */
        json_object_put(json);
        /* return */
        return 4;
    }

    /* debugging */
    printf("Parsed JSON: %s\n", json_object_to_json_string(json));

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
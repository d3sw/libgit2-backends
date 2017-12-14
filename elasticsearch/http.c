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
char *request_http(const char *url, const char *action, char **headers, int headers_count, struct http_payload *fetch) {

    /* init curl handle */
    CURL *handle = curl_easy_init();

    /* init payload */
    init_payload(fetch);

    /* set url to fetch */
    curl_easy_setopt(handle, CURLOPT_URL, url);

    /* set curl headers */
    init_headers(handle, headers, headers_count);

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
    char *content = NULL;

    char *get_headers[] = {};
    content = request_http("http://dimuthu.org", "GET", get_headers, 0, cf);
    printf("%s", content);

    /* create json object for post */
    json = json_object_new_object();

    /* build post data */
    json_object_object_add(json, "title", json_object_new_string("testies"));
    json_object_object_add(json, "body", json_object_new_string("testies ... testies ... 1,2,3"));
    json_object_object_add(json, "userId", json_object_new_int(133));

    /* fetch page and capture return code */
    char *post_headers[] = {"Accept: application/json","Content-Type: application/json"};
    content = request_http("http://jsonplaceholder.typicode.com/posts/", "POST", post_headers, 2, cf);

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
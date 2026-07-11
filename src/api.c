#include "api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

struct buf {
    char *data;
    size_t size;
};

static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userp)
{
    struct buf *b = userp;
    size_t total = size * nmemb;
    char *tmp = realloc(b->data, b->size + total + 1);
    if (!tmp)
        return 0;
    b->data = tmp;
    memcpy(b->data + b->size, ptr, total);
    b->size += total;
    b->data[b->size] = '\0';
    return total;
}

int api_http_get(const char *url, const char *apikey,
                 char **body_out, long *code_out)
{
    CURL *curl = curl_easy_init();
    if (!curl)
        return -1;

    struct buf b = { .data = malloc(1), .size = 0 };
    b.data[0] = '\0';

    struct curl_slist *headers = NULL;
    if (apikey && *apikey) {
        char auth[512];
        snprintf(auth, sizeof(auth), "Authorization: Bearer %s", apikey);
        headers = curl_slist_append(headers, auth);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &b);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

    if (code_out) *code_out = code;

    int ret = 0;
    if (res != CURLE_OK) {
        free(b.data);
        *body_out = NULL;
        ret = -1;
    } else {
        *body_out = b.data;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return ret;
}

static char *url_escape(const char *s)
{
    CURL *curl = curl_easy_init();
    char *esc = curl_easy_escape(curl, s, 0);
    char *out = strdup(esc);
    curl_free(esc);
    curl_easy_cleanup(curl);
    return out;
}

json_object *api_list_loras(const char *base_url, const char *apikey)
{
    char url[1024];
    snprintf(url, sizeof(url), "%s/object_info/LoraLoader", base_url);

    char *body = NULL;
    long code = 0;
    if (api_http_get(url, apikey, &body, &code) != 0 || !body) {
        fprintf(stderr, "error: failed to fetch lora list (HTTP %ld)\n", code);
        free(body);
        return NULL;
    }

    json_object *root = json_tokener_parse(body);
    free(body);
    if (!root) {
        fprintf(stderr, "error: invalid JSON from server\n");
        return NULL;
    }

    /* Response shape: {"LoraLoader": {"input": {"required": {"lora_name": [[...]]}}}} */
    json_object *loader = NULL;
    if (!json_object_object_get_ex(root, "LoraLoader", &loader)) {
        json_object_put(root);
        return NULL;
    }

    json_object *input = NULL;
    if (!json_object_object_get_ex(loader, "input", &input)) {
        json_object_put(root);
        return NULL;
    }

    json_object *required = NULL;
    if (!json_object_object_get_ex(input, "required", &required)) {
        json_object_put(root);
        return NULL;
    }

    json_object *lora_name = NULL;
    if (!json_object_object_get_ex(required, "lora_name", &lora_name)) {
        json_object_put(root);
        return NULL;
    }

    /* lora_name is [ [ "file1.safetensors", ... ] ] */
    if (json_object_array_length(lora_name) == 0) {
        json_object_put(root);
        return NULL;
    }

    json_object *arr = json_object_array_get_idx(lora_name, 0);
    if (!arr) {
        json_object_put(root);
        return NULL;
    }

    /* Return a ref to the inner array; bump refcount so caller can put root */
    json_object_get(arr);
    json_object_put(root);
    return arr;
}

json_object *api_get_metadata(const char *base_url, const char *apikey,
                              const char *filename)
{
    char *enc = url_escape(filename);
    char url[2048];
    snprintf(url, sizeof(url), "%s/view_metadata/loras?filename=%s", base_url, enc);
    free(enc);

    char *body = NULL;
    long code = 0;
    if (api_http_get(url, apikey, &body, &code) != 0 || !body) {
        free(body);
        return NULL;
    }

    if (code == 404) {
        free(body);
        return NULL;
    }

    json_object *meta = json_tokener_parse(body);
    free(body);
    return meta;
}

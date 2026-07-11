#ifndef GTWC_API_H
#define GTWC_API_H

#include <json-c/json.h>

/* Fetch lora names from /object_info/LoraLoader.
 * Returns a json_object array of strings, or NULL on error.
 * Caller must json_object_put() the result. */
json_object *api_list_loras(const char *base_url, const char *apikey);

/* Fetch metadata for a single lora via /view_metadata/loras?filename=...
 * Returns a json_object (the __metadata__ dict), or NULL on error.
 * Caller must json_object_put() the result. */
json_object *api_get_metadata(const char *base_url, const char *apikey,
                              const char *filename);

/* Raw HTTP GET. Returns malloc'd response body and http code.
 * Returns 0 on success, -1 on curl error. */
int api_http_get(const char *url, const char *apikey,
                 char **body_out, long *code_out);

#endif

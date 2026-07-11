#ifndef GTWC_TRIGGER_H
#define GTWC_TRIGGER_H

#include <json-c/json.h>

typedef struct {
    char *word;
    int count;
    int priority;
} trigger_word_t;

typedef struct {
    trigger_word_t *items;
    int count;
    int capacity;
} trigger_list_t;

/* Extract trigger words from a lora metadata object.
 * lora_name is the filename (e.g. "Bambi_il.safetensors") used as fallback.
 * Returns a trigger_list_t sorted by priority then frequency. */
trigger_list_t trigger_extract(json_object *metadata, const char *lora_name);

/* Free a trigger_list_t and all its strings. */
void trigger_list_free(trigger_list_t *list);

/* Print trigger words to stdout. */
void trigger_list_print(const trigger_list_t *list);

#endif

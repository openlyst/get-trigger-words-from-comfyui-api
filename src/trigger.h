#ifndef GTWC_TRIGGER_H
#define GTWC_TRIGGER_H

#include <json-c/json.h>

typedef struct {
    char *word;
    int count;
} trigger_word_t;

typedef struct {
    trigger_word_t *items;
    int count;
    int capacity;
} trigger_list_t;

/* Extract trigger words from a lora metadata object.
 * Looks at ss_tag_frequency and explicit trigger word fields.
 * Returns a trigger_list_t sorted by frequency (descending). */
trigger_list_t trigger_extract(json_object *metadata);

/* Free a trigger_list_t and all its strings. */
void trigger_list_free(trigger_list_t *list);

/* Print trigger words to stdout. */
void trigger_list_print(const trigger_list_t *list);

#endif

#include "trigger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void trigger_list_free(trigger_list_t *list)
{
    for (int i = 0; i < list->count; i++)
        free(list->items[i].word);
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void list_add(trigger_list_t *list, const char *word, int count)
{
    if (!word || !*word)
        return;

    /* Check if word already exists, update count if so */
    for (int i = 0; i < list->count; i++) {
        if (strcmp(list->items[i].word, word) == 0) {
            if (count > list->items[i].count)
                list->items[i].count = count;
            return;
        }
    }

    if (list->count >= list->capacity) {
        list->capacity = list->capacity ? list->capacity * 2 : 16;
        list->items = realloc(list->items, list->capacity * sizeof(trigger_word_t));
    }
    list->items[list->count].word = strdup(word);
    list->items[list->count].count = count;
    list->count++;
}

static int cmp_desc(const void *a, const void *b)
{
    const trigger_word_t *wa = a;
    const trigger_word_t *wb = b;
    return wb->count - wa->count;
}

/* Check common explicit trigger word metadata fields */
static void check_explicit_fields(json_object *meta, trigger_list_t *out)
{
    const char *fields[] = {
        "civitai_trigger_words",
        "trigger_word",
        "trigger_words",
        "ss_trigger_word",
        "modelspec.title",
        "ss_output_name",
        NULL
    };

    for (int i = 0; fields[i]; i++) {
        json_object *val = NULL;
        if (json_object_object_get_ex(meta, fields[i], &val) && val) {
            json_type t = json_object_get_type(val);
            if (t == json_type_string) {
                const char *s = json_object_get_string(val);
                if (s && *s)
                    list_add(out, s, 999999);
            } else if (t == json_type_array) {
                int n = json_object_array_length(val);
                for (int j = 0; j < n; j++) {
                    json_object *item = json_object_array_get_idx(val, j);
                    if (item && json_object_get_type(item) == json_type_string) {
                        const char *s = json_object_get_string(item);
                        if (s && *s)
                            list_add(out, s, 999999);
                    }
                }
            }
        }
    }
}

/* Extract candidate trigger words from dataset directory names.
 * e.g. "100_Bambi" -> "Bambi", "10_BoyKisser" -> "BoyKisser" */
static void extract_from_dataset_names(json_object *tag_freq, trigger_list_t *out)
{
    json_object_object_foreach(tag_freq, key, val) {
        (void)val;
        /* Skip the leading number prefix if present */
        const char *p = key;
        while (*p && (isdigit(*p) || *p == '_')) {
            if (*p == '_') { p++; break; }
            p++;
        }
        if (*p)
            list_add(out, p, 999998);
    }
}

/* Parse ss_tag_frequency JSON string and collect all tags with counts */
static void parse_tag_frequency(json_object *meta, trigger_list_t *out)
{
    json_object *tf_str = NULL;
    if (!json_object_object_get_ex(meta, "ss_tag_frequency", &tf_str))
        return;
    if (json_object_get_type(tf_str) != json_type_string)
        return;

    const char *raw = json_object_get_string(tf_str);
    json_object *tf = json_tokener_parse(raw);
    if (!tf)
        return;

    extract_from_dataset_names(tf, out);

    /* Iterate each dataset -> {tag: count} */
    json_object_object_foreach(tf, dataset, tags) {
        (void)dataset;
        if (json_object_get_type(tags) != json_type_object)
            continue;
        json_object_object_foreach(tags, tag, count_obj) {
            int count = 0;
            if (json_object_get_type(count_obj) == json_type_int)
                count = json_object_get_int(count_obj);
            else if (json_object_get_type(count_obj) == json_type_string)
                count = atoi(json_object_get_string(count_obj));
            list_add(out, tag, count);
        }
    }

    json_object_put(tf);
}

/* Heuristic: a trigger word typically appears in nearly every training image.
 * We compute the max count and include tags that are >= 50% of the max,
 * excluding generic tags that are just common booru tags. */
static void filter_and_sort(trigger_list_t *list)
{
    if (list->count == 0)
        return;

    qsort(list->items, list->count, sizeof(trigger_word_t), cmp_desc);

    /* Tags with count >= 999998 are from explicit fields or dataset names,
     * keep them all at the top. For the rest, keep those >= 50% of the
     * highest "real" tag count. */
    int max_real = 0;
    for (int i = 0; i < list->count; i++) {
        if (list->items[i].count < 999998 && list->items[i].count > max_real)
            max_real = list->items[i].count;
    }

    if (max_real == 0)
        return;

    int threshold = max_real / 2;
    int keep = 0;
    for (int i = 0; i < list->count; i++) {
        if (list->items[i].count >= 999998 || list->items[i].count >= threshold) {
            if (i != keep) {
                free(list->items[keep].word);
                list->items[keep].word = list->items[i].word;
                list->items[keep].count = list->items[i].count;
                list->items[i].word = NULL;
            }
            keep++;
        } else {
            free(list->items[i].word);
            list->items[i].word = NULL;
        }
    }
    list->count = keep;
}

trigger_list_t trigger_extract(json_object *metadata)
{
    trigger_list_t list = { 0 };

    if (!metadata)
        return list;

    /* Check explicit fields first (highest priority) */
    check_explicit_fields(metadata, &list);

    /* Parse tag frequency data */
    parse_tag_frequency(metadata, &list);

    /* Sort and filter */
    filter_and_sort(&list);

    return list;
}

void trigger_list_print(const trigger_list_t *list)
{
    if (list->count == 0) {
        printf("No trigger words found.\n");
        return;
    }

    for (int i = 0; i < list->count; i++) {
        if (list->items[i].count >= 999999)
            printf("  %s  (explicit)\n", list->items[i].word);
        else if (list->items[i].count >= 999998)
            printf("  %s  (from dataset name)\n", list->items[i].word);
        else
            printf("  %s  (count: %d)\n", list->items[i].word, list->items[i].count);
    }
}

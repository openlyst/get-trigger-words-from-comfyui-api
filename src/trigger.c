#include "trigger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>

void trigger_list_free(trigger_list_t *list)
{
    for (int i = 0; i < list->count; i++)
        free(list->items[i].word);
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void list_add(trigger_list_t *list, const char *word, int count, int priority)
{
    if (!word || !*word)
        return;

    for (int i = 0; i < list->count; i++) {
        if (strcasecmp(list->items[i].word, word) == 0) {
            if (count > list->items[i].count)
                list->items[i].count = count;
            if (priority > list->items[i].priority)
                list->items[i].priority = priority;
            return;
        }
    }

    if (list->count >= list->capacity) {
        list->capacity = list->capacity ? list->capacity * 2 : 16;
        list->items = realloc(list->items, list->capacity * sizeof(trigger_word_t));
    }
    list->items[list->count].word = strdup(word);
    list->items[list->count].count = count;
    list->items[list->count].priority = priority;
    list->count++;
}

/* Priority levels:
 * 4 = explicit trigger word field
 * 3 = from dataset name
 * 2 = high-frequency tag
 * 1 = from filename (fallback)
 */
#define PRI_EXPLICIT  4
#define PRI_DATASET   3
#define PRI_TAG       2
#define PRI_FILENAME  1

static int cmp_results(const void *a, const void *b)
{
    const trigger_word_t *wa = a;
    const trigger_word_t *wb = b;
    if (wa->priority != wb->priority)
        return wb->priority - wa->priority;
    return wb->count - wa->count;
}

/* Common booru tags that are not trigger words */
static const char *generic_tags[] = {
    "solo", "1girl", "2girls", "1boy", "2boys", "multiple girls", "multiple boys",
    "smile", "open mouth", "closed mouth", "looking at viewer", "simple background",
    "white background", "grey background", "black background", "transparent background",
    "full body", "upper body", "lower body", "close-up", "detailed face", "detailed eyes",
    "long hair", "short hair", "blonde hair", "black hair", "blue hair", "brown hair",
    "blue eyes", "green eyes", "red eyes", "brown eyes", "black eyes", "yellow eyes",
    "breasts", "large breasts", "small breasts", "medium breasts",
    "blush", "light blush", "nose blush",
    "bangs", "eyebrows", "eyelashes",
    "shirt", "white shirt", "black shirt", "jacket", "hoodie", "pants", "black pants",
    "shoes", "black footwear", "barefoot",
    "sitting", "standing", "lying", "kneeling",
    "outdoors", "indoors", "sky", "cloudy sky", "blue sky",
    "tree", "trees", "grass", "flower", "flowers",
    "day", "night", "sunset", "sunrise",
    "photo", "photorealistic", "realistic", "anime", "cartoon", "comic",
    "high quality", "best quality", "masterpiece", "detailed", "highres",
    "no humans", "animal focus", "animal", "mammal",
    "male focus", "female focus", "furry", "anthro",
    "tail", "ears", "animal ears", "cat ears", "dog ears", "cat tail", "dog tail",
    "paws", "claws", "fangs", "teeth",
    "collar", "bell", "bow", "necklace",
    "gloves", "socks", "stockings", "boots",
    "hat", "cap", "glasses", "sunglasses",
    "dress", "skirt", "shorts", "jeans",
    "coat", "scarf", "vest",
    "hand up", "hands up", "hand in pocket", "hand on own face", "hand on own cheek",
    "pocket", "drawstring",
    "off shoulder", "sleeveless", "sleeveless shirt", "long sleeves", "short sleeves",
    "layered sleeves",
    "jewelry", "earrings", "piercing", "ear piercing",
    "muscular", "muscular male", "bara", "pectorals", "large pectorals", "abs",
    "thighs", "thick thighs", "stomach", "navel",
    "nipples", "nude", "completely nude", "uncensored", "penis", "erection",
    "testicles", "anus",
    "blush", "heart", "sparkle", "crescent",
    "tongue", "tongue out", "fang",
    "2boys", "yaoi", "furry with furry", "furry male",
    "flat color", "monochrome", "colored sclera",
    "purple theme", "purple skin", "purple background", "red theme",
    "young", "female", "male", "dog", "cat", "wolf", "fox",
    "rectangular-shaped body", "two-tone fur", "blue fur", "red fur", "white fur",
    "black nose", "red blush",
    "focusing", "focusing on", "furry female",
    "solo focus",
    "animal feet", "pawpads",
    "vest", "bell", "jingle bell", "tail ornament",
    "crescent", "bow",
    "off shoulder",
    "drawstring",
    "hand on own face", "hand on own cheek",
    "muscular male", "bara", "pectorals", "large pectorals",
    "nipples", "uncensored", "nude", "completely nude",
    "penis", "erection", "testicles", "anus",
    "thick thighs", "thighs", "stomach", "navel",
    "purple skin", "purple background", "purple theme",
    "necklace", "sparkle", "sleeveless shirt", "sleeveless",
    "jacket", "animal feet", "colored sclera",
    "hand up", "flat color", "pocket",
    "hand in pocket", "fang",
    "furry with furry", "blush", "heart",
    "2boys", "dog ears", "multiple boys",
    "tongue out", "tongue", "yaoi", "dog tail",
    "1boy", "official style", "from behind", "from below", "from rear",
    "pigtails", "teeth", "deer", "fish", "robot", "animatronic",
    "canine", "feline", "marine", "lamia", "anglerfish", "3d",
    "semi-anthro", "quadruped", "anthro", "furry", "feral",
    "whiskers", "animal nose", "pink nose", "shiny nose",
    "glowing eyes", "black sclera", "white sclera", "red glowing eyes",
    "orange eyes", "orange fur", "black fur", "purple fur", "brown fur",
    "grey fur", "white fur", "blue fur", "green fur", "red fur",
    "multicolored fur", "striped fur", "light tan fur accents",
    "tufted fur", "fluffy chest", "fluffy ears",
    "short antlers", "small ears", "small tail", "fox tail",
    "3 spots on back", "dipstick tail", "hooves", "paws", "pawpads",
    "orange pawpads", "green paws", "green neck tuft",
    "pumpkin-shaped head", "black body",
    "red nose", "red and white striped antlers",
    "heavily damaged", "missing face", "jagged teeth",
    "expose wiring", "red bow tie", "two black chest buttons",
    "scuffed", "ripped", "misaligned rabbit ears", "hollow face",
    "metal right hand", "metal left foot", "amputated arm", "withering",
    "animatronic rabbit", "sharp teeth", "red body",
    "black thigh high socks", "robot joints",
    "anglerfish lure", "3 arm", "3 eye",
    "beanie", "cat girl", "pointed ears", "short",
    "tan muzzle", "reddish-brown fur", "white eyebrows", "white whiskers",
    "pink shirt", "hairbow", "collar", "bell",
    NULL
};

static int is_generic(const char *tag)
{
    for (int i = 0; generic_tags[i]; i++) {
        if (strcasecmp(tag, generic_tags[i]) == 0)
            return 1;
    }
    return 0;
}

/* Generic dataset folder names that don't contain trigger words */
static int is_generic_dataset(const char *name)
{
    const char *generic[] = {
        "img", "images", "image", "train", "training", "data", "dataset",
        "pics", "photos", "input", "lora", "my_lora", "custom",
        "nueva carpeta", "new folder", "untitled", "folder", NULL
    };
    for (int i = 0; generic[i]; i++) {
        if (strcasecmp(name, generic[i]) == 0)
            return 1;
    }
    return 0;
}

/* Check explicit trigger word metadata fields */
static void check_explicit_fields(json_object *meta, trigger_list_t *out)
{
    const char *fields[] = {
        "civitai_trigger_words",
        "trigger_word",
        "trigger_words",
        "ss_trigger_word",
        "civitai",
        NULL
    };

    for (int i = 0; fields[i]; i++) {
        json_object *val = NULL;
        if (json_object_object_get_ex(meta, fields[i], &val) && val) {
            json_type t = json_object_get_type(val);
            if (t == json_type_string) {
                const char *s = json_object_get_string(val);
                if (s && *s)
                    list_add(out, s, 0, PRI_EXPLICIT);
            } else if (t == json_type_array) {
                int n = json_object_array_length(val);
                for (int j = 0; j < n; j++) {
                    json_object *item = json_object_array_get_idx(val, j);
                    if (item && json_object_get_type(item) == json_type_string) {
                        const char *s = json_object_get_string(item);
                        if (s && *s)
                            list_add(out, s, 0, PRI_EXPLICIT);
                    }
                }
            } else if (t == json_type_object) {
                /* civitai field might be an object with trigger_words inside */
                json_object *tw = NULL;
                if (json_object_object_get_ex(val, "trigger_words", &tw) && tw) {
                    if (json_object_get_type(tw) == json_type_array) {
                        int n = json_object_array_length(tw);
                        for (int j = 0; j < n; j++) {
                            json_object *item = json_object_array_get_idx(tw, j);
                            if (item && json_object_get_type(item) == json_type_string) {
                                const char *s = json_object_get_string(item);
                                if (s && *s)
                                    list_add(out, s, 0, PRI_EXPLICIT);
                            }
                        }
                    }
                }
            }
        }
    }
}

/* Extract trigger word from dataset directory name.
 * e.g. "100_Bambi" -> "Bambi", "10_BoyKisser" -> "BoyKisser",
 * "1_yoshi yoshisaur" -> "yoshi yoshisaur" */
static void extract_from_dataset_names(json_object *tag_freq, trigger_list_t *out)
{
    json_object_object_foreach(tag_freq, key, val) {
        (void)val;
        /* Skip leading number prefix: digits followed by underscore */
        const char *p = key;
        while (*p && isdigit(*p)) p++;
        if (*p == '_') p++;

        /* If nothing left after stripping prefix, skip this dataset name */
        if (!*p)
            continue;

        if (!is_generic_dataset(p))
            list_add(out, p, 0, PRI_DATASET);
    }
}

/* Check if tags look like full captions (long strings with spaces) */
static int is_caption_style(json_object *tags)
{
    int long_count = 0;
    int total = 0;
    json_object_object_foreach(tags, tag, val) {
        (void)val;
        total++;
        if (strlen(tag) > 30) long_count++;
    }
    return total > 0 && long_count > total / 2;
}

/* Extract common leading token from caption-style tags */
static void extract_caption_trigger(json_object *tags, trigger_list_t *out)
{
    /* Find the most common first word across all captions */
    struct { char *word; int count; } words[64];
    int nwords = 0;

    json_object_object_foreach(tags, tag, val) {
        (void)val;
        /* Get first token */
        char tmp[256];
        strncpy(tmp, tag, sizeof(tmp) - 1);
        tmp[sizeof(tmp) - 1] = '\0';
        char *space = strchr(tmp, ' ');
        if (space) *space = '\0';
        if (!*tmp) continue;

        int found = -1;
        for (int i = 0; i < nwords; i++) {
            if (strcasecmp(words[i].word, tmp) == 0) {
                found = i;
                break;
            }
        }
        if (found >= 0) {
            words[found].count++;
        } else if (nwords < 64) {
            words[nwords].word = strdup(tmp);
            words[nwords].count = 1;
            nwords++;
        }
    }

    /* Find the word that appears in the most captions */
    int best = -1, best_count = 0;
    for (int i = 0; i < nwords; i++) {
        if (words[i].count > best_count) {
            best = i;
            best_count = words[i].count;
        }
    }

    if (best >= 0 && best_count > 1) {
        list_add(out, words[best].word, best_count, PRI_TAG);
    }

    for (int i = 0; i < nwords; i++)
        free(words[i].word);
}

/* Parse ss_dataset_dirs to get image count per dataset.
 * Returns a json_object ref or NULL. */
static json_object *parse_dataset_dirs(json_object *meta)
{
    json_object *dd_str = NULL;
    if (!json_object_object_get_ex(meta, "ss_dataset_dirs", &dd_str))
        return NULL;
    if (json_object_get_type(dd_str) != json_type_string)
        return NULL;
    return json_tokener_parse(json_object_get_string(dd_str));
}

/* Get img_count for a given dataset name from ss_dataset_dirs */
static int get_img_count(json_object *dataset_dirs, const char *ds_name)
{
    if (!dataset_dirs)
        return 0;
    json_object *ds = NULL;
    if (!json_object_object_get_ex(dataset_dirs, ds_name, &ds))
        return 0;
    json_object *count = NULL;
    if (!json_object_object_get_ex(ds, "img_count", &count))
        return 0;
    return json_object_get_int(count);
}

/* Derive a trigger word from the lora filename.
 * e.g. "Rudie.safetensors" -> "Rudie"
 * "Bambi_il.safetensors" -> "Bambi_il"
 * "CatNap_Poppy_Playtime_illustrious.safetensors" -> "CatNap_Poppy_Playtime_illustrious" */
static void extract_from_filename(const char *lora_name, trigger_list_t *out)
{
    if (!lora_name)
        return;

    char buf[512];
    strncpy(buf, lora_name, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    /* Strip .safetensors extension */
    char *dot = strstr(buf, ".safetensors");
    if (dot) *dot = '\0';

    /* Strip trailing version patterns like -V2, -000010, _e90_s1260 */
    int len = strlen(buf);
    while (len > 0) {
        /* Strip -XXXXX (5+ digits) or _sXXXX or _eXX patterns from the end */
        char *last_under = strrchr(buf, '_');
        char *last_dash = strrchr(buf, '-');
        char *sep = (last_under > last_dash) ? last_under : last_dash;
        if (!sep || sep == buf)
            break;

        /* Check if the suffix after sep is a version-like pattern */
        int suffix_len = len - (sep - buf) - 1;
        if (suffix_len < 2)
            break;

        int is_version = 1;
        for (char *p = sep + 1; *p; p++) {
            if (!isdigit(*p) && !(*p >= 'A' && *p <= 'F') &&
                !(*p >= 'a' && *p <= 'f') && *p != 's' && *p != 'e' &&
                *p != 'V' && *p != 'v') {
                is_version = 0;
                break;
            }
        }

        if (is_version && suffix_len <= 8) {
            *sep = '\0';
            len = strlen(buf);
        } else {
            break;
        }
    }

    if (*buf)
        list_add(out, buf, 0, PRI_FILENAME);
}

/* Parse ss_tag_frequency JSON string and collect tags with counts */
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

    json_object *dataset_dirs = parse_dataset_dirs(meta);

    /* First pass: find max count for fallback threshold */
    int max_count = 0;
    json_object_object_foreach(tf, dataset, tags) {
        (void)dataset;
        if (json_object_get_type(tags) != json_type_object)
            continue;

        if (is_caption_style(tags)) {
            extract_caption_trigger(tags, out);
            continue;
        }

        json_object_object_foreach(tags, tag, count_obj) {
            (void)tag;
            int count = 0;
            if (json_object_get_type(count_obj) == json_type_int)
                count = json_object_get_int(count_obj);
            else if (json_object_get_type(count_obj) == json_type_string)
                count = atoi(json_object_get_string(count_obj));
            if (count > max_count)
                max_count = count;
        }
    }

    /* Second pass: add high-frequency non-generic tags.
     * Use img_count from ss_dataset_dirs as threshold when available,
     * otherwise fall back to 60% of max count. */
    {
    json_object_object_foreach(tf, ds, ts) {
        if (json_object_get_type(ts) != json_type_object)
            continue;
        if (is_caption_style(ts))
            continue;

        int img_count = get_img_count(dataset_dirs, ds);
        int threshold;
        if (img_count > 0) {
            /* Trigger words appear in most images. Use 70% to allow
             * for images where the trigger word was omitted. */
            threshold = img_count * 0.7;
        } else {
            threshold = max_count * 0.6;
        }

        json_object_object_foreach(ts, tag, count_obj) {
            int count = 0;
            if (json_object_get_type(count_obj) == json_type_int)
                count = json_object_get_int(count_obj);
            else if (json_object_get_type(count_obj) == json_type_string)
                count = atoi(json_object_get_string(count_obj));
            if (count >= threshold && !is_generic(tag))
                list_add(out, tag, count, PRI_TAG);
        }
    }
    }

    if (dataset_dirs)
        json_object_put(dataset_dirs);
    json_object_put(tf);
}

trigger_list_t trigger_extract(json_object *metadata, const char *lora_name)
{
    trigger_list_t list = { 0 };

    if (!metadata)
        return list;

    check_explicit_fields(metadata, &list);
    parse_tag_frequency(metadata, &list);

    /* If we still have nothing, try the filename */
    if (list.count == 0)
        extract_from_filename(lora_name, &list);

    qsort(list.items, list.count, sizeof(trigger_word_t), cmp_results);

    return list;
}

void trigger_list_print(const trigger_list_t *list)
{
    if (list->count == 0) {
        printf("  No trigger words found.\n");
        return;
    }

    for (int i = 0; i < list->count; i++) {
        const char *src;
        switch (list->items[i].priority) {
            case PRI_EXPLICIT: src = "explicit"; break;
            case PRI_DATASET:  src = "dataset name"; break;
            case PRI_TAG:      src = "tag frequency"; break;
            default:           src = "filename"; break;
        }
        if (list->items[i].count > 0)
            printf("  %s  (%s, count: %d)\n", list->items[i].word, src, list->items[i].count);
        else
            printf("  %s  (%s)\n", list->items[i].word, src);
    }
}

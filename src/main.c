#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "config.h"
#include "api.h"
#include "trigger.h"

static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s <command> [args]\n"
        "\n"
        "Commands:\n"
        "  list              List all available LoRAs\n"
        "  get <name>        Get trigger words for a LoRA (supports partial match)\n"
        "  set url <url>     Set the ComfyUI server URL\n"
        "  set apikey <key>  Set the API key (optional)\n"
        "  set apikey clear  Remove the stored API key\n"
        "\n"
        "Examples:\n"
        "  %s set url http://127.0.0.1:8188\n"
        "  %s list\n"
        "  %s get Bambi\n"
        "  %s get Bambi_il.safetensors\n",
        prog, prog, prog, prog, prog);
}

static int cmd_list(config_t *cfg)
{
    json_object *loras = api_list_loras(cfg->url, cfg->apikey);
    if (!loras) {
        fprintf(stderr, "error: could not fetch lora list from %s\n", cfg->url);
        return 1;
    }

    int n = json_object_array_length(loras);
    if (n == 0) {
        printf("No LoRAs found.\n");
        json_object_put(loras);
        return 0;
    }

    printf("Found %d LoRA(s):\n", n);
    for (int i = 0; i < n; i++) {
        json_object *item = json_object_array_get_idx(loras, i);
        const char *name = json_object_get_string(item);
        printf("  [%d] %s\n", i, name);
    }

    json_object_put(loras);
    return 0;
}

/* Case-insensitive substring match */
static int name_matches(const char *name, const char *query)
{
    /* Exact match */
    if (strcasecmp(name, query) == 0)
        return 2;

    /* Exact match with .safetensors appended */
    char buf[512];
    snprintf(buf, sizeof(buf), "%s.safetensors", query);
    if (strcasecmp(name, buf) == 0)
        return 2;

    /* Substring match */
    const char *p = name;
    while (*p) {
        if (strncasecmp(p, query, strlen(query)) == 0)
            return 1;
        p++;
    }
    return 0;
}

static int cmd_get(config_t *cfg, const char *query)
{
    json_object *loras = api_list_loras(cfg->url, cfg->apikey);
    if (!loras) {
        fprintf(stderr, "error: could not fetch lora list from %s\n", cfg->url);
        return 1;
    }

    int n = json_object_array_length(loras);
    const char *best_name = NULL;
    int best_score = 0;

    for (int i = 0; i < n; i++) {
        json_object *item = json_object_array_get_idx(loras, i);
        const char *name = json_object_get_string(item);
        int score = name_matches(name, query);
        if (score > best_score) {
            best_score = score;
            best_name = name;
        }
    }

    if (!best_name) {
        fprintf(stderr, "error: no LoRA matching '%s' found\n", query);
        json_object_put(loras);
        return 1;
    }

    json_object_put(loras);

    printf("LoRA: %s\n", best_name);

    json_object *meta = api_get_metadata(cfg->url, cfg->apikey, best_name);
    if (!meta) {
        fprintf(stderr, "error: no metadata found for %s\n", best_name);
        return 1;
    }

    printf("Trigger words:\n");
    trigger_list_t triggers = trigger_extract(meta);
    trigger_list_print(&triggers);
    trigger_list_free(&triggers);
    json_object_put(meta);
    return 0;
}

static int cmd_set_url(const char *url)
{
    /* Strip trailing slash */
    char clean[1024];
    strncpy(clean, url, sizeof(clean) - 1);
    clean[sizeof(clean) - 1] = '\0';
    int len = strlen(clean);
    while (len > 0 && clean[len - 1] == '/')
        clean[--len] = '\0';

    if (config_save(clean, NULL) != 0) {
        fprintf(stderr, "error: could not save config\n");
        return 1;
    }
    printf("URL set to: %s\n", clean);
    return 0;
}

static int cmd_set_apikey(const char *key)
{
    const char *save_key = NULL;
    if (strcmp(key, "clear") == 0 || strcmp(key, "none") == 0)
        save_key = "";
    else
        save_key = key;

    if (config_save(NULL, save_key) != 0) {
        fprintf(stderr, "error: could not save config\n");
        return 1;
    }
    if (save_key[0] == '\0')
        printf("API key cleared.\n");
    else
        printf("API key set.\n");
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "set") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s set <url|apikey> <value>\n", argv[0]);
            return 1;
        }
        if (strcmp(argv[2], "url") == 0)
            return cmd_set_url(argv[3]);
        else if (strcmp(argv[2], "apikey") == 0)
            return cmd_set_apikey(argv[3]);
        else {
            fprintf(stderr, "error: unknown set option '%s'\n", argv[2]);
            return 1;
        }
    }

    /* For list and get, we need to load config */
    config_t cfg;
    config_load(&cfg);

    int ret = 1;
    if (strcmp(cmd, "list") == 0) {
        ret = cmd_list(&cfg);
    } else if (strcmp(cmd, "get") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s get <lora name or id>\n", argv[0]);
        } else {
            /* If the arg is a number, treat it as an index from list */
            if (isdigit(argv[2][0])) {
                int idx = atoi(argv[2]);
                json_object *loras = api_list_loras(cfg.url, cfg.apikey);
                if (loras) {
                    int n = json_object_array_length(loras);
                    if (idx >= 0 && idx < n) {
                        json_object *item = json_object_array_get_idx(loras, idx);
                        const char *name = json_object_get_string(item);
                        ret = cmd_get(&cfg, name);
                    } else {
                        fprintf(stderr, "error: index %d out of range (0-%d)\n", idx, n - 1);
                    }
                    json_object_put(loras);
                }
            } else {
                ret = cmd_get(&cfg, argv[2]);
            }
        }
    } else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0) {
        usage(argv[0]);
        ret = 0;
    } else {
        fprintf(stderr, "error: unknown command '%s'\n", cmd);
        usage(argv[0]);
    }

    config_free(&cfg);
    return ret;
}

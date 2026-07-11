#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>

char *expand_home(const char *path)
{
    if (path[0] == '~') {
        const char *home = getenv("HOME");
        if (!home) {
            struct passwd *pw = getpwuid(getuid());
            home = pw ? pw->pw_dir : "/";
        }
        char *out = malloc(strlen(home) + strlen(path));
        strcpy(out, home);
        strcat(out, path + 1);
        return out;
    }
    return strdup(path);
}

static char *config_path(void)
{
    char *base = expand_home("~/" CONFIG_DIR "/" CONFIG_FILE);
    return base;
}

static void ensure_config_dir(void)
{
    char *dir = expand_home("~/" CONFIG_DIR);
    /* mkdir -p style */
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s", dir);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
    free(dir);
}

static char *trim(char *s)
{
    while (*s == ' ' || *s == '\t') s++;
    char *end = s + strlen(s) - 1;
    while (end > s && (*end == '\n' || *end == '\r' || *end == ' '))
        *end-- = '\0';
    return s;
}

void config_load(config_t *cfg)
{
    cfg->url = strdup(DEFAULT_URL);
    cfg->apikey = NULL;

    char *path = config_path();
    FILE *f = fopen(path, "r");
    if (!f) {
        free(path);
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = trim(line);
        char *val = trim(eq + 1);
        if (strcmp(key, "url") == 0) {
            free(cfg->url);
            cfg->url = strdup(val);
        } else if (strcmp(key, "apikey") == 0) {
            free(cfg->apikey);
            cfg->apikey = strdup(val);
        }
    }
    fclose(f);
    free(path);
}

void config_free(config_t *cfg)
{
    free(cfg->url);
    free(cfg->apikey);
    cfg->url = NULL;
    cfg->apikey = NULL;
}

int config_save(const char *url, const char *apikey)
{
    ensure_config_dir();

    /* Read existing values so we don't clobber the other field */
    config_t existing;
    config_load(&existing);

    const char *save_url = url ? url : existing.url;
    const char *save_key = apikey ? apikey : existing.apikey;

    char *path = config_path();
    FILE *f = fopen(path, "w");
    if (!f) {
        free(path);
        config_free(&existing);
        return -1;
    }

    fprintf(f, "url=%s\n", save_url);
    if (save_key)
        fprintf(f, "apikey=%s\n", save_key);

    fclose(f);
    free(path);
    config_free(&existing);
    return 0;
}

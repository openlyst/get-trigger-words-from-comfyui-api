#ifndef GTWC_CONFIG_H
#define GTWC_CONFIG_H

#define DEFAULT_URL "http://127.0.0.1:8188"
#define CONFIG_DIR  ".config/gtwc"
#define CONFIG_FILE "config"

typedef struct {
    char *url;
    char *apikey;
} config_t;

/* Load config from disk. Missing fields get defaults. */
void config_load(config_t *cfg);

/* Free internally allocated strings. */
void config_free(config_t *cfg);

/* Persist url and/or apikey to disk. Either can be NULL to skip. */
int config_save(const char *url, const char *apikey);

/* Expand ~ to the user's home dir. Returns malloc'd string. */
char *expand_home(const char *path);

#endif

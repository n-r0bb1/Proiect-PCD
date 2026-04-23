#ifndef CONFIG_H
#define CONFIG_H

/* Loaded server configuration */
typedef struct {
    int max_workers;
    int chunk_size;
    int threshold;
} ServerConfig;

/* Load config from file path.
   Returns 0 on success, -1 on error. */
int config_load(const char *path, ServerConfig *out);

#endif /* CONFIG_H */

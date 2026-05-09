/* threeds.c – libconfig-based ServerConfig loader */
#include "../include/config.h"

#include <stdio.h>
#include <libconfig.h>

#define DEFAULT_MAX_WORKERS 4
#define DEFAULT_CHUNK_SIZE  100
#define DEFAULT_THRESHOLD   25

/* Încarcă configurația serverului din fișier */
int config_load(const char *path, ServerConfig *out)
{
    /* Validare parametri de intrare */
    if (!path || !out) {
        return -1;
    }

    /* Inițializare structură cu zero (evită garbage values) */
    ServerConfig zero = {0, 0, 0};
    *out = zero;

    /* Inițializare context libconfig */
    config_t cfg;
    config_init(&cfg);

    /* Citire fișier de configurare */
    if (config_read_file(&cfg, path) == CONFIG_FALSE) {
        /* Afișare eroare de parsing din fișier */
        (void)fprintf(stderr, "[config] %s:%d - %s\n",
                      config_error_file(&cfg),
                      config_error_line(&cfg),
                      config_error_text(&cfg));

        config_destroy(&cfg);
        return -1;
    }

    int val = 0;

    /* --- max_workers --- */
    /* Citește server.max_workers sau folosește default */
    if (config_lookup_int(&cfg, "server.max_workers", &val) == CONFIG_TRUE
            && val > 0) {
        out->max_workers = val;
    } else {
        out->max_workers = DEFAULT_MAX_WORKERS;
    }

    /* --- chunk_size --- */
    /* Citește server.chunk_size sau fallback */
    if (config_lookup_int(&cfg, "server.chunk_size", &val) == CONFIG_TRUE
            && val > 0) {
        out->chunk_size = val;
    } else {
        out->chunk_size = DEFAULT_CHUNK_SIZE;
    }

    /* --- threshold --- */
    /* Citește server.threshold sau fallback */
    if (config_lookup_int(&cfg, "server.threshold", &val) == CONFIG_TRUE
            && val >= 0) {
        out->threshold = val;
    } else {
        out->threshold = DEFAULT_THRESHOLD;
    }

    /* Eliberare resurse libconfig */
    config_destroy(&cfg);

    return 0;
}
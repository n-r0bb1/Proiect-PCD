/* inetsample2.c – client main: getopt parsing + validation + client_run() */
#include "../include/sclient.h"

#include <stdio.h> //NOLINT -> librarie specifica eroare
#include <stdlib.h>
#include <string.h> //NOLINT -> librarie specifica eroare
#include <unistd.h> //NOLINT -> librarie specifica eroare
#include <limits.h>

#define USAGE_MSG \
    "Usage: ./client -v <video_path> -c <chunk_size> -t <threshold>\n"
#define PATH_BUF_MAX 512

/* Funcție pentru afișarea modului de utilizare */
static void print_usage(void)
{
    (void)fprintf(stderr, USAGE_MSG);
}

int main(int argc, char *argv[])
{
    char video_path[PATH_BUF_MAX];
    video_path[0] = '\0';

    int chunk_size = 0;   // dimensiunea fragmentelor procesate
    int threshold  = -1;  // prag pentru detecție

    int opt = 0;

    /* Parsare argumente din linia de comandă */
    /* getopt folosește stare globală -> NOLINT */
    while ((opt = getopt(argc, argv, "v:c:t:")) != -1) { /* NOLINT */

        switch (opt) {

        /* -v <video_path> */
        case 'v':
            /* Copiere sigură a stringului video_path */
            if (snprintf(video_path, sizeof(video_path),
                         "%s", optarg /* NOLINT(misc-include-cleaner) */) < 0) {
                (void)fprintf(stderr, "[client] video path too long\n");
                return 1;
            }
            break;

        /* -c <chunk_size> */
        case 'c': {
            char *endptr = NULL;

            /* Conversie string → număr */
            long val = strtol(optarg /* NOLINT */, &endptr, 10);

            /* Validare: trebuie să fie număr pozitiv */
            if (endptr == optarg /* NOLINT */ || *endptr != '\0'
                || val <= 0 || val > INT_MAX) {

                (void)fprintf(stderr,
                              "[client] -c must be a positive integer\n");
                print_usage();
                return 1;
            }

            chunk_size = (int)val;
            break;
        }

        /* -t <threshold> */
        case 't': {
            char *endptr = NULL;

            /* Conversie string → număr */
            long val = strtol(optarg /* NOLINT */, &endptr, 10);

            /* Validare: număr întreg >= 0 */
            if (endptr == optarg /* NOLINT */ || *endptr != '\0'
                || val < 0 || val > INT_MAX) {

                (void)fprintf(stderr,
                              "[client] -t must be a non-negative integer\n");
                print_usage();
                return 1;
            }

            threshold = (int)val;
            break;
        }

        /* Argument necunoscut */
        default:
            print_usage();
            return 1;
        }
    }

    /* Verificare că toate argumentele obligatorii au fost primite */
    if (video_path[0] == '\0' || chunk_size == 0 || threshold < 0) {
        (void)fprintf(stderr, "[client] all arguments are mandatory\n");
        print_usage();
        return 1;
    }

    /* Apel către logica de client TCP */
    long no_motion = 0L;

    if (client_run(CLIENT_SERVER_HOST, CLIENT_SERVER_PORT,
                   video_path, chunk_size, threshold,
                   &no_motion) != 0) {

        (void)fprintf(stderr, "[client] request failed\n");
        return 1;
    }

    /* Afișarea rezultatului primit de la server */
    (void)printf("RES|%ld\n", no_motion);

    return 0;
}
/* serverds.c – TCP server main: getopt, config, accept loop */
#include "../include/server.h"
#include "../include/config.h"

#include <stdio.h>  //NOLINT
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h> //NOLINT
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define USAGE_MSG  "Usage: ./server -f <config_file>\n"
#define ERRBUF_SIZE 64
#define CFG_PATH_MAX 512

/* Afișează modul de utilizare al serverului */
static void print_usage(void)
{
    (void)fprintf(stderr, USAGE_MSG);
}

/* Curăță procesele copil terminate (evită zombie processes) */
static void reap_children(void)
{
    int status = 0;

    /* Recoltează toate procesele copil terminate fără blocare */
    while (waitpid(-1, &status, WNOHANG) > 0) {
        /* intenționat gol – doar cleanup */
    }
}

/* Inițializează socket-ul serverului (bind + listen) */
static int setup_server_socket(int *out_fd)
{
    char errbuf[ERRBUF_SIZE];

    /* Creare socket TCP */
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
        (void)strerror_r(errno, errbuf, sizeof(errbuf));
        (void)fprintf(stderr, "[server] socket: %s\n", errbuf);
        return -1;
    }

    /* Setare SO_REUSEADDR pentru a evita "Address already in use" */
    int optval = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, //NOLINT
                   &optval, sizeof(optval)) == -1) {
        (void)strerror_r(errno, errbuf, sizeof(errbuf));
        (void)fprintf(stderr, "[server] setsockopt: %s\n", errbuf);
        (void)close(sfd);
        return -1;
    }

    /* Configurare adresă server */
    struct sockaddr_in addr;
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((uint16_t)SERVER_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Inițializare padding */
    (void)memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

    /* Bind socket la port */
    if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        (void)strerror_r(errno, errbuf, sizeof(errbuf));
        (void)fprintf(stderr, "[server] bind: %s\n", errbuf);
        (void)close(sfd);
        return -1;
    }

    /* Ascultă conexiuni */
    if (listen(sfd, SERVER_BACKLOG) == -1) {
        (void)strerror_r(errno, errbuf, sizeof(errbuf));
        (void)fprintf(stderr, "[server] listen: %s\n", errbuf);
        (void)close(sfd);
        return -1;
    }

    *out_fd = sfd;
    return 0;
}

/* Loop principal: acceptă conexiuni și creează procese copil */
static void accept_loop(int server_fd, int max_workers)
{
    char errbuf[ERRBUF_SIZE];
    char client_ip[INET_ADDRSTRLEN];

    for (;;) {

        /* Curățare procese zombie */
        reap_children();

        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        (void)memset(&client_addr, 0, sizeof(client_addr));

        /* Acceptare conexiune client */
        int cfd = accept(server_fd,
                         (struct sockaddr *)&client_addr,
                         &client_len);

        if (cfd == -1) {
            /* Dacă a fost întrerupt de semnal, reîncearcă */
            if (errno == EINTR) {
                continue;
            }

            (void)strerror_r(errno, errbuf, sizeof(errbuf));
            (void)fprintf(stderr, "[server] accept: %s\n", errbuf);
            continue;
        }

        /* Afișare IP client */
        if (inet_ntop(AF_INET, &client_addr.sin_addr,
                      client_ip, sizeof(client_ip)) != NULL) {
            (void)fprintf(stdout,
                          "[server] accepted connection from %s\n", client_ip);
        }

        /* Fork pentru a crea proces copil */
        pid_t pid = fork();//NOLINT

        if (pid == -1) //NOLINT
        {
            (void)strerror_r(errno, errbuf, sizeof(errbuf));
            (void)fprintf(stderr, "[server] fork: %s\n", errbuf);
            (void)close(cfd);
            continue;
        }

        /* === PROCES COPIL === */
        if (pid == 0) {

            /* Copilul nu mai are nevoie de server socket */
            if (close(server_fd) == -1) {
                _Exit(1);
            }

            /* Procesare client */
            server_handle_client(cfd, max_workers);

            /* Închidere conexiune client */
            if (close(cfd) == -1) {
                _Exit(1);
            }

            _Exit(0);
        }

        /* === PROCES PĂRINTE === */

        /* Părintele închide descriptorul clientului */
        if (close(cfd) == -1) {
            (void)strerror_r(errno, errbuf, sizeof(errbuf));
            (void)fprintf(stderr, "[server] close cfd: %s\n", errbuf);
        }
    }
}

/* Entry point server */
int main(int argc, char *argv[])
{
    char cfg_path[CFG_PATH_MAX];
    cfg_path[0] = '\0';

    int opt = 0;

    /* Parsare argumente linie comandă (-f config file) */
    while ((opt = getopt(argc, argv, "f:")) != -1) { //NOLINT
        switch (opt) {

        case 'f':
            /* Copiere path config */
            if (snprintf(cfg_path, sizeof(cfg_path), "%s", optarg /* NOLINT(misc-include-cleaner) */) < 0) {
                (void)fprintf(stderr, "[server] config path too long\n");
                return 1;
            }
            break;

        default:
            print_usage();
            return 1;
        }
    }

    /* Verificare existență argument config */
    if (cfg_path[0] == '\0') {
        print_usage();
        return 1;
    }

    /* Verificare acces la fișier config */
    if (access(cfg_path, R_OK) != 0) {
        (void)fprintf(stderr,
                      "[server] cannot read config file: %s\n", cfg_path);
        return 1;
    }

    /* Încărcare configurație server */
    ServerConfig cfg = {0, 0, 0};
    if (config_load(cfg_path, &cfg) != 0) {
        (void)fprintf(stderr,
                      "[server] failed to load config: %s\n", cfg_path);
        return 1;
    }

    /* Log configurare încărcată */
    (void)fprintf(stdout,
                  "[server] config: max_workers=%d chunk_size=%d threshold=%d\n",
                  cfg.max_workers, cfg.chunk_size, cfg.threshold);

    /* Inițializare socket server */
    int server_fd = -1;
    if (setup_server_socket(&server_fd) != 0) {
        return 1;
    }

    (void)fprintf(stdout, "[server] listening on port %d\n", SERVER_PORT);

    /* Start accept loop (server rulează permanent) */
    accept_loop(server_fd, cfg.max_workers);

    /* Închidere server (practic nu ajunge aici decât la exit forțat) */
    (void)close(server_fd);
    return 0;
}
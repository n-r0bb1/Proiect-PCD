//* inetclient.c – TCP client: connect, send REQ, receive RES */
#include "../include/sclient.h"
#include "../protocol/proto.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ERRBUF_SIZE 64

/* Tip alias pentru file descriptor */
typedef int SockFd;

/* Funcție care citește o linie de pe socket (până la '\n') */
static ssize_t read_line(SockFd sock_fd, char *buf, size_t bufsz)
{
    size_t  pos = 0;
    ssize_t nr  = 0;
    char    ch  = '\0';

    /* Citește byte cu byte până la newline sau până se umple bufferul */
    while (pos + 1 < bufsz) {
        nr = read(sock_fd, &ch, 1);

        if (nr == 0) { // conexiune închisă de server
            break;
        }
        if (nr < 0) { // eroare la citire
            return -1;
        }

        buf[pos++] = ch;

        if (ch == '\n') { // sfârșit de linie
            break;
        }
    }

    buf[pos] = '\0'; // terminator de string
    return (ssize_t)pos;
}

/* Aliasuri de tipuri pentru claritate semantică */
typedef int ChunkSize;
typedef int Threshold;

int client_run(const char *host, int port,
               const char *video_path, ChunkSize chunk_size,
               Threshold threshold, long *no_motion_out)
{
    char errbuf[ERRBUF_SIZE];

    /* Verificare parametri de intrare */
    if (!host || !video_path || !no_motion_out || port <= 0) {
        return -1;
    }

    /* Construire request (structura ce va fi trimisă la server) */
    ProtoRequest req = {"", 0, 0};

    /* Copiere sigură a path-ului video */
    if (snprintf(req.video_path, sizeof(req.video_path),
                 "%s", video_path) < 0) {
        return -1;
    }

    /* Setarea parametrilor pentru procesare */
    req.chunk_size = chunk_size;
    req.threshold  = threshold;

    /* Serializare request într-un buffer de transmis prin rețea */
    char req_buf[PROTO_BUF_SIZE];
    if (proto_serialize_request(&req, req_buf, sizeof(req_buf)) != 0) {
        (void)fprintf(stderr, "[client] serialize request failed\n");
        return -1;
    }

    /* Creare socket TCP */
    SockFd sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
        (void)strerror_r(errno, errbuf, sizeof(errbuf));
        (void)fprintf(stderr, "[client] socket: %s\n", errbuf);
        return -1;
    }

    /* Configurare adresă server */
    struct sockaddr_in srv_addr;
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port   = htons((uint16_t)port);

    /* Inițializare câmpuri nefolosite */
    (void)memset(srv_addr.sin_zero, 0, sizeof(srv_addr.sin_zero));

    /* Conversie IP din string în format binar */
    if (inet_pton(AF_INET, host, &srv_addr.sin_addr) != 1) {
        (void)fprintf(stderr,
                      "[client] invalid host address: %s\n", host);
        (void)close(sfd);
        return -1;
    }

    /* Conectare la server */
    if (connect(sfd, (struct sockaddr *)&srv_addr,
                sizeof(srv_addr)) == -1) {
        (void)strerror_r(errno, errbuf, sizeof(errbuf));
        (void)fprintf(stderr, "[client] connect: %s\n", errbuf);
        (void)close(sfd);
        return -1;
    }

    /* Trimitere request către server */
    size_t  req_len = strlen(req_buf);
    ssize_t sent    = write(sfd, req_buf, req_len);

    /* Verificare dacă s-au trimis toți bytes */
    if (sent != (ssize_t)req_len) {
        (void)strerror_r(errno, errbuf, sizeof(errbuf));
        (void)fprintf(stderr, "[client] write: %s\n", errbuf);
        (void)close(sfd);
        return -1;
    }

    /* Buffer pentru răspunsul serverului */
    char res_buf[PROTO_BUF_SIZE];

    /* Citire răspuns (o linie) */
    if (read_line(sfd, res_buf, sizeof(res_buf)) <= 0) {
        (void)fprintf(stderr, "[client] read response failed\n");
        (void)close(sfd);
        return -1;
    }

    /* Închidere conexiune socket */
    if (close(sfd) == -1) {
        (void)strerror_r(errno, errbuf, sizeof(errbuf));
        (void)fprintf(stderr, "[client] close: %s\n", errbuf);
    }

    /* Parsare răspuns primit de la server */
    ProtoResponse res = {0};
    if (proto_parse_response(res_buf, &res) != 0) {
        (void)fprintf(stderr,
                      "[client] malformed response: %s\n", res_buf);
        return -1;
    }

    /* Returnarea rezultatului către caller */
    *no_motion_out = res.no_motion_count;

    return 0;
}
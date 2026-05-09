/* server_logic.c – handler per client: primește request, procesează, trimite răspuns */
#include "../include/server.h"
#include "../protocol/proto.h"
#include "../core/video_io.h"
#include "../core/frame_splitter.h"
#include "../core/worker.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define ERRBUF_SIZE 64

/* Tip alias pentru file descriptor client (claritate + evitarea swap-urilor greșite) */
typedef int ClientFd;

/* Citește o linie terminată cu '\n' de pe socket */
static ssize_t read_line(ClientFd sock_fd, char *buf, size_t bufsz)
{
    size_t  pos = 0;
    ssize_t nr  = 0;
    char    ch  = '\0';

    /* Citește byte cu byte până la newline sau buffer plin */
    while (pos + 1 < bufsz) {
        nr = read(sock_fd, &ch, 1);

        if (nr == 0) { // conexiune închisă de client
            break;
        }
        if (nr < 0) { // eroare la citire
            return -1;
        }

        buf[pos++] = ch;

        if (ch == '\n') { // sfârșit de mesaj
            break;
        }
    }

    buf[pos] = '\0'; // terminare string
    return (ssize_t)pos;
}

/* Handler principal pentru un client conectat */
void server_handle_client(ClientFd client_fd, int max_workers)
{
    char errbuf[ERRBUF_SIZE];

    /* --- 1. Citire request de la client --- */
    char req_buf[SERVER_BUF_SIZE];

    /* Citește mesajul complet trimis de client */
    ssize_t nr = read_line(client_fd, req_buf, sizeof(req_buf));
    if (nr <= 0) {
        (void)fprintf(stderr, "[server] failed to read request\n");
        return;
    }

    /* Parsează request-ul în structură internă */
    ProtoRequest req = {"", 0, 0};
    if (proto_parse_request(req_buf, &req) != 0) {
        (void)fprintf(stderr, "[server] malformed request: %s\n", req_buf);
        return;
    }

    /* Log request primit */
    (void)fprintf(stdout,
                  "[server] REQ video=%s chunk=%d threshold=%d\n",
                  req.video_path, req.chunk_size, req.threshold);

    /* --- 2. Obținere număr total de frame-uri din video --- */
    long total_frames = video_get_frame_count(req.video_path);
    if (total_frames <= 0) {
        (void)fprintf(stderr, "[server] cannot get frame count for %s\n",
                      req.video_path);
        return;
    }

    /* --- 3. Împărțire video în bucăți (chunks) --- */
    FrameChunk chunks[SPLITTER_MAX_CHUNKS];
    size_t     n_chunks = 0;

    if (frame_splitter_split(total_frames, req.chunk_size,
                             chunks, &n_chunks) != 0) {
        (void)fprintf(stderr, "[server] frame split failed\n");
        return;
    }

    /* --- 4. Execuție workers (procesare paralelă / fork-based) --- */
    long no_motion = workers_run(req.video_path,
                                 chunks, n_chunks,
                                 req.threshold,
                                 max_workers);

    /* Verificare rezultat workers */
    if (no_motion < 0) {
        (void)fprintf(stderr, "[server] workers failed\n");
        return;
    }

    /* Log rezultat final */
    (void)fprintf(stdout, "[server] no_motion_frames=%ld\n", no_motion);

    /* --- 5. Construire și trimitere răspuns către client --- */

    /* Serializare rezultat în format protocol */
    char res_buf[SERVER_BUF_SIZE];
    ProtoResponse res = {no_motion};

    if (proto_serialize_response(&res, res_buf, sizeof(res_buf)) != 0) {
        (void)fprintf(stderr, "[server] serialize response failed\n");
        return;
    }

    /* Trimitere răspuns prin socket */
    size_t  to_send = strlen(res_buf);
    ssize_t sent    = write(client_fd, res_buf, to_send);

    /* Verificare trimitere completă */
    if (sent != (ssize_t)to_send) {
        (void)strerror_r(errno, errbuf, sizeof(errbuf));
        (void)fprintf(stderr, "[server] write response failed: %s\n", errbuf);
    }
}
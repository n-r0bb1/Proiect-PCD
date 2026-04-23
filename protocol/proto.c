/* proto.c – serializare / deserializare protocol */
#include "proto.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Transformă structura request într-un string pentru trimitere prin rețea */
int proto_serialize_request(const ProtoRequest *req, char *buf, size_t bufsz)
{
    /* Verificare parametri de intrare */
    if (!req || !buf || bufsz == 0) {
        return -1;
    }

    /* Construire mesaj tip text: REQ|video|chunk|threshold */
    int ret = snprintf(buf, bufsz, "REQ|%s|%d|%d\n",
                       req->video_path, req->chunk_size, req->threshold);

    /* Verificare dacă bufferul a fost suficient de mare */
    if (ret < 0 || (size_t)ret >= bufsz) {
        return -1;
    }

    return 0;
}

/* Parsează un request primit sub formă de string */
int proto_parse_request(const char *buf, ProtoRequest *out)
{
    /* Validare input */
    if (!buf || !out) {
        return -1;
    }

    /* Copiere buffer pentru a-l putea modifica (strtok modifică stringul) */
    char tmp[PROTO_BUF_SIZE];
    if (snprintf(tmp, sizeof(tmp), "%s", buf) < 0) {
        return -1;
    }

    /* Eliminare newline de la final dacă există */
    size_t len = strlen(tmp);
    if (len > 0 && tmp[len - 1] == '\n') {
        tmp[len - 1] = '\0';
    }

    /* Separare string după delimitatorul '|' */
    char *saveptr = NULL;
    char *tok = strtok_r(tmp, "|", &saveptr);

    /* Primul câmp trebuie să fie "REQ" */
    if (!tok || strcmp(tok, "REQ") != 0) {
        return -1;
    }

    /* Al doilea câmp: path video */
    tok = strtok_r(NULL, "|", &saveptr);
    if (!tok) {
        return -1;
    }

    /* Copiere sigură a path-ului în struct */
    if (snprintf(out->video_path, sizeof(out->video_path), "%s", tok) < 0) {
        return -1;
    }

    /* Al treilea câmp: chunk_size */
    tok = strtok_r(NULL, "|", &saveptr);
    if (!tok) {
        return -1;
    }

    char *endptr = NULL;
    long chunk = strtol(tok, &endptr, 10);

    /* Validare chunk_size (trebuie să fie > 0) */
    if (endptr == tok || *endptr != '\0' || chunk <= 0) {
        return -1;
    }

    out->chunk_size = (int)chunk;

    /* Al patrulea câmp: threshold */
    tok = strtok_r(NULL, "|", &saveptr);
    if (!tok) {
        return -1;
    }

    endptr = NULL;
    long thr = strtol(tok, &endptr, 10);

    /* Validare threshold (>= 0) */
    if (endptr == tok || *endptr != '\0' || thr < 0) {
        return -1;
    }

    out->threshold = (int)thr;

    return 0;
}

/* Serializare response către string pentru client */
int proto_serialize_response(const ProtoResponse *res, char *buf, size_t bufsz)
{
    /* Validare parametri */
    if (!res || !buf || bufsz == 0) {
        return -1;
    }

    /* Format: RES|<valoare>\n */
    int ret = snprintf(buf, bufsz, "RES|%ld\n", res->no_motion_count);

    /* Verificare overflow buffer */
    if (ret < 0 || (size_t)ret >= bufsz) {
        return -1;
    }

    return 0;
}

/* Parsează response primit de la server */
int proto_parse_response(const char *buf, ProtoResponse *out)
{
    /* Validare input */
    if (!buf || !out) {
        return -1;
    }

    /* Copiere buffer pentru modificare */
    char tmp[PROTO_BUF_SIZE];
    if (snprintf(tmp, sizeof(tmp), "%s", buf) < 0) {
        return -1;
    }

    /* Eliminare newline final */
    size_t len = strlen(tmp);
    if (len > 0 && tmp[len - 1] == '\n') {
        tmp[len - 1] = '\0';
    }

    /* Tokenizare mesaj */
    char *saveptr = NULL;
    char *tok = strtok_r(tmp, "|", &saveptr);

    /* Verificare tip mesaj */
    if (!tok || strcmp(tok, "RES") != 0) {
        return -1;
    }

    /* Citire valoare numerică */
    tok = strtok_r(NULL, "|", &saveptr);
    if (!tok) {
        return -1;
    }

    char *endptr = NULL;
    long count = strtol(tok, &endptr, 10);

    /* Validare valoare numerică */
    if (endptr == tok || *endptr != '\0' || count < 0) {
        return -1;
    }

    out->no_motion_count = count;

    return 0;
}
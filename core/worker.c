/* worker.c – gestionare procese worker în paralel (fork + pipe) */
#include "worker.h"
#include "frame_splitter.h"
#include "motion_detection.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define WORKER_SLOT_MAX 64
#define ERRBUF_SIZE     64

/* Tipuri distincte pentru file descriptor (evită swap accidental de parametri) */
typedef int PipeReadFd;
typedef int PipeWriteFd;

/* Structură care ține evidența unui worker activ */
typedef struct {
    pid_t      pid; //NOLINT
    PipeReadFd read_fd;
} WorkerSlot;

/* Colectează rezultatul unui worker terminat și eliberează slotul */
static void slot_collect(WorkerSlot *slots, size_t slot_idx,
                         size_t *active, long *total)
{
    char errbuf[ERRBUF_SIZE];
    long partial = 0L;

    /* Citește rezultatul din pipe */
    ssize_t nread = read(slots[slot_idx].read_fd,
                         &partial, sizeof(partial));

    /* Dacă citirea este validă, adaugă la total */
    if (nread == (ssize_t)sizeof(partial)) {
        *total += partial;
    }

    /* Închide descriptorul de citire */
    if (close(slots[slot_idx].read_fd) == -1) {
        (void)strerror_r(errno, errbuf, sizeof(errbuf));
        (void)fprintf(stderr, "[worker] close read_fd: %s\n", errbuf);
    }

    /* Compactare array (mută ultimul element în locul celui eliberat) */
    slots[slot_idx] = slots[*active - 1];
    --(*active);
}

/* Așteaptă terminarea unui proces copil și colectează rezultatul */
static void wait_one(WorkerSlot *slots, size_t *active, long *total)
{
    char errbuf[ERRBUF_SIZE];
    int  status = 0;

    /* Așteaptă orice copil */
    pid_t ret = waitpid(-1, &status, 0);

    if (ret <= 0) {
        (void)strerror_r(errno, errbuf, sizeof(errbuf));
        (void)fprintf(stderr, "[worker] waitpid: %s\n", errbuf);
        return;
    }

    /* Identifică slotul corespunzător PID-ului terminat */
    for (size_t si = 0; si < *active; ++si) {
        if (slots[si].pid == ret) {
            slot_collect(slots, si, active, total);
            return;
        }
    }
}

/* Așteaptă și colectează toate procesele rămase */
static void drain_all(WorkerSlot *slots, size_t *active, long *total)
{
    while (*active > 0) {
        wait_one(slots, active, total);
    }
}

/* Execuție worker în proces copil */
static void child_run(const char  *video_path,
                      long         start_frame,
                      long         end_frame,
                      int          threshold,
                      PipeWriteFd  write_fd)
{
    char errbuf[ERRBUF_SIZE];

    /* Rulează detecția de mișcare pe chunk-ul asignat */
    long result = motion_detect_chunk(video_path,
                                      start_frame,
                                      end_frame,
                                      threshold);

    /* Trimite rezultatul înapoi prin pipe */
    ssize_t written = write(write_fd, &result, sizeof(result));

    if (written != (ssize_t)sizeof(result)) {
        (void)fprintf(stderr, "[worker] write pipe failed\n");
    }

    /* Închidere pipe write */
    if (close(write_fd) == -1) {
        (void)strerror_r(errno, errbuf, sizeof(errbuf));
        (void)fprintf(stderr, "[worker] close write_fd: %s\n", errbuf);
    }

    _Exit(0);
}

/* Rulează toate chunk-urile în paralel folosind fork + limitare max_workers */
long workers_run(const char         *video_path,
                 const FrameChunk   *chunks,
                 size_t              n_chunks,
                 int                 threshold,
                 int                 max_workers)
{
    /* Validare input */
    if (!video_path || !chunks || n_chunks == 0 || max_workers <= 0) {
        return -1L;
    }

    /* Limitare număr maxim de workeri */
    if (max_workers > WORKER_SLOT_MAX) {
        max_workers = WORKER_SLOT_MAX;
    }

    char       errbuf[ERRBUF_SIZE];
    WorkerSlot slots[WORKER_SLOT_MAX];

    size_t active = 0;  // număr workeri activi
    long   total  = 0L; // rezultat agregat final

    /* Parcurgere toate chunk-urile */
    for (size_t ci = 0; ci < n_chunks; ++ci) {

        /* Dacă s-au atins workerii maximi, așteaptă unul să termine */
        while (active >= (size_t)max_workers) {
            wait_one(slots, &active, &total);
        }

        /* Creare pipe pentru comunicare părinte–copil */
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            (void)strerror_r(errno, errbuf, sizeof(errbuf));
            (void)fprintf(stderr, "[worker] pipe: %s\n", errbuf);
            drain_all(slots, &active, &total);
            return -1L;
        }

        /* Fork proces worker */
        pid_t pid = fork();

        if (pid == -1) {
            (void)strerror_r(errno, errbuf, sizeof(errbuf));
            (void)fprintf(stderr, "[worker] fork: %s\n", errbuf);
            (void)close(pipefd[0]);
            (void)close(pipefd[1]);
            drain_all(slots, &active, &total);
            return -1L;
        }

        /* === PROCES COPIL === */
        if (pid == 0) {

            /* Copilul nu citește din pipe */
            if (close(pipefd[0]) == -1) {
                _Exit(1);
            }

            /* Rulează procesarea chunk-ului */
            child_run(video_path,
                      chunks[ci].start_frame,
                      chunks[ci].end_frame,
                      threshold,
                      pipefd[1]);

            _Exit(1); /* fallback (nu ar trebui să ajungă aici) */
        }

        /* === PROCES PĂRINTE === */

        /* Părintele nu scrie în pipe */
        if (close(pipefd[1]) == -1) {
            (void)strerror_r(errno, errbuf, sizeof(errbuf));
            (void)fprintf(stderr, "[worker] close write end: %s\n", errbuf);
        }

        /* Înregistrare worker activ */
        slots[active].pid     = pid;
        slots[active].read_fd = pipefd[0];
        ++active;
    }

    /* Așteaptă terminarea tuturor workerilor */
    drain_all(slots, &active, &total);

    /* Returnează suma rezultatelor */
    return total;
}
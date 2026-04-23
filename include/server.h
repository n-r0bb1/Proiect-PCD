#ifndef SERVER_H
#define SERVER_H

#include <stddef.h>  /* size_t */

/* Server listen port */
#define SERVER_PORT      9090
#define SERVER_BACKLOG      8
#define SERVER_BUF_SIZE  1024

/* Maximum number of parallel worker processes */
#define SERVER_MAX_WORKERS 16

/* ClientFd typedef defined locally in server_logic.c;
   handle_client declared here with plain int for external use */
void server_handle_client(int client_fd, int max_workers);

#endif /* SERVER_H */

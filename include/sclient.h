#ifndef SCLIENT_H
#define SCLIENT_H

/* Default server address/port */
#define CLIENT_SERVER_HOST  "127.0.0.1"
#define CLIENT_SERVER_PORT  9090

/*
 * Connect to server, send request, receive response.
 * Returns 0 on success, -1 on error.
 * no_motion_out receives the no-motion frame count.
 */
int client_run(const char *host, int port,
               const char *video_path, int chunk_size,
               int threshold, long *no_motion_out);

#endif /* SCLIENT_H */

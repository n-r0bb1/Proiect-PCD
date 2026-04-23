#ifndef FRAME_SPLITTER_H
#define FRAME_SPLITTER_H

#include <stddef.h>

#define SPLITTER_MAX_CHUNKS 64

typedef struct {
    long start_frame;
    long end_frame;   /* exclusive */
} FrameChunk;

/*
 * Split [0, total_frames) into chunks of at most chunk_size frames.
 * Fills chunks[] and sets *n_chunks.
 * Returns 0 on success, -1 on bad parameters.
 */
int frame_splitter_split(long          total_frames,
                         int           chunk_size,
                         FrameChunk   *chunks,
                         size_t       *n_chunks);

#endif /* FRAME_SPLITTER_H */

/* frame_splitter.c – imparte videoul in n chunks */
#include "frame_splitter.h"

#include <stddef.h>

int frame_splitter_split(long          total_frames,
                         int           chunk_size,
                         FrameChunk   *chunks,
                         size_t       *n_chunks)
{
    if (total_frames <= 0 || chunk_size <= 0 || !chunks || !n_chunks) {
        return -1;
    }

    size_t count = 0;
    long   start = 0L;

    while (start < total_frames) {
        if (count >= SPLITTER_MAX_CHUNKS) {
            return -1;
        }
        long end = start + (long)chunk_size;
        if (end > total_frames) {
            end = total_frames;
        }
        chunks[count].start_frame = start;
        chunks[count].end_frame   = end;
        ++count;
        start = end;
    }

    *n_chunks = count;
    return 0;
}

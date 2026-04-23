#ifndef WORKER_H
#define WORKER_H

#include "frame_splitter.h"

/*
 * Spawn up to max_workers child processes, one per chunk.
 * Each worker writes its no-motion count (as a long) to the write end of a
 * dedicated pipe.  The parent collects all results via aggregator_collect().
 *
 * Returns total no-motion frame count on success, -1 on error.
 */
long workers_run(const char         *video_path,
                 const FrameChunk   *chunks,
                 size_t              n_chunks,
                 int                 threshold,
                 int                 max_workers);

#endif /* WORKER_H */

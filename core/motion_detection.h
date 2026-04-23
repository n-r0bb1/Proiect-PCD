#ifndef MOTION_DETECTION_H
#define MOTION_DETECTION_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Count frames WITHOUT motion in [start_frame, end_frame) of video_path.
 * A frame has motion when any pixel difference vs. previous frame exceeds
 * threshold (0-255).
 *
 * returneaza count >= 0 on success, -1 on error.
 */
long motion_detect_chunk(const char *video_path,
                         long start_frame,
                         long end_frame,
                         int  threshold);

#ifdef __cplusplus
}
#endif

#endif /* MOTION_DETECTION_H */

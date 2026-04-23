#ifndef VIDEO_IO_H
#define VIDEO_IO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Return total frame count for video at path, or -1 on error. */
long video_get_frame_count(const char *path);

/* Implementation in video_io_cv.cpp */
long video_get_frame_count_impl(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* VIDEO_IO_H */

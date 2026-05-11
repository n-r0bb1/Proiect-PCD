#ifndef VIDEO_IO_H
#define VIDEO_IO_H

/* Return total frame count for video at path, or -1 on error. */
long video_get_frame_count(const char *path);

/* Implementation in video_io_cv.c */
long video_get_frame_count_impl(const char *path);

#endif /* VIDEO_IO_H */
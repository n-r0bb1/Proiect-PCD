/* video_io.c – wrapper subțire pentru obținerea metadatelor video (OpenCV) */
#include "video_io.h"

/* Funcție wrapper care apelează implementarea OpenCV */
long video_get_frame_count(const char *path)
{
    /* Validare simplă a input-ului */
    if (!path) {
        return -1L;
    }

    /* Delegare către implementarea reală (OpenCV backend) */
    return video_get_frame_count_impl(path);
}
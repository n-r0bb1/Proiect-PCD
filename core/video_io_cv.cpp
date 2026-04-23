/* video_io_cv.cpp – OpenCV VideoCapture query pentru numărul de frame-uri */
#include "video_io.h"
#include <cstdio>
#include <opencv2/videoio.hpp>  //NOLINT

extern "C" {

/* Returnează numărul total de frame-uri dintr-un video */
long video_get_frame_count_impl(const char *path)
{
    /* Validare input */
    if (path == nullptr) {
        return -1L;
    }

    /* Deschidere fișier video cu OpenCV */
    cv::VideoCapture cap(path);

    /* Verificare dacă video-ul a fost deschis corect */
    if (!cap.isOpened()) {
        (void)fprintf(stderr, "[video_io] cannot open: %s\n", path);
        return -1L;
    }

    /* Citire metadata: număr total de frame-uri */
    double fc = cap.get(cv::CAP_PROP_FRAME_COUNT);

    /* Eliberare resurse */
    cap.release();

    /* Validare rezultat */
    if (fc < 0.0) {
        return -1L;
    }

    /* Conversie la long și returnare */
    return static_cast<long>(fc);
}

} /* extern "C" */
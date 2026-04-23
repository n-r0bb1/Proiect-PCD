/* motion_detection.cpp – OpenCV frame detectare mișcare în frame-uri */
#include "motion_detection.h"

#include <cstdio>
#include <opencv2/core.hpp> //NOLINT
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

extern "C" {

/* Funcție C-callable pentru detectarea numărului de frame-uri fără mișcare */
long motion_detect_chunk(const char *video_path,
                         long        start_frame,
                         long        end_frame,
                         int         threshold)
{
    /* Validare parametri de intrare */
    if (video_path == nullptr || start_frame < 0 || end_frame <= start_frame || threshold < 0) {
        return -1L;
    }

    /* Deschidere video cu OpenCV */
    cv::VideoCapture cap(video_path);
    if (!cap.isOpened()) {
        (void)fprintf(stderr, "[motion] cannot open video: %s\n", video_path);
        return -1L;
    }

    /* Mutare la frame-ul de start */
    if (!cap.set(cv::CAP_PROP_POS_FRAMES, static_cast<double>(start_frame))) {
        (void)fprintf(stderr, "[motion] seek failed to frame %ld\n", start_frame);
        cap.release();
        return -1L;
    }

    /* Structuri OpenCV pentru procesare frame-uri */
    cv::Mat prev_gray;
    cv::Mat curr_frame;
    cv::Mat curr_gray;
    cv::Mat diff;

    long no_motion_count = 0L;

    /* Parcurgere frame-uri din chunk */
    for (long idx = start_frame; idx < end_frame; ++idx) {

        /* Citire frame curent */
        if (!cap.read(curr_frame) || curr_frame.empty()) {
            /* Final video înainte de end_frame */
            break;
        }

        /* Conversie la grayscale pentru comparație simplificată */
        cv::cvtColor(curr_frame, curr_gray, cv::COLOR_BGR2GRAY);

        /* Dacă este primul frame din chunk */
        if (prev_gray.empty()) {
            /* Nu există frame anterior -> considerat baseline */
            ++no_motion_count;
            prev_gray = curr_gray.clone();
            continue;
        }

        /* Diferență absolută între frame-uri consecutive */
        cv::absdiff(curr_gray, prev_gray, diff);

        double max_val = 0.0; //NOLINT valoarea maximă a diferenței

        /* Detectează intensitatea maximă a schimbării */
        cv::minMaxLoc(diff, nullptr, &max_val);

        /* Dacă diferența este sub prag → considerat "fără mișcare" */
        if (max_val <= static_cast<double>(threshold)) {
            ++no_motion_count;
        }

        /* Actualizare frame anterior */
        prev_gray = curr_gray.clone();
    }

    /* Eliberare resurse video */
    cap.release();

    /* Returnează numărul de frame-uri fără mișcare */
    return no_motion_count;
}

} /* extern "C" */
//
// Created by chenhao on 12/30/20.
//

#ifndef AVSAMPLES_VIDEO_SPLIT_YUV_H
#define AVSAMPLES_VIDEO_SPLIT_YUV_H

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>


int split_yuv(const char *input_file, const char *output_dir);


#endif //AVSAMPLES_VIDEO_SPLIT_YUV_H

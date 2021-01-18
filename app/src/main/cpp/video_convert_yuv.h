//
// Created by chenhao on 12/30/20.
//

#ifndef AVSAMPLES_VIDEO_CONVERT_YUV_H
#define AVSAMPLES_VIDEO_CONVERT_YUV_H

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

int convert_to_yuv420(const char *input_file, const char *output_file);


#endif //AVSAMPLES_VIDEO_CONVERT_YUV_H

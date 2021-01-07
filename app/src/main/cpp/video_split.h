//
// Created by chenhao on 12/29/20.
//

#ifndef AVSAMPLES_VIDEO_SPLIT_H
#define AVSAMPLES_VIDEO_SPLIT_H

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>

#define TEMP_FILE_NAME "temp"

/**
 * 传入视频和输出目录，在输出文件夹下按GOP切成多个小mp4
 * @param input_file
 * @param output_dir
 * @return 切出文件的个数
 */
int split_video_by_gop(const char *input_file, const char *output_dir);

#endif //AVSAMPLES_VIDEO_SPLIT_H

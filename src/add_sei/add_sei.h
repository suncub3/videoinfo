//
// Created by sunchongyang on 2022/4/19.
//

#ifndef SEI_TEST_ADD_READ_SEI_H
#define SEI_TEST_ADD_READ_SEI_H

#include <iostream>
#include <stdio.h>
#include <map>
#include <time.h>

extern "C" {
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include "libavutil/intreadwrite.h"
}

#define UUID_SIZE 16
#define SECOND 1000000000

class AddSEI {
 public:
  AddSEI();
  ~AddSEI();

  bool AddSEIWork(const char* input_url, const char* output_url);

 private:
  bool AddSEIInternal(const char* input_url, const char* output_url);

  const AVBitStreamFilter *prefix_filter;
  const AVBitStreamFilter *rm_filter;
  const AVBitStreamFilter *add_filter;

  AVBSFContext *prefix_bsf_ = nullptr;
  AVBSFContext *add_bsf_ = nullptr;
  AVBSFContext *rm_bsf_ = nullptr;

  AVFrame* MakeVideoFrame(int w, int h);
  AVCodecContext* InitVideoEncoder(int w, int h, int fps, int bps, const char* preset, int gop);
  AVPacket* GetPacket(int w, int h);
  int AllocateBsf(AVBSFContext **bsf,const AVBitStreamFilter *filter, const char * name, const char *content, AVStream *st);
  void FreeBsf(AVBSFContext **bsf);
  int FilterPacket(AVBSFContext *bsf, AVPacket *pkt);
  AVFormatContext* OpenInputFormat(const char *input_url);
  AVFormatContext *OpenOutPutFormat(const char *out_url);
  bool CopyStream(AVFormatContext *ictx, AVFormatContext *octx);
  int WriteFrame(AVFormatContext *ifmt_ctx, AVFormatContext *ofmt_ctx);
};

#endif //SEI_TEST_ADD_READ_SEI_H

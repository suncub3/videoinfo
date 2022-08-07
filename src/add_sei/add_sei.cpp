//
// Created by sunchongyang on 2022/4/19.
//

#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>

#include "add_sei.h"
#include "glog/logging.h"

static const char* iuuid = "73756e63756265206e6f20757569643a+";

AddSEI::AddSEI() :prefix_filter(nullptr), rm_filter(nullptr), add_filter(nullptr){
  prefix_filter = av_bsf_get_by_name("h264_mp4toannexb");
  rm_filter = av_bsf_get_by_name("filter_units");
  add_filter = av_bsf_get_by_name("h264_metadata");
}

AddSEI::~AddSEI() {

}

AVFrame* AddSEI::MakeVideoFrame(int w, int h) {
    AVFrame* f = av_frame_alloc();
    f->width = w;
    f->height = h;
    f->format = AV_PIX_FMT_YUV420P;  // AV_PIX_FMT_YUV420P / AV_PIX_FMT_RGBA
    av_frame_get_buffer(f, 32);
    return f;
}

AVCodecContext* AddSEI::InitVideoEncoder(int w, int h, int fps, int bps, const char* preset, int gop) {
    AVCodecContext* ctx;
    AVCodec *codec = avcodec_find_encoder_by_name("libx264");
    if (codec == nullptr) {
        LOG(ERROR) << "avcodec_find_encoder_by_name failed";
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (codec == nullptr) {
            LOG(ERROR) << "avcodec_find_encoder(AV_CODEC_ID_H264) failed";
            return nullptr;
        }
    }

    ctx = avcodec_alloc_context3(codec);
    ctx->profile = FF_PROFILE_H264_MAIN;
    ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    ctx->width = w;
    ctx->height = h;
    if (gop >= 25 && gop <= 250) {
        ctx->gop_size = gop;
    } else {
        ctx->gop_size = 10000;
        ctx->keyint_min = 10000;
    }


    ctx->framerate = (AVRational) {fps, 1};
    ctx->time_base = (AVRational) {1, fps};
    ctx->max_b_frames = 0;
    ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    av_opt_set(ctx->priv_data, "preset", preset, 0);
    av_opt_set(ctx->priv_data, "tune", "zerolatency", 0);
    av_opt_set(ctx->priv_data, "threads", "auto", 0);

    // 设置码率限制
    if (bps != 0) {
        ctx->bit_rate = bps;
        ctx->rc_max_rate = bps;
        ctx->rc_buffer_size = bps;
    }
    if (avcodec_open2(ctx, codec, nullptr) < 0) {
        LOG(ERROR) << "avcodec_open2 failed";
        return nullptr;
    }
    return ctx;
}

AVPacket* AddSEI::GetPacket(int w, int h) {
    static int64_t pts= 0;
    AVFrame* f = MakeVideoFrame(w, h);
    f->pts = pts;
    pts += 40000000;

    int bps = w * h * 12 * 8;
    AVCodecContext* encode_ctx_ = InitVideoEncoder(w, h, 25, bps, "superfast", 0);
    int ret = 0;
    // 获得了解码后的数据，然后进行重新编码
    if ((ret = avcodec_send_frame(encode_ctx_, f)) < 0) {
        return nullptr;
    }

    while (true) {
        AVPacket *pkt = av_packet_alloc();
        if ((ret = avcodec_receive_packet(encode_ctx_, pkt)) < 0) {
            break;
        }
        return pkt;
    }
    return nullptr;
}

int AddSEI::AllocateBsf(AVBSFContext **bsf, const AVBitStreamFilter *filter_, const char * name, const char *content, AVStream *st) {

  int ret = 0;
  if (!filter_) {
    DLOG(INFO) << "get "<< name <<" filter filed";
    return AVERROR_BSF_NOT_FOUND;
  }
  ret = av_bsf_alloc(filter_, bsf);
  if (ret < 0) {
    return ret;
  }
  if(strcmp(name, "filter_units") == 0){
    av_opt_set((*bsf)->priv_data, "remove_types", "6", AV_OPT_SEARCH_CHILDREN);
  }
  if(strcmp(name, "h264_metadata") == 0){
    av_opt_set((*bsf)->priv_data, "sei_user_data", content, AV_OPT_SEARCH_CHILDREN);
  }
  ret = avcodec_parameters_copy((*bsf)->par_in, st->codecpar);
  if (ret < 0) {
    return ret;
  }
  ret = av_bsf_init(*bsf);
  if (ret < 0) {
    return ret;
  }
  ret = avcodec_parameters_copy(st->codecpar, (*bsf)->par_out);
  if (ret < 0) {
    return ret;
  }
//  DLOG(INFO) << "Allocate bsf successful";
  return 0;
}

void AddSEI::FreeBsf(AVBSFContext **bsf){
  av_bsf_free(bsf);
}

int AddSEI::FilterPacket(AVBSFContext *bsf, AVPacket *pkt) {
  int ret = 0;

  if (bsf) {
    ret = av_bsf_send_packet(bsf, pkt);
    if (ret < 0) {
      LOG(ERROR) << "filter failed to send input packet";
      av_packet_unref(pkt);
      return ret;
    }

    while (!ret) {
      ret = av_bsf_receive_packet(bsf, pkt);
    }

    if (ret < 0 && (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)) {
      LOG(ERROR) << "filter failed to receive output packet";
      return ret;
    }
  }
  return 0;
}

AVFormatContext* AddSEI::OpenInputFormat(const char *input_url) {

  AVFormatContext *ifmt_ctx = nullptr;
  if ((avformat_open_input(&ifmt_ctx, input_url, 0, 0)) < 0) {
    DLOG(ERROR) <<"avformat_open_input failed:" << input_url;
    return nullptr;
  }
  // 1.2 解码一段数据，获取流相关信息
  if ((avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
    DLOG(ERROR) <<"Failed to retrieve input stream information";
    return nullptr;
  }
  //av_dump_format(ifmt_ctx, 0, input_url, 0);

  return ifmt_ctx;
}

AVFormatContext* AddSEI::OpenOutPutFormat(const char *out_url) {
  AVFormatContext *ofmt_ctx = nullptr;
  int ret = 0;
  avformat_alloc_output_context2(&ofmt_ctx, nullptr, nullptr, out_url);
  if (!ofmt_ctx) {
    DLOG(ERROR) <<"Could not create output context\n";
    ret = AVERROR_UNKNOWN;
    return nullptr;
  }
  return ofmt_ctx;
}

bool AddSEI::CopyStream(AVFormatContext *ictx, AVFormatContext *octx){
  for (int i = 0; i < ictx->nb_streams; i++)
  {
    AVStream *in_stream = ictx->streams[i];
    AVCodecParameters *in_codecpar = in_stream->codecpar;

    //创建输出流
    AVStream *avs = avformat_new_stream(octx, nullptr);
    if (!avs) {
      DLOG(ERROR) <<"Failed allocating output stream\n";
      return false;
    }
    if (avcodec_parameters_copy(avs->codecpar, in_codecpar) < 0) {
      DLOG(ERROR) <<"Failed to copy codec parameters\n";
      return false;
    }
    avs->codecpar->codec_tag = 0;
    if (i == 0) {
      avs->codecpar->codec_tag = MKTAG('a', 'v', 'c', '1');
    }
  }
  return true;
}

int AddSEI::WriteFrame(AVFormatContext *ifmt_ctx, AVFormatContext *ofmt_ctx) {
  int ret = 0;
  int video_count = 0;

  const char *rm ="filter_units";
  const char *prefix = "h264_mp4toannexb";
  const char *add = "h264_metadata";

  ret = AllocateBsf(&prefix_bsf_, prefix_filter, prefix, nullptr,ifmt_ctx->streams[0]);
  if (ret < 0){
    DLOG(ERROR)<<"allocate h264_mp4toannexb bsf failed";
    return ret;
  }
  ret = AllocateBsf(&rm_bsf_, rm_filter, rm, nullptr, ifmt_ctx->streams[0]);
  if (ret < 0){
    DLOG(ERROR)<<"allocate filter_units bsf failed";
    return ret;
  }

  AVPacket* pkt = av_packet_alloc();
//  AVPacket* v_pkt = GetPacket(1920, 1080);
//  if (!v_pkt) {
//    DLOG(ERROR) << "get packet failed";
//    return -1;
//  }
  while (1) {
    AVStream *in_stream, *out_stream;
    //  从输出流读取一个packet
    ret = av_read_frame(ifmt_ctx, pkt);
    if (ret < 0) {
      break;
    }
    in_stream = ifmt_ctx->streams[pkt->stream_index];
    out_stream = ofmt_ctx->streams[pkt->stream_index];

    if (in_stream->codecpar->codec_type==AVMEDIA_TYPE_VIDEO) {
//        continue;
        video_count++;
        ret = FilterPacket(prefix_bsf_, pkt);
        if(ret < 0){
            av_packet_free(&pkt);
            return ret;
        }
        ret = FilterPacket(rm_bsf_, pkt);
        if(ret < 0){
            av_packet_free(&pkt);
            return ret;
        }
        if(pkt->pts % 12800 < 512) {
            int64_t time = pkt->pts / 512 * 40;
            std::string content="pts: " + std::to_string(time);
            std::string temp = std::string(iuuid) + content;
//            DLOG(INFO)<<video_count<<"  content: "<<temp;
            const char* allcontent = temp.c_str();
            ret = AllocateBsf(&add_bsf_, add_filter, add, allcontent, in_stream);
            if(ret < 0){
                DLOG(ERROR)<<"allocate filter_units bsf failed";
                return -1;
            }
            ret = FilterPacket(add_bsf_, pkt);
            if(ret < 0){
                av_packet_free(&pkt);
                return -1;
            }
        }
    }

    av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base);
    pkt->pos = -1;
    //log_packet(ofmt_ctx, pkt);

    // 将packet写入输出
    if (av_interleaved_write_frame(ofmt_ctx, pkt) < 0) {
      DLOG(ERROR) <<"Error muxing packet";
      break;
    }
  }
  av_packet_free(&pkt);

  av_bsf_free(&prefix_bsf_);
  av_bsf_free(&add_bsf_);
  av_bsf_free(&rm_bsf_);
  //DLOG(INFO)<<"Write frame success";
  return 0;
}

bool AddSEI::AddSEIInternal(const char *input_url, const char *output_url) {
  LOG(INFO) << "input_url:" << input_url;
  LOG(INFO) << "output_url:" << output_url;
  LOG(INFO) << "start add sei...";

  //输入输出封装上下文
  AVFormatContext *ictx = OpenInputFormat(input_url);
  if (ictx == nullptr){
    DLOG(ERROR) << "create ictx failed";
    return false;
  }
  AVFormatContext *octx = OpenOutPutFormat(output_url);
  if (octx == nullptr){
    DLOG(ERROR) << "create octx failed";
    return false;
  }

  if (!CopyStream(ictx, octx)){
    DLOG(ERROR) << "copy stream info failed";
    return false;
  }

  // Everything is ready. Now open the output stream.
  if (avio_open(&octx->pb, output_url, AVIO_FLAG_WRITE) < 0) {
    DLOG(ERROR) << "FFMPEG: Could not open";
    return false;
  }
  DLOG(INFO) << "open outstream success";
  av_dump_format(octx, 0, output_url, 1);

  // Write the container header
  if (avformat_write_header(octx, nullptr) < 0) {
    DLOG(ERROR) << "FFMPEG: avformat_write_header error!";
    return false;
  }
  DLOG(INFO) << "write header success";

  //add content to packet
  if (WriteFrame(ictx,octx) < 0){
    DLOG(ERROR) << "write frame failed";
    return false;
  }

  DLOG(INFO) << "add sei info success";
  av_write_trailer(octx);
  avformat_close_input(&ictx);
  avio_close(octx->pb);
  avformat_free_context(octx);
  return true;
}

bool AddSEI::AddSEIWork(const char *input_url, const char *output_url) {
  return AddSEIInternal(input_url, output_url);
}

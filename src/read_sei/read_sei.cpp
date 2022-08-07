//
// Created by sunchongyang on 2021/7/8.
//

#include "read_sei.h"

static unsigned char uuid[] = { 0x73, 0x75, 0x6e, 0x63, 0x75, 0x62, 0x65, 0x20, 0x6e, 0x6f, 0x20, 0x75, 0x75, 0x69, 0x64, 0x3a };

read_sei::read_sei(){

}

read_sei::~read_sei() {

}

AVFormatContext* read_sei::OpenInputFormat(const char *input_url) {

    AVFormatContext *ifmt_ctx = nullptr;
    if ((avformat_open_input(&ifmt_ctx, input_url, 0, 0)) < 0) {
        DLOG(ERROR) <<"Could not open input file ";
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


bool read_sei::readsei(const char* input_url, std::map<int, std::string> *buffer){
    //输入封装上下文打开文件
    AVFormatContext *ictx = OpenInputFormat(input_url);
    if(ictx == nullptr){
        DLOG(ERROR) << "create ictx failed";
        return false;
    }
    //buffer中存放content
    ReadContent(ictx,buffer);
    avformat_close_input(&ictx);
    return true;
}

void read_sei::ReadContent(AVFormatContext *ictx, std::map<int,std::string> *buffer){
    AVPacket *pkt = av_packet_alloc();
    int video_count = 0;
    for(;;){
        av_packet_unref(pkt);
        if (av_read_frame(ictx, pkt) < 0) {
            break;
        }
        if(ictx->streams[pkt->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            video_count++;
            std::string content="";
            if(get_sei_content(pkt->data, pkt->size, &content) < 0){
//                std::cout <<frame_count<<" content is empty"<<std::endl;
            }
            else{
//                std::cout <<frame_count<<" content is "<<content<<std::endl;
                buffer->insert(std::pair<int, std::string>(video_count,content));
            }
        }
    }
    av_packet_free(&pkt);
}

uint32_t read_sei::reversebytes(uint32_t value) {
    return (value & 0x000000FFU) << 24 | (value & 0x0000FF00U) << 8 |
           (value & 0x00FF0000U) >> 8 | (value & 0xFF000000U) >> 24;
}

int read_sei::get_sei_content(uint8_t *packet, int size, std::string *content){
    if(packet == nullptr) return -1;
    unsigned char ANNEXB_CODE[] = { 0x00,0x00,0x00,0x01 };
    unsigned char ANNEXB_CODE_LOW[] = { 0x00,0x00,0x01 };
    unsigned char *data = packet;
    bool isAnnexb = false;

    if(memcmp(data, ANNEXB_CODE, 4) == 0 || memcmp(data, ANNEXB_CODE_LOW, 3) == 0){
        isAnnexb = true;
    }
    //start code prefixed
    if(isAnnexb){
        while (data < packet + 100 && data < packet + size){
            int start_code_len = 0;
            //start code 0x000001 or 0x00000001
            if(data[0] == 0x00 && data[1] == 0x00){
                if(data[2] == 0x00 && data[3] == 0x01) start_code_len = 4;
                if(data[2] == 0x01) start_code_len = 3;
                if(start_code_len == 3||start_code_len == 4){
                    //judge whether it is sei
                    if(data[start_code_len] == 0x06){
                        return get_sei_buffer(data+start_code_len+1,content);
                    }
                }
            }
            //find start code till data > size
            data++;
        }
        return -1;
    }
    //length prefixed
    else{
        while (data < packet + 100 && data < packet + size){
            //get length
            unsigned int *length = (unsigned int *)data;
            int nalu_size = (int)reversebytes(*length);
            //NALU header
            if ((*(data + 4) & 0x1F) == 6)
            {
                //SEI
                unsigned char * sei = data + 4 + 1;
                return get_sei_buffer(sei,content);
            }
            data += (4 + nalu_size);
        }
        return -1;
    }
}

int read_sei::get_sei_buffer(unsigned char* data, std::string *buffer){
    if (data == nullptr) return -1;
    unsigned char * sei = data;
    int sei_type = 0;
    unsigned sei_size = 0;
    // SEI payload type
    do {
        sei_type += *sei;
    } while (*sei++ == 255);
    // 数据长度
    do {
        sei_size += *sei;
    } while (*sei++ == 255);

    // 检查UUID
    // 此时的sei_size为UUID+未注册数据的长度
    if (sei_size >= UUID_SIZE && sei_type == 5 && memcmp(sei, uuid, UUID_SIZE) == 0)
    {
        sei += UUID_SIZE;  // 将地址移动到未注册数据首地址
        sei_size -= UUID_SIZE;  // 未注册数据长度

        if (buffer != nullptr)
        {
            *buffer = "";
            for(int i = 0; i < (int)sei_size - 1; i++){
                *buffer += *sei++;
            }
        }
        return (int)sei_size;
    }
    return -1;
}
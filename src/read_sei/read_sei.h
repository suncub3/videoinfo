//
// Created by sunchongyang on 2021/7/8.
//

#ifndef READSEI_READ_SEI_H
#define READSEI_READ_SEI_H

#include <iostream>
#include <stdio.h>
#include <map>
#include <time.h>
#include "glog/logging.h"
extern "C"
{
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include "libavutil/intreadwrite.h"
}

#define UUID_SIZE 16

class read_sei {
public:
    read_sei();
    ~read_sei();
    bool readsei(const char* input_url, std::map<int, std::string> *buffer);

private:
    uint32_t reversebytes(uint32_t value);
    AVFormatContext* OpenInputFormat(const char *input_url);
    int get_sei_buffer(unsigned char* data, std::string *buffer);
    int get_sei_content(uint8_t *packet, int size, std::string *content);
    void ReadContent(AVFormatContext *ictx, std::map<int,std::string> *buffer);

};


#endif //READSEI_READ_SEI_H

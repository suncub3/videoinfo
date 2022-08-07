#include <stdio.h>
#include "src/add_sei/add_sei.h"
#include "src/read_sei/read_sei.h"
using namespace std;
int main()
{
    AddSEI add;
    const char* input_url = "../res/fps25.mp4";
    const char* output_url = "../res/res.ts";

//   clock_t start = clock();
    for(int i = 0; i < 1; i++){//
        add.AddSEIWork(input_url, output_url);
    }
//    clock_t end = clock();
//    cout<<"total time:"<<end - start<<endl;

    read_sei read;
    std::map<int, std::string> buffer;
    read.readsei(output_url, &buffer);
    for (auto iter = buffer.begin(); iter != buffer.end(); iter++){
        std::cout<<iter->first<<": "<<iter->second<<std::endl;
    }

    return 0;
}
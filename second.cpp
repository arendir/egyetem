#include "pvm3.h"
#include <future>
#include <vector>
#include <cmath>
#include <tuple>
#include <algorithm>
#include <fstream>
#include <iostream>

static const int TERMINATE = 0;

using row_t = std::vector<std::tuple<int,int,int> >;
using image_t = std::vector<row_t>;

image_t encode_image(image_t &input_img);
row_t encode_row(row_t row);

int main() {
    pvm_recv(pvm_parent(),1);
    int prev_tid, next_tid;
    pvm_upkint(&prev_tid,1,1);
    pvm_upkint(&next_tid,1,1);
    bool terminate = false;

    while(!terminate) {
        int size;
        pvm_recv(prev_tid,2);
        pvm_upkint(&size,1,1);
        terminate = size == 0;
        if (!terminate) {
            image_t in_img;
            int img_size = size;
            in_img.reserve(img_size);
            for (int i = 0; i < img_size; ++i) {
                row_t row;
                row.reserve(img_size);
                for (int j = 0; j < img_size; ++j) {
                    int red;
                    int green;
                    int blue;
                    pvm_upkint(&red,1,1);
                    pvm_upkint(&green,1,1);
                    pvm_upkint(&blue,1,1);                
                    row.emplace_back(std::make_tuple(red,green,blue));
                }
                in_img.push_back(row);
            }
            image_t return_v = encode_image(in_img);
                        
            pvm_initsend(PvmDataDefault);
            int s = return_v.size();

            pvm_pkint(&s,1,1);
            
            for(auto row : return_v){
                for (auto item : row) 
                {
                    int red = std::get<0>(item);
                    int green=std::get<1>(item); 
                    int blue=std::get<2>(item);
                    pvm_pkint(&red,1,1);
                    pvm_pkint(&green,1,1);
                    pvm_pkint(&blue,1,1);
                }
            }
            pvm_send(next_tid,2);
        }
    }

    pvm_initsend(PvmDataDefault);

    int s = 0;
    pvm_pkint(&s,1,1);
    pvm_send(next_tid,2);

    
    pvm_exit();
    return 0;
}
image_t encode_image(image_t &input_img) {
    int size = input_img.size();
    std::vector<std::future<row_t>> result;
    result.reserve(size);
    image_t ret;
    ret.reserve(size);
    for (auto row : input_img) {
        result.push_back(std::async(std::launch::async, encode_row, row));
    }
    for(auto& res : result) {
        ret.push_back(res.get());
    }
    return ret;
}

row_t encode_row(row_t row) {
    row_t ret;
    ret.reserve(row.size());
    std::transform(row.begin(),row.end(),std::back_inserter(ret),
    [](std::tuple<int,int,int> item){
        int red = std::get<0>(item) > 127 ? 255 : 0;
        int green = std::get<1>(item) > 127 ? 255 : 0;
        int blue = std::get<2>(item) > 127 ? 255 : 0;
        return std::make_tuple(red,green,blue);
    });
    return ret;

}




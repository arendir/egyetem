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

std::vector<std::vector<int>>  encode_image(image_t &input_img);
std::vector<int>  encode_row(row_t row);
//std::vector<int>  encode_collum(row_t row);

int main() {
    pvm_recv(pvm_parent(),1);
    int prev_tid, next_tid = pvm_parent();
    pvm_upkint(&prev_tid,1,1);
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

            std::vector<std::vector<int>> return_v = encode_image(in_img);
            
            pvm_initsend(PvmDataDefault);
            int size_i = in_img.size();
            pvm_pkint(&size_i ,1,1);
            for(auto row : in_img){
                for (auto item : row) 
                {
                    int red = std::get<0>(item);
                    int green = std::get<1>(item);
                    int blue = std::get<2>(item);
                    pvm_pkint(&red,1,1);
                    pvm_pkint(&green,1,1);
                    pvm_pkint(&blue,1,1);
                }
            }
            for(auto row : return_v){
                int row_size = row.size();
                pvm_pkint(&row_size,1,1);
                for (auto item : row) 
                {
                    pvm_pkint(&item,1,1);
                }
            }
            //send through orig and tranformed
            pvm_send(next_tid, 3);
        }
    }
    pvm_initsend(PvmDataDefault);

    int s = 0;
    pvm_pkint(&s,1,1);
    pvm_send(next_tid,3);

    
    pvm_exit();
    return 0;
}
std::vector<std::vector<int>>  encode_image(image_t &input_img) {
    int size = input_img.size();
    std::vector<std::future<std::vector<int> >> result;
    result.reserve(size);
    std::vector<std::vector<int>> ret;
    ret.reserve(size*2);
    for (auto row : input_img) {
        result.push_back(std::async(std::launch::async, encode_row, row));
    }
    for(auto& res : result) {
        ret.push_back(res.get());
    }
    std::vector<std::future<std::vector<int> >> result2;
    result2.reserve(size);

    image_t collums;
    collums.reserve(size);
    for(int i = 0; i < size ;++i) {
        row_t collum;
        for(int j = 0; j < size ;++j) {
            collum.push_back(input_img[j][i]);
        }     
        collums.push_back(collum);  
    }
    for (auto collum : collums) {
        result2.push_back(std::async(std::launch::async, encode_row, collum));
    }
    for(auto& res : result2) {
        ret.push_back(res.get());
    }
    return ret;
}

std::vector<int>  encode_row(row_t row) {
    std::vector<int> ret;
    int c = 1;
       for(size_t i = 0; i < row.size()-1 ; ++i){
           if(row[i] == row[i+1]) ++c;
           else {
               ret.push_back(c);
               c = 1;
           }
       }
       ret.push_back(c);

    return ret;

}




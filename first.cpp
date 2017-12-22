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
image_t async_r_solve_shrinking(image_t image_to_shrinken, int level,int min_block_size, int width );
image_t seq_solve_shrinking(image_t &image_to_shrinken, int min_block_size, int width);


int main() {
    
    pvm_recv(pvm_parent(),1);
    //unpackage
    int percentage, next_tid;
    pvm_upkint(&next_tid,1,1);
    pvm_upkint(&percentage,1,1);

    bool terminate = false;
    while(!terminate) {
        pvm_recv(pvm_parent(),2);
        int img_size;
        pvm_upkint(&img_size,1,1);
        terminate = img_size == 0;
        if(img_size != 0) {
            image_t in_img;
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
            image_t res = async_r_solve_shrinking(in_img,1,100/percentage,img_size);
            
            pvm_initsend(PvmDataDefault);

            int s = res.size();

            pvm_pkint(&s,1,1);
            
            for(auto row : res){
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

image_t async_r_solve_shrinking(image_t image_to_shrinken, int level,int min_block_size, int width ) {

    if (level == 0 || width == min_block_size) {
      image_t res = seq_solve_shrinking(image_to_shrinken,min_block_size, width);
        return res;
    } else {
        image_t res;
        res.reserve(width/min_block_size);
        for(int i = 0; i < width/min_block_size; ++i) {
            row_t row;
            row.reserve(width/min_block_size);
            for(int j = 0 ; j < width/min_block_size; ++j) row.emplace_back(std::make_tuple(0,0,0));
            res.push_back(row);
        }   
        std::vector<std::future<image_t>> result;
        result.reserve(4);
        for(int i = 0 ; i < 2 ; ++i){
            for (int j = 0 ; j < 2; ++j) {
                image_t quarter;
                quarter.reserve(width/2);
                for (int k = 0; k < width/2; ++k) {
                    row_t row; 
                    row.reserve(width/2);
                    for (int l = 0; l < width/2; ++l) {
                        row.push_back(image_to_shrinken[i*(width/2)+k][j*(width/2)+l]);
                    }
                    quarter.push_back(row);
                }
                result.push_back(std::async(std::launch::async,async_r_solve_shrinking , quarter  ,level-1,min_block_size,width/2));
            }
        }

        for(int i = 0 ; i < 2 ; ++i){
            for (int j = 0 ; j < 2; ++j) {
                image_t q = result[i*2+j].get();
                for (size_t k =0 ; k < q.size(); ++k) {
                    for (size_t l =0 ; l < q.size(); ++l) {
                        res[i*(res.size()/2)+ k][j*(res.size()/2)+l] = q[k][l];
                    }   
                }
            }
        }
   

        return res;
    }
    //return new image_t();
}
image_t seq_solve_shrinking(image_t &image_to_shrinken, int min_block_size, int width) {
    int new_width = width / min_block_size;
    image_t new_pixels;
    int offset = min_block_size;
    new_pixels.reserve(new_width);
    for(int i = 0; i < new_width; ++i) {
        row_t row;
        row.reserve(new_width);
        for (int j = 0; j < new_width; ++j) {
            float red = 0, green = 0, blue = 0;
            for (int k = 0; k < offset; ++k) {
                for (int l = 0; l < offset; ++l) {
                    red += std::get<0>(image_to_shrinken[i*offset+k][j*offset+l]);
                    green += std::get<1>(image_to_shrinken[i*offset+k][j*offset+l]);
                    blue += std::get<2>(image_to_shrinken[i*offset+k][j*offset+l]);
                    
                }
            }
            red = std::trunc(red / (offset*offset));
            green = std::trunc(green / (offset*offset));
            blue = std::trunc(blue / (offset*offset));
            row.emplace_back(std::make_tuple(red,green,blue));
        }
        new_pixels.push_back(row);

    }
    return new_pixels;
}

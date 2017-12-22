#include <iostream>
#include <fstream>
#include <tuple>
#include <cstdlib>
#include <chrono>
#include "pvm3.h"
#include <fstream>
#include <vector>
static const int TERMINATE = 0;
using row_t = std::vector<std::tuple<int,int,int> > ;
using image_t = std::vector<row_t>;
enum errors {
    COULD_NOT_INIT_CHILD
};
void spawn_child(char* process_name, int* tid);
void spawn_children(int* tids);

std::vector<image_t > read_images_from_file(std::string path);
void read_img(std::ifstream &input, int size, image_t &img );


int main(int argc, char** argv) {
    if(argc != 4) {
        std::cerr << "The arguments are the following:\nshrinkage\tinput file location\toutput file location" << std::endl;
        pvm_exit();
        return -1; 
    }
    int percentage = atoi(argv[1]);
    auto img_container(read_images_from_file(argv[2]));
    int child[3];    
    try {       
        spawn_children(child);
    } catch (errors e) {
        if (e == COULD_NOT_INIT_CHILD) {
            std::cerr << "Couldn't make child processes!" << std::endl;
            pvm_exit();
            return -1;
        }
    }   
    std::ofstream file(argv[3]);
    pvm_initsend(PvmDataDefault);
    pvm_pkint(&(child[1]),1,1);
    pvm_send(child[2],1);

    pvm_initsend(PvmDataDefault);
    pvm_pkint(&(child[0]),1,1);
    pvm_pkint(&(child[2]),1,1);
    pvm_send(child[1],1);

    pvm_initsend(PvmDataDefault);
    pvm_pkint(&(child[1]),1,1);
    pvm_pkint(&percentage, 1,1);
    pvm_send(child[0],1);

    for (auto img : img_container) {
        pvm_initsend(PvmDataDefault);
        int size_of_img = img.size();
        pvm_pkint(&size_of_img,1,1);
        for(auto row: img) {
            for (auto item : row) {
                pvm_pkint(&std::get<0>(item),1,1);
                pvm_pkint(&std::get<1>(item),1,1);
                pvm_pkint(&std::get<2>(item),1,1);
            }
        }
        pvm_send(child[0],2);
        
    }
    pvm_initsend(PvmDataDefault);
    int term = TERMINATE;
    pvm_pkint(&term,1,1);
    pvm_send(child[0],2);
    bool terminate = false;

    while(!terminate) 
    {
       
        int size;
        pvm_recv(child[2],3);
        pvm_upkint(&size,1,1);
        terminate = size == 0;
        if (!terminate) {
            int img_size = size;
            for (int i = 0; i < img_size; ++i) {
                for (int j = 0; j < img_size; ++j) {
                    int red;
                    int green;
                    int blue;
                    pvm_upkint(&red,1,1);
                    pvm_upkint(&green,1,1);
                    pvm_upkint(&blue,1,1);  
                    file << "(" << red << "," << green << "," << blue << ") ";
               }
                file << std::endl;
            }
            for(int i = 0; i < img_size*2; ++i) {
                int row_size;
                pvm_upkint(&row_size,1,1);
                for (int j = 0; j < row_size; ++j) {
                    int item;
                    pvm_upkint(&item,1,1);
                    file << item << " ";
                }
                file << std::endl;
            }
        }
    }
    pvm_exit();
    return 0; 

   


}

void spawn_children(int* tids) {
    try {
        spawn_child("first",tids);
        spawn_child("second",tids+1);
        spawn_child("third",tids+2);
    } catch (errors e) {
        throw;
    }
    return;
}
void spawn_child(char* process_name, int* tid) {
    int child_succ = pvm_spawn(process_name, nullptr, 0, "", 1, tid);
    if (child_succ < 1 ) {
        throw COULD_NOT_INIT_CHILD;
    }
    return;
}

std::vector<image_t > read_images_from_file(std::string path) {

    std::ifstream input(path);
    int img_c;
    std::vector<image_t> image;
    input >> img_c;
    image.resize(img_c);
    for (int i = 0; i < img_c; ++i){
        int img_size;
        input >> img_size; 
        read_img(input,img_size, image[i]);            
    }
    return std::move(image);
}
void read_img(std::ifstream &input, int size, image_t &img ) {
    img.reserve(size);
    for (int i = 0; i < size; ++i) {
        row_t row;
        row.reserve(size);
        for (int j = 0; j < size; ++j) {
            int red, green, blue;
            input >> red >> green >> blue;
            row.emplace_back(std::make_tuple(red,green,blue));
        }
        img.push_back(row);
    }
}
#ifndef BLOCK_INFO_H
#define BLOCK_INFO_H

#include <iostream>
using namespace std;

struct block{
    char host_name[1024];
    char ip[16];
    long port_num;
};

#endif
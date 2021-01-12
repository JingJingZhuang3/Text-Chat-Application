/***
 * @jzhuang3_assignment1
 * @author  JingJing Zhuang <jzhuang3@buffalo.edu> 
***/
#ifndef INFO_H
#define INFO_H
#include <iostream>
#include <vector>
#include <string.h>

#include "block_info.h"
#include "msg_info.h"

using namespace std;

struct info
{
    vector<block> block_list;
    vector<message> msg_buffer;
    char host_name[1024];
    char ip[16];
    long port_num;
    string status; // login status
    int num_msg_sent = 0;
    int num_msg_rcv = 0;
    int sock_idx;
};

#endif
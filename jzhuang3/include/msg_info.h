#ifndef MSG_INFO_H
#define MSG_INFO_H

#include <iostream>
using namespace std;

struct message{
    char from_ip[16];
    char rcv_ip[16];
    char msg[BUFFER_SIZE+1];
};

#endif
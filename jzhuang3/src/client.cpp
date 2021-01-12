/***
 *@jzhuang3_assignment1
 * @author  JingJing Zhuang <jzhuang3@buffalo.edu>
 * 
 * References:
 * 	- https://beej.us/guide/bgnet/pdf/bgnet_usl_c_1.pdf 
 *  - server.c and client.c from https://cse.buffalo.edu/~lusu/cse4589/ 
 * 
***/
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fstream>
#include <algorithm>
#include <vector>

#include "../include/global.h"
#include "../include/logger.h"
#include "../include/client.h"
#include "../include/info.h"
#include "../include/block_info.h"
#include "../include/msg_info.h"

using namespace std;

bool client_vaildate_ip(char *ip)
{
    struct sockaddr_in addr;
    int vaild = inet_pton(AF_INET, ip, &(addr.sin_addr));
    return vaild != 0;
}

bool comp_port2(const info &lhs, const info &rhs)
{
    return lhs.port_num < rhs.port_num;
}

bool is_online(char *ip, vector<info> list)
{
    bool check = false;
    for (auto &c : list)
    {
        if (strcmp(ip, c.ip) == 0)
        {
            check = true;
            break;
        }
    }
    return check;
}

bool already_blocked(char *victim_ip, vector<string> block_list)
{
    bool check = false;
    for (auto &c : block_list)
    {
        if (c == victim_ip)
        {
            check = true;
            break;
        }
    }
    return check;
}

client::client(char *port)
{
    /* 
    * Get hostname and IP address using UDP socket, method reference from  
	* https://ubmnc.wordpress.com/2010/09/22/on-getting-the-ip-name-of-a-machine-for-chatty/
    * 
	* Create UDP socket:
    */
    // ofstream client_output;     //test grader output
    // client_output.open ("/home/csdue/jzhuang3/client_op.txt");
    int UDP_fd = 0; // init UDP socket
    struct sockaddr_in myaddr;
    if ((UDP_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Create UDP socket failed.\n");
        exit(1);
    }
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = inet_addr("8.8.8.8");
    myaddr.sin_port = htons(53);
    if (connect(UDP_fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
    {
        perror("connection failed.\n");
        close(UDP_fd);
        exit(1);
    }
    unsigned int addr_len = sizeof(myaddr);
    if (getsockname(UDP_fd, (struct sockaddr *)&myaddr, &addr_len) < 0)
    {
        perror("getsockname failed.\n");
        close(UDP_fd);
        exit(1);
    }
    char ip_addr[16]; //Get my local address
    inet_ntop(AF_INET, &myaddr.sin_addr, ip_addr, sizeof(ip_addr));
    //cout << ip_addr << endl;
    close(UDP_fd); //close UDP socket
    /* Get hostname */
    struct hostent *host;
    struct in_addr ipv4addr;
    inet_pton(AF_INET, ip_addr, &ipv4addr);
    host = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET);
    char *input_ptr;
    vector<string> my_block_list; //store my blocked
    while (true)
    {
        char *command_str = (char *)malloc(sizeof(char) * CMD_SIZE);
        memset(command_str, '\0', CMD_SIZE);
        if (fgets(command_str, CMD_SIZE - 1, stdin) == NULL) //Mind the newline character that will be written to cmd
            exit(-1);
        input_ptr = strtok(command_str, "\n");
        if (strcmp(input_ptr, "AUTHOR") == 0)
        {
            char ubitname[] = "jzhuang3";
            cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
            cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", ubitname);
            cse4589_print_and_log("[%s:END]\n", command_str);
        }
        else if (strcmp(input_ptr, "IP") == 0)
        {
            cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
            cse4589_print_and_log("IP:%s\n", ip_addr);
            cse4589_print_and_log("[%s:END]\n", command_str);
        }
        else if (strcmp(input_ptr, "PORT") == 0)
        {
            cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
            cse4589_print_and_log("PORT:%d\n", atoi(port));
            cse4589_print_and_log("[%s:END]\n", command_str);
        }
        else if (strcmp(input_ptr, "EXIT") == 0)
        {
            cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
            cse4589_print_and_log("[%s:END]\n", command_str);
            exit(0);
        }
        else if (strncmp(input_ptr, "LOGIN", 5) == 0)
        {
            // client_output << "Command before login:|" << input_ptr << "|\n";
            vector<info> list_buffer; //store logged in client info
            char *serv_ip, *serv_port;
            char *pt;
            //cout<<"CHECK INPUT: " <<input_ptr<<endl;
            strtok(input_ptr, " ");
            serv_ip = strtok(NULL, " ");
            serv_port = strtok(NULL, " ");
            if (serv_port != nullptr)
            {
                int server_port = atoi(serv_port);
                // cout << "ip: " << serv_ip << "\nport: " << server_port << endl;
                if ((server_port > 0 && server_port < 65535) && (client_vaildate_ip(serv_ip)))
                {
                    int fdsocket, len;
                    struct sockaddr_in remote_server_addr;

                    fdsocket = socket(AF_INET, SOCK_STREAM, 0);
                    if (fdsocket < 0)
                    {
                        perror("Failed to create socket");
                        continue;
                    }

                    bzero(&remote_server_addr, sizeof(remote_server_addr));
                    remote_server_addr.sin_family = AF_INET;
                    inet_pton(AF_INET, serv_ip, &remote_server_addr.sin_addr);
                    remote_server_addr.sin_port = htons(server_port);

                    if (connect(fdsocket, (struct sockaddr *)&remote_server_addr, sizeof(remote_server_addr)) < 0)
                    {
                        perror("Connect failed");
                        continue;
                    }
                    if (send(fdsocket, port, strlen(port), 0) < 0)
                    {
                        perror("Failed sending my listening port.");
                    }
                    char respond_list[8192];
                    bzero(&respond_list, sizeof(respond_list));
                    if (recv(fdsocket, respond_list, sizeof(respond_list), 0) < 0)
                    {
                        perror("Failed to receive Server responds list.");
                    }
                    char *list_info;
                    // printf("respond size: %d\n", strlen(respond_list));
                    // printf("RESPOND FROM SERVER:\n%s\n",respond_list);
                    list_info = strtok(respond_list, "*");
                    while (strcmp(list_info, "&") != 0)
                    {
                        info client_info;
                        list_info = strtok(NULL, " "); // host name
                        if (strcmp(list_info, "&") != 0)
                        {
                            strcpy(client_info.host_name, list_info); // store host name
                            // printf("%s\n", client_info.host_name);
                            list_info = strtok(NULL, " ");     // ip
                            strcpy(client_info.ip, list_info); // store ip
                            // printf("%s\n", client_info.ip);
                            list_info = strtok(NULL, " ");          // port#
                            client_info.port_num = atoi(list_info); // store port#
                            // printf("%d\n", client_info.port_num);
                            client_info.status = "logged-in";
                            list_buffer.push_back(client_info);
                        }
                        else
                        {
                            break;
                        }
                    }
                    // after loop, list_info = '&'
                    // printf("info list: \n%s", list_info);
                    list_info = strtok(NULL, "`"); // list_info = '-'
                    // printf("info list: %s\n", list_info);
                    /* Print received messages */
                    int i = 0;
                    while (list_info != nullptr)
                    {
                        list_info = strtok(NULL, "\n"); // sender_ip
                        // printf("info list: %s\n", list_info);
                        if ((list_info != nullptr) && client_vaildate_ip(list_info))
                        {
                            // if (i == 0)
                            // {
                            cse4589_print_and_log("[RECEIVED:SUCCESS]\n");
                            //     i++;
                            // }
                            cse4589_print_and_log("msg from:%s\n", list_info);
                            list_info = strtok(NULL, "\n"); //  msg
                            // printf("info list: %s\n", list_info);
                            cse4589_print_and_log("[msg]:%s\n", list_info);
                            cse4589_print_and_log("[RECEIVED:END]\n");
                        }
                        else
                        {
                            // if (i == 1)
                            // {
                            //     cse4589_print_and_log("[RECEIVED:END]\n");
                            // }
                            break;
                        }
                    }
                    cse4589_print_and_log("[%s:SUCCESS]\n", input_ptr);
                    cse4589_print_and_log("[%s:END]\n", input_ptr);
                    // fflush(stdout);
                    int head_socket, selret; //*
                    while (TRUE)
                    {
                        fflush(stdout);
                        fd_set watch_server;                                               //*watch target only server
                        FD_ZERO(&watch_server);                                            //*
                        FD_SET(fdsocket, &watch_server);                                   //*
                        FD_SET(STDIN, &watch_server);                                      //*
                        head_socket = fdsocket;                                            //*
                        selret = select(head_socket + 1, &watch_server, NULL, NULL, NULL); //*
                        if (selret < 0)
                        { //*
                            // perror("select failed.");
                            break;
                        }                                       //*
                        if (selret > 0)                         //*
                        {                                       //*
                            if (FD_ISSET(STDIN, &watch_server)) //*
                            {                                   //*
                                char *msg = (char *)malloc(sizeof(char) * 1024);
                                memset(msg, '\0', 1024);
                                if (fgets(msg, 1024 - 1, stdin) == NULL) //Mind the newline character that will be written to msg
                                    exit(-1);
                                char *ptr2 = strtok(msg, "\n");
                                if (strcmp(ptr2, "AUTHOR") == 0)
                                {
                                    // client_output << "Command After login:|" << ptr2 << "|\n";
                                    char ubitname[] = "jzhuang3";
                                    cse4589_print_and_log("[%s:SUCCESS]\n", ptr2);
                                    cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", ubitname);
                                    cse4589_print_and_log("[%s:END]\n", ptr2);
                                }
                                else if (strcmp(ptr2, "IP") == 0)
                                {
                                    // client_output << "Command After login:|" << ptr2 << "|\n";
                                    cse4589_print_and_log("[%s:SUCCESS]\n", ptr2);
                                    cse4589_print_and_log("IP:%s\n", ip_addr);
                                    cse4589_print_and_log("[%s:END]\n", ptr2);
                                }
                                else if (strcmp(ptr2, "PORT") == 0)
                                {
                                    // client_output << "Command After login:|" << ptr2 << "|\n";
                                    cse4589_print_and_log("[%s:SUCCESS]\n", ptr2);
                                    cse4589_print_and_log("PORT:%d\n", atoi(port));
                                    cse4589_print_and_log("[%s:END]\n", ptr2);
                                }
                                else if (strcmp(ptr2, "LIST") == 0)
                                {
                                    // client_output << "Command After login:|" << ptr2 << "|\n";
                                    cse4589_print_and_log("[%s:SUCCESS]\n", ptr2);
                                    int i = 0;
                                    // cout << "Size of list: " << list.size() << endl;
                                    sort(list_buffer.begin(), list_buffer.end(), comp_port2); // sort the port number
                                    for (auto &client : list_buffer)
                                    {
                                        //Print only logged-in client
                                        if (client.host_name != nullptr && client.status == "logged-in")
                                        {
                                            i += 1;
                                            cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", i, client.host_name, client.ip, client.port_num);
                                        }
                                    }
                                    cse4589_print_and_log("[%s:END]\n", ptr2);
                                }
                                else if (strcmp(ptr2, "REFRESH") == 0)
                                {
                                    // client_output << "Command After login:|" << ptr2 << "|\n";
                                    if (send(fdsocket, msg, strlen(msg), 0) < 0)
                                        perror("Failed sending my command");
                                    fflush(stdout);
                                    /* Initialize buffer to receieve response */
                                    char *buffer = (char *)malloc(sizeof(char) * 2048);
                                    memset(buffer, '\0', 2048);
                                    if (recv(fdsocket, buffer, 2048, 0) >= 0)
                                    {
                                        // printf("Server responded: %s\n", buffer);
                                        list_buffer.clear();
                                        char *c_info;
                                        c_info = strtok(buffer, "*");
                                        cse4589_print_and_log("[%s:SUCCESS]\n", ptr2);
                                        while (strcmp(c_info, "&") != 0)
                                        {
                                            info c_update;
                                            c_info = strtok(NULL, " "); // host name
                                            if (strcmp(c_info, "&") != 0)
                                            {
                                                strcpy(c_update.host_name, c_info); // store host name
                                                // printf("%s\n", client_info.host_name);
                                                c_info = strtok(NULL, " ");  // ip
                                                strcpy(c_update.ip, c_info); // store ip
                                                // printf("%s\n", client_info.ip);
                                                c_info = strtok(NULL, " ");       // port#
                                                c_update.port_num = atoi(c_info); // store port#
                                                // printf("%d\n", client_info.port_num);
                                                c_update.status = "logged-in";
                                                list_buffer.push_back(c_update);
                                            }
                                            else
                                            {
                                                break;
                                            }
                                        }
                                        cse4589_print_and_log("[%s:END]\n", ptr2);
                                        fflush(stdout);
                                    }
                                    else
                                    {
                                        cse4589_print_and_log("[%s:ERROR]\n", ptr2);
                                        cse4589_print_and_log("[%s:END]\n", ptr2);
                                    }
                                    free(buffer);
                                }
                                else if (strcmp(ptr2, "LOGOUT") == 0)
                                {
                                    // client_output << "Command After login:|" << ptr2 << "|\n";
                                    char *tell_server = (char *)malloc(sizeof(char) * BUFFER_SIZE);
                                    memset(tell_server, '\0', BUFFER_SIZE);
                                    strcat(tell_server, msg);
                                    strcat(tell_server, " ");
                                    strcat(tell_server, ip_addr);
                                    if (send(fdsocket, tell_server, strlen(tell_server), 0) < 0)
                                        perror("Failed sending my command");
                                    fflush(stdout);
                                    /* Initialize buffer to receieve response */
                                    char *buffer = (char *)malloc(sizeof(char) * BUFFER_SIZE);
                                    memset(buffer, '\0', BUFFER_SIZE);
                                    if (recv(fdsocket, buffer, BUFFER_SIZE, 0) >= 0)
                                    {
                                        cse4589_print_and_log("[%s:SUCCESS]\n", ptr2);
                                        cse4589_print_and_log("[%s:END]\n", ptr2);
                                        fflush(stdout);
                                        close(fdsocket);
                                        break; // logout
                                    }
                                    else
                                    {
                                        cse4589_print_and_log("[%s:ERROR]\n", ptr2);
                                        cse4589_print_and_log("[%s:END]\n", ptr2);
                                    }
                                    free(buffer);
                                    free(tell_server);
                                }
                                else if (strcmp(ptr2, "EXIT") == 0)
                                {
                                    // client_output << "Command After login:|" << ptr2 << "|\n";
                                    char *tell_server = (char *)malloc(sizeof(char) * BUFFER_SIZE);
                                    memset(tell_server, '\0', BUFFER_SIZE);
                                    strcat(tell_server, "EXIT ");
                                    strcat(tell_server, ip_addr);
                                    if (send(fdsocket, tell_server, strlen(tell_server), 0) < 0)
                                    {
                                        // perror("Failed sending my command");
                                        cse4589_print_and_log("[%s:ERROR]\n", ptr2);
                                        cse4589_print_and_log("[%s:END]\n", ptr2);
                                    }
                                    else
                                    {
                                        cse4589_print_and_log("[%s:SUCCESS]\n", ptr2);
                                        cse4589_print_and_log("[%s:END]\n", ptr2);
                                        free(tell_server);
                                        close(fdsocket);
                                        // client_output.close();
                                        exit(0);
                                    }
                                    free(tell_server);
                                    fflush(stdout);
                                }
                                else if (strncmp(ptr2, "SEND", 4) == 0)
                                {
                                    // client_output << "Command After login:|" << ptr2 << "|\n";
                                    char *to_c;
                                    char *message;
                                    char *tell_server = (char *)malloc(sizeof(char) * 1024);
                                    memset(tell_server, '\0', 1024);
                                    if ((strcmp(ptr2, "SEND") == 0) || (strcmp(ptr2, "SEND ") == 0) || (strlen(ptr2) < 16))
                                    {
                                        cse4589_print_and_log("[%s:ERROR]\n", ptr2);
                                        cse4589_print_and_log("[%s:END]\n", ptr2);
                                        free(tell_server);
                                        fflush(stdout);
                                        continue;
                                    }
                                    strtok(ptr2, " ");
                                    to_c = strtok(NULL, " ");
                                    if (client_vaildate_ip(to_c) && is_online(to_c, list_buffer))
                                    {
                                        message = strtok(NULL, "\0");
                                        // printf("msg: %s\n", message);
                                        // printf("msg length: %d\n", strlen(message));
                                        if (strlen(message) <= BUFFER_SIZE)
                                        {
                                            strcat(tell_server, "SEND ");
                                            strcat(tell_server, ip_addr);
                                            strcat(tell_server, " ");
                                            strcat(tell_server, to_c);
                                            strcat(tell_server, " ");
                                            strcat(tell_server, message);
                                            // cout << "MSG LENGTH: " << strlen(tell_server) << endl;
                                            if (send(fdsocket, tell_server, strlen(tell_server), 0) < 0)
                                            {
                                                // perror("Failed sending my command");
                                                cse4589_print_and_log("[%s:ERROR]\n", ptr2);
                                                cse4589_print_and_log("[%s:END]\n", ptr2);
                                            }
                                            else
                                            {
                                                cse4589_print_and_log("[%s:SUCCESS]\n", ptr2);
                                                cse4589_print_and_log("[%s:END]\n", ptr2);
                                            }
                                        }
                                        else
                                        {
                                            /* Invalid message length */
                                            cse4589_print_and_log("[%s:ERROR]\n", ptr2);
                                            cse4589_print_and_log("[%s:END]\n", ptr2);
                                        }
                                    }
                                    else
                                    {
                                        /* Invalid IP address || IP Non-exist in list buffer */
                                        cse4589_print_and_log("[%s:ERROR]\n", ptr2);
                                        cse4589_print_and_log("[%s:END]\n", ptr2);
                                    }
                                    free(tell_server);
                                    fflush(stdout);
                                }
                                else if (strncmp(ptr2, "BLOCK ", 6) == 0)
                                {
                                    // client_output << "Command After login:|" << ptr2 << "|\n";
                                    if ((strcmp(ptr2, "BLOCK") == 0) || (strcmp(ptr2, "BLOCK ") == 0))
                                    {
                                        cse4589_print_and_log("[%s:ERROR]\n", ptr2);
                                        cse4589_print_and_log("[%s:END]\n", ptr2);
                                        continue;
                                    }
                                    char *tell_server = (char *)malloc(sizeof(char) * 1024);
                                    memset(tell_server, '\0', 1024);
                                    strcat(tell_server, ptr2);
                                    strcat(tell_server, " ");
                                    strcat(tell_server, ip_addr);
                                    /* tell_server = "BLOCK <victim-ip> <my-ip>"" */
                                    strtok(ptr2, " ");
                                    char *victim_ip;
                                    victim_ip = strtok(NULL, "\0");
                                    if (client_vaildate_ip(victim_ip) && is_online(victim_ip, list_buffer))
                                    {
                                        if (already_blocked(victim_ip, my_block_list))
                                        {
                                            cse4589_print_and_log("[%s:ERROR]\n", ptr2);
                                            cse4589_print_and_log("[%s:END]\n", ptr2);
                                            free(tell_server);
                                            fflush(stdout);
                                            continue;
                                        }
                                        else
                                        {
                                            /* First time block */
                                            my_block_list.push_back(victim_ip);
                                            if (send(fdsocket, tell_server, strlen(tell_server), 0) < 0)
                                            {
                                                // perror("Failed sending my command");
                                                cse4589_print_and_log("[%s:ERROR]\n", ptr2);
                                                cse4589_print_and_log("[%s:END]\n", ptr2);
                                            }
                                            else
                                            {
                                                cse4589_print_and_log("[%s:SUCCESS]\n", ptr2);
                                                cse4589_print_and_log("[%s:END]\n", ptr2);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        cse4589_print_and_log("[%s:ERROR]\n", ptr2);
                                        cse4589_print_and_log("[%s:END]\n", ptr2);
                                    }
                                    free(tell_server);
                                    fflush(stdout);
                                }
                                else if (strncmp(ptr2, "UNBLOCK ", 6) == 0)
                                {
                                    // client_output << "Command After login:|" << ptr2 << "|\n";
                                    if ((strcmp(ptr2, "UNBLOCK") == 0) || (strcmp(ptr2, "UNBLOCK ") == 0))
                                    {
                                        cse4589_print_and_log("[%s:ERROR]\n", ptr2);
                                        cse4589_print_and_log("[%s:END]\n", ptr2);
                                        continue;
                                    }
                                    char *tell_server = (char *)malloc(sizeof(char) * 1024);
                                    memset(tell_server, '\0', 1024);
                                    strcat(tell_server, ptr2);
                                    strcat(tell_server, " ");
                                    strcat(tell_server, ip_addr);
                                    /* tell_server = "UNBLOCK <victim-ip> <my-ip>"" */
                                    strtok(ptr2, " ");
                                    char *victim_ip;
                                    victim_ip = strtok(NULL, "\0");
                                    if (client_vaildate_ip(victim_ip) && is_online(victim_ip, list_buffer))
                                    {
                                        if (!already_blocked(victim_ip, my_block_list))
                                        {
                                            cse4589_print_and_log("[%s:ERROR]\n", ptr2);
                                            cse4589_print_and_log("[%s:END]\n", ptr2);
                                            free(tell_server);
                                            fflush(stdout);
                                            continue;
                                        }
                                        else
                                        {
                                            /* First time block */
                                            int pos = -1;
                                            for (auto &c : my_block_list)
                                            {
                                                if (victim_ip == c)
                                                {
                                                    pos++;
                                                    break;
                                                }
                                                pos++;
                                            }
                                            // printf("Before unblock block list contain: %d client\n", my_block_list.size());
                                            my_block_list.erase(my_block_list.begin() + pos);
                                            // printf("After unblock block list contain: %d client\n", my_block_list.size());
                                            if (send(fdsocket, tell_server, strlen(tell_server), 0) < 0)
                                            {
                                                // perror("Failed sending my command");
                                                cse4589_print_and_log("[%s:ERROR]\n", ptr2);
                                                cse4589_print_and_log("[%s:END]\n", ptr2);
                                            }
                                            else
                                            {
                                                cse4589_print_and_log("[%s:SUCCESS]\n", ptr2);
                                                cse4589_print_and_log("[%s:END]\n", ptr2);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        cse4589_print_and_log("[%s:ERROR]\n", ptr2);
                                        cse4589_print_and_log("[%s:END]\n", ptr2);
                                    }
                                    free(tell_server);
                                    fflush(stdout);
                                }
                                else if (strncmp(ptr2, "BROADCAST ", 10) == 0)
                                {
                                    // client_output << "Command After login:|" << ptr2 << "|\n";
                                    if ((strcmp(ptr2, "BROADCAST") == 0) || (strcmp(ptr2, "BROADCAST ") == 0))
                                    {
                                        cse4589_print_and_log("[%s:ERROR]\n", ptr2);
                                        cse4589_print_and_log("[%s:END]\n", ptr2);
                                        continue;
                                    }
                                    char *bc_msg;
                                    char *tell_server = (char *)malloc(sizeof(char) * 1024);
                                    memset(tell_server, '\0', 1024);
                                    strtok(ptr2, " ");
                                    bc_msg = strtok(NULL, "\0");
                                    strcat(tell_server, "BROADCAST ");
                                    strcat(tell_server, ip_addr);
                                    strcat(tell_server, " ");
                                    strcat(tell_server, bc_msg);
                                    /* tell_server = "BROADCAST <my_ip> <message>" */
                                    if (send(fdsocket, tell_server, strlen(tell_server), 0) < 0)
                                    {
                                        // perror("Failed sending my command");
                                        cse4589_print_and_log("[%s:ERROR]\n", ptr2);
                                        cse4589_print_and_log("[%s:END]\n", ptr2);
                                    }
                                    else
                                    {
                                        cse4589_print_and_log("[%s:SUCCESS]\n", ptr2);
                                        cse4589_print_and_log("[%s:END]\n", ptr2);
                                    }
                                    free(tell_server);
                                    fflush(stdout);
                                }
                                else
                                {
                                    cse4589_print_and_log("[%s:ERROR]\n", ptr2);
                                    cse4589_print_and_log("[%s:END]\n", ptr2);
                                }
                            }
                            else if (FD_ISSET(fdsocket, &watch_server))
                            { //*
                                char *get_msg = (char *)malloc(sizeof(char) * 1024);
                                memset(get_msg, '\0', 1024);
                                if (recv(fdsocket, get_msg, 1024, 0) > 0)
                                {
                                    char *from_ip;
                                    char *msg_c;
                                    from_ip = strtok(get_msg, " ");
                                    msg_c = strtok(NULL, "\0");
                                    cse4589_print_and_log("[RECEIVED:SUCCESS]\n");
                                    cse4589_print_and_log("msg from:%s\n[msg]:%s\n", from_ip, msg_c);
                                    cse4589_print_and_log("[RECEIVED:END]\n");
                                }
                                else
                                {
                                    close(fdsocket);
                                    FD_CLR(fdsocket, &watch_server);
                                }
                                free(get_msg);
                            } //*
                        }     //*
                    }
                    // fflush(stdout);
                }
                else
                {
                    cse4589_print_and_log("[%s:ERROR]\n", input_ptr);
                    cse4589_print_and_log("[%s:END]\n", input_ptr);
                }
            }
            else
            {
                cse4589_print_and_log("[%s:ERROR]\n", input_ptr);
                cse4589_print_and_log("[%s:END]\n", input_ptr);
            }
        }
        else
        {
            cse4589_print_and_log("[%s:ERROR]\n", input_ptr);
            cse4589_print_and_log("[%s:END]\n", input_ptr);
        }
        free(command_str);
        // client_output.close();
    }
}
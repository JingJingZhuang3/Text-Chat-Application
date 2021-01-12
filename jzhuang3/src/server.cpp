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
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <algorithm>
#include <fstream>
#include <vector>

#include "../include/global.h"
#include "../include/logger.h"
#include "../include/server.h"
#include "../include/info.h"
#include "../include/block_info.h"
#include "../include/msg_info.h"

using namespace std;

bool comp_port(const info &lhs, const info &rhs)
{
    return lhs.port_num < rhs.port_num;
}
bool comp_block(const block &lhs, const block &rhs)
{
    return lhs.port_num < rhs.port_num;
}
bool vaildate_ip(char *ip)
{
    struct sockaddr_in addr;
    int vaild = inet_pton(AF_INET, ip, &(addr.sin_addr));
    return vaild != 0;
}

bool is_exist(char *ip, vector<info> list)
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

bool is_blocked(char *to_c, char *from_c, vector<info> list)
{
    bool check = false;
    for (auto &c : list)
    {
        if (strcmp(to_c, c.ip) == 0)
        {
            /* check receiver's block list*/
            for (auto &b : c.block_list)
            {
                if (strcmp(b.ip, from_c) == 0)
                {
                    /* sender is in the block list*/
                    check = true;
                    break;
                }
            }
        }
    }
    return check;
}

string get_hostname(char *ip, vector<info> list)
{
    for (auto &c : list)
    {
        if (strcmp(ip, c.ip) == 0)
        {
            return c.host_name;
        }
    }
    return "";
}

long get_port(char *ip, vector<info> list)
{
    for (auto &c : list)
    {
        if (strcmp(ip, c.ip) == 0)
        {
            return c.port_num;
        }
    }
    return 0;
}

server::server(int port)
{
    // ofstream server_output;
    // server_output.open ("/home/csdue/jzhuang3/server_op.txt");
    int server_socket, head_socket, selret, sock_index, fdaccept = 0;
    socklen_t caddr_len;
    struct sockaddr_in server_addr, client_addr;
    fd_set master_list, watch_list;
    /* 
    * Get hostname and IP address using UDP socket, method reference from  
	* https://ubmnc.wordpress.com/2010/09/22/on-getting-the-ip-name-of-a-machine-for-chatty/
    * 
	* Create UDP socket:
    */
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
    char ip_addr[16]; //Get my ip address
    inet_ntop(AF_INET, &myaddr.sin_addr, ip_addr, sizeof(ip_addr));
    //cout << ip_addr << endl;
    close(UDP_fd); //close UDP socket
    /* Get hostname */
    struct hostent *host;
    struct in_addr ipv4addr;
    inet_pton(AF_INET, ip_addr, &ipv4addr);
    host = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET);
    //cout << host->h_name << endl;

    /* Create TCP Socket */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Cannot create socket");
        exit(1);
    }
    /* Fill up sockaddr_in struct */
    bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    /* Bind */
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
    }
    /* Listen */
    if (listen(server_socket, BACKLOG) < 0)
    {
        perror("Unable to listen on port");
        exit(1);
    }

    /* ---------------------------------------------------------------------------- */

    /* Zero select FD sets */
    FD_ZERO(&master_list);
    FD_ZERO(&watch_list);

    /* Register the listening socket */
    FD_SET(server_socket, &master_list);
    /* Register STDIN */
    FD_SET(STDIN, &master_list);
    head_socket = server_socket;
    char *input_ptr;
    vector<info> list;
    while (TRUE)
    {
        memcpy(&watch_list, &master_list, sizeof(master_list));
        /* select() system call. This will BLOCK */
        selret = select(head_socket + 1, &watch_list, NULL, NULL, NULL);
        if (selret < 0)
            perror("select failed.");

        /* Check if we have sockets/STDIN to process */
        if (selret > 0)
        {
            /* Loop through socket descriptors to check which ones are ready */
            for (sock_index = 0; sock_index <= head_socket; sock_index += 1)
            {

                if (FD_ISSET(sock_index, &watch_list))
                {

                    /* Check if new command on STDIN */
                    if (sock_index == STDIN)
                    {
                        char *command_str = (char *)malloc(sizeof(char) * CMD_SIZE);
                        memset(command_str, '\0', CMD_SIZE);
                        if (fgets(command_str, CMD_SIZE - 1, stdin) == NULL) //Mind the newline character that will be written to cmd
                            exit(-1);
                        input_ptr = strtok(command_str, "\n");
                        // cout << "cmd: " << input_ptr << endl;
                        // cout << "whole: " << command_str << endl;
                        //cout << strcmp(input_ptr, "AUTHOR") << "size of cmd: " << sizeof(input_ptr) << endl;
                        if (strcmp(input_ptr, "AUTHOR") == 0)
                        {
                            char ubitname[] = "jzhuang3";
                            cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                            cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n",
                                                  ubitname);
                            cse4589_print_and_log("[%s:END]\n", command_str);
                        }
                        else if (strcmp(input_ptr, "IP") == 0)
                        {
                            cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                            // Resource from Beej Socket Guide
                            cse4589_print_and_log("IP:%s\n", ip_addr);
                            cse4589_print_and_log("[%s:END]\n", command_str);
                        }
                        else if (strcmp(input_ptr, "PORT") == 0)
                        {
                            cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                            cse4589_print_and_log("PORT:%d\n", port);
                            cse4589_print_and_log("[%s:END]\n", command_str);
                        }
                        else if (strcmp(input_ptr, "LIST") == 0)
                        {
                            // server_output << "Command:|" << input_ptr << "|\n";
                            cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                            int i = 0;
                            // cout << "Size of list: " << list.size() << endl;
                            sort(list.begin(), list.end(), comp_port); // sort the port number
                            for (auto &hst : list)
                            {
                                //Print only logged-in client
                                if (hst.host_name != nullptr && hst.status == "logged-in")
                                {
                                    i += 1;
                                    cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", i, hst.host_name, hst.ip, hst.port_num);
                                }
                            }
                            cse4589_print_and_log("[%s:END]\n", command_str);
                        }
                        else if (strcmp(input_ptr, "STATISTICS") == 0)
                        {
                            // server_output << "Command:|" << input_ptr << "|\n";
                            cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                            int i = 0;
                            sort(list.begin(), list.end(), comp_port); // sort the port number
                            for (auto &hst : list)
                            {
                                //Print whole table
                                if (hst.host_name != nullptr)
                                {
                                    i += 1;
                                    cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", i, hst.host_name, hst.num_msg_sent,
                                                          hst.num_msg_rcv, hst.status.c_str());
                                }
                            }
                            cse4589_print_and_log("[%s:END]\n", command_str);
                        }
                        else if (strncmp(input_ptr, "BLOCKED", 7) == 0)
                        {
                            // server_output << "Command:|" << input_ptr << "|\n";
                            char *client_ip;
                            strtok(input_ptr, " ");
                            client_ip = strtok(NULL, " "); // get client ip
                            // check ip address validation
                            info c_info;
                            if (client_ip != nullptr && vaildate_ip(client_ip) && is_exist(client_ip, list))
                            {
                                bool found = false;
                                for (auto &c : list) // find the client info based on this ip address
                                {
                                    if (strcmp(client_ip, c.ip) == 0)
                                    {
                                        c_info = c;
                                        found = true;
                                        break;
                                    }
                                }
                                if (!found) // IP not found in the list
                                {
                                    cse4589_print_and_log("[%s:ERROR]\n", input_ptr);
                                    cse4589_print_and_log("[%s:END]\n", input_ptr);
                                }
                                else
                                {
                                    cse4589_print_and_log("[%s:SUCCESS]\n", input_ptr);
                                    int i = 0;
                                    sort(c_info.block_list.begin(), c_info.block_list.end(), comp_block); // sort the port number
                                    for (auto &victim : c_info.block_list)
                                    {
                                        //Display the client list who was blocked by <ip>
                                        if (victim.host_name != nullptr)
                                        {
                                            i += 1;
                                            cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", i, victim.host_name, victim.ip, victim.port_num);
                                        }
                                    }
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
                            cse4589_print_and_log("[%s:ERROR]\n", command_str);
                            cse4589_print_and_log("[%s:END]\n", command_str);
                        }
                        free(command_str);
                    }
                    /* Check if new client is requesting connection */
                    else if (sock_index == server_socket)
                    {
                        caddr_len = sizeof(client_addr);
                        fdaccept = accept(server_socket, (struct sockaddr *)&client_addr, &caddr_len);
                        if (fdaccept < 0)
                        {
                            perror("Accept failed.");
                            continue;
                        }
                        else
                        {
                            /* Add to watched socket list */
                            FD_SET(fdaccept, &master_list);
                            if (fdaccept > head_socket)
                            {
                                head_socket = fdaccept;
                            }
                            /* Get client ip address */
                            char client_ip_addr[16];
                            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip_addr, sizeof(client_ip_addr));
                            // printf("Client IP: %s\n", client_ip_addr);
                            /* Get hostname */
                            struct hostent *client_host;
                            struct in_addr ipv4addr;
                            inet_pton(AF_INET, client_ip_addr, &ipv4addr);
                            client_host = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET);
                            // printf("Client hostname: %s\n", client_host->h_name);
                            /* Get client port number */
                            char *client_port = (char *)malloc(sizeof(char) * BUFFER_SIZE);
                            memset(client_port, '\0', BUFFER_SIZE);
                            if (recv(fdaccept, client_port, sizeof(client_port), 0) < 0)
                            {
                                perror("Failed to receive client listening port.");
                            }
                            // printf("Client port: %d", atoi(client_port));
                            /* Check is the client already in the LIST */
                            bool found_client = false;
                            // printf("\nList size:%d", list.size());
                            for (auto &c : list) // find the client info based on this ip address
                            {
                                // cout << "\ncompare client ip: " << client_ip_addr << " with list record:" << c.ip << endl;
                                // cout << "\n**List 4: " << c.host_name << " " << c.ip << " " << c.port_num << endl;
                                if (strcmp(client_ip_addr, c.ip) == 0)
                                {
                                    // cout << "\nupdate client loggin status" << endl;
                                    /* update login status */
                                    c.status = "logged-in";
                                    found_client = true;
                                    break;
                                }
                            }
                            if (!found_client) // IP not found in the list
                            {
                                info client_info;
                                // cout << "\nadd new client" << endl;
                                /* Add new client information to List */
                                client_info.sock_idx = fdaccept;
                                strcpy(client_info.host_name, client_host->h_name);
                                strcpy(client_info.ip, client_ip_addr);
                                client_info.port_num = atoi(client_port);
                                client_info.status = "logged-in";
                                list.push_back(client_info);
                            }
                            /* Get logged in client information */
                            char *list_to_client = (char *)malloc(sizeof(char) * 8192);
                            memset(list_to_client, '\0', 8192);
                            strcat(list_to_client, "-*");
                            for (auto &live : list)
                            {
                                //Print only logged-in client
                                if (live.host_name != nullptr && live.status == "logged-in")
                                {
                                    char buf[8];
                                    strcat(list_to_client, live.host_name);
                                    strcat(list_to_client, " ");
                                    strcat(list_to_client, live.ip);
                                    strcat(list_to_client, " ");
                                    snprintf(buf, sizeof(buf), "%d", live.port_num);
                                    strcat(list_to_client, buf);
                                    strcat(list_to_client, " ");
                                }
                            }
                            // cout << list_to_client << endl;
                            /* Get client message buffer information */
                            // char msg_rcv[2048];
                            // strcat(list_to_client, "embankment.cse.buffalo.edu 128.205.36.35 5000 "); //test
                            strcat(list_to_client, "& -`"); //delimiters
                            for (auto &c : list)            // find the client info based on this ip address
                            {
                                if (strcmp(client_ip_addr, c.ip) == 0)
                                {
                                    for (auto &m : c.msg_buffer)
                                    {
                                        if (c.msg_buffer.size() > 0)
                                        {
                                            cse4589_print_and_log("[RELAYED:SUCCESS]\n");
                                            cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", m.from_ip, m.rcv_ip, m.msg);
                                            cse4589_print_and_log("[RELAYED:END]\n");
                                        }
                                        if (strcmp(client_ip_addr, m.rcv_ip) == 0)
                                        {
                                            // printf("----------------------------------\n");
                                            strcat(list_to_client, m.from_ip);
                                            strcat(list_to_client, "\n");
                                            // printf("IN LOOP 1:\n%s\n", list_to_client);
                                            strcat(list_to_client, m.msg);
                                            // printf("MSG:\n%s\n", m.msg);
                                            strcat(list_to_client, "\n");
                                            // printf("IN LOOP 2:\n%s\n", list_to_client);
                                        }
                                    }
                                    break;
                                }
                            }
                            // printf("SEND TO CLIENT:\n%s\n", list_to_client);
                            if (send(fdaccept, list_to_client, strlen(list_to_client), 0) < 0)
                                perror("Failed to respond login client list.");
                            free(list_to_client);
                            free(client_port);
                        }
                    }
                    /* Read from existing clients */
                    else
                    {
                        /* Initialize buffer to receieve response */
                        char *buffer = (char *)malloc(sizeof(char) * 1024);
                        memset(buffer, '\0', 1024);
                        if ((recv(sock_index, buffer, 1024, 0) <= 0) || (strncmp(buffer, "EXIT", 4) == 0))
                        {
                            // printf("-test- %s\n", buffer);
                            if (strncmp(buffer, "EXIT", 4) == 0)
                            {
                                char *c_ip;
                                int pos = -1;
                                strtok(buffer, " ");
                                // printf("-test-%s\n", cmd);
                                c_ip = strtok(NULL, "\0");
                                // printf("-test-%s\n", c_ip);
                                for (auto &c : list) // find the client info based on this ip address
                                {
                                    if (strcmp(c_ip, c.ip) == 0)
                                    {
                                        pos++;
                                        break;
                                    }
                                    pos++;
                                }
                                // printf("-test 1-%d", pos);
                                list.erase(list.begin() + pos);
                                // printf("-test 2-");
                                fflush(stdout);
                            }
                            close(sock_index);
                            // printf("Remote Host terminated connection!\n");
                            /* Remove from watched list */
                            FD_CLR(sock_index, &master_list);
                        }
                        else
                        {
                            //Process incoming data from existing clients here ...
                            if (strcmp(buffer, "REFRESH") == 0)
                            {
                                char *live_client = (char *)malloc(sizeof(char) * 2048);
                                memset(live_client, '\0', 2048);
                                strcat(live_client, "~*");
                                sort(list.begin(), list.end(), comp_port);
                                for (auto &live2 : list)
                                {
                                    //Print only logged-in client
                                    if (live2.host_name != nullptr && live2.status == "logged-in")
                                    {
                                        char buf[8];
                                        strcat(live_client, live2.host_name);
                                        strcat(live_client, " ");
                                        strcat(live_client, live2.ip);
                                        strcat(live_client, " ");
                                        snprintf(buf, sizeof(buf), "%d", live2.port_num);
                                        strcat(live_client, buf);
                                        strcat(live_client, " ");
                                    }
                                }
                                strcat(live_client, "& ");
                                if (send(sock_index, live_client, strlen(live_client), 0) < 0)
                                    perror("Failed to respond back.");
                                fflush(stdout);
                                free(live_client);
                            }
                            else if (strncmp(buffer, "LOGOUT", 6) == 0)
                            {
                                if (send(sock_index, buffer, strlen(buffer), 0) == strlen(buffer))
                                {
                                    char *c_ip;
                                    strtok(buffer, " ");
                                    c_ip = strtok(NULL, " ");
                                    for (auto &c : list) // find the client info based on this ip address
                                    {
                                        if (strcmp(c_ip, c.ip) == 0)
                                        {
                                            /* update login status */
                                            c.status = "logged-out";
                                            break;
                                        }
                                    }
                                }
                                else
                                {
                                    perror("Failed to respond logout success back.");
                                }
                                fflush(stdout);
                            }
                            else if (strncmp(buffer, "SEND", 4) == 0)
                            {
                                char *from_c;
                                char *to_c;
                                char *msg;
                                strtok(buffer, " ");
                                from_c = strtok(NULL, " "); //sender ip
                                to_c = strtok(NULL, " ");   //receiver ip
                                msg = strtok(NULL, "\0");   //message
                                /* check is the client exist */
                                if (is_exist(to_c, list) && (!is_blocked(to_c, from_c, list)))
                                {
                                    // cse4589_print_and_log("[RELAYED:SUCCESS]\n");
                                    // cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", from_c, to_c, msg);
                                    // cse4589_print_and_log("[RELAYED:END]\n");
                                    /* store msg info */
                                    message s_msg;
                                    strcpy(s_msg.from_ip, from_c);
                                    strcpy(s_msg.rcv_ip, to_c);
                                    strcpy(s_msg.msg, msg);
                                    /* find client info & record sended message */
                                    for (auto &c : list)
                                    {
                                        if ((strcmp(from_c, c.ip) == 0))
                                        {
                                            c.num_msg_sent++;
                                        }
                                        if ((strcmp(to_c, c.ip) == 0) && (c.status == "logged-out") && (!is_blocked(to_c, from_c, list)))
                                        {
                                            c.num_msg_rcv++;
                                            // printf("PUSHBACK: %s\n", s_msg.msg);
                                            c.msg_buffer.push_back(s_msg);
                                            // c.msg_buffer.insert(c.msg_buffer.begin(), 1, s_msg);
                                        }
                                        else if ((strcmp(to_c, c.ip) == 0) && (c.status == "logged-in") && (!is_blocked(to_c, from_c, list)))
                                        {
                                            cse4589_print_and_log("[RELAYED:SUCCESS]\n");
                                            cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", from_c, to_c, msg);
                                            cse4589_print_and_log("[RELAYED:END]\n");
                                            c.num_msg_rcv++;
                                            /* 实时发送到client那边 */
                                            char *send_msg = (char *)malloc(sizeof(char) * 1024);
                                            memset(send_msg, '\0', 1024);
                                            strcat(send_msg, from_c);
                                            strcat(send_msg, " ");
                                            strcat(send_msg, msg);
                                            if (send(c.sock_idx, send_msg, strlen(send_msg), 0) < 0)
                                                perror("Failed to respond back.");
                                            free(send_msg);
                                        }
                                    }
                                }
                                else
                                {
                                    cse4589_print_and_log("[RELAYED:ERROR]\n");
                                    cse4589_print_and_log("[RELAYED:END]\n");
                                }
                            }
                            else if (strncmp(buffer, "BLOCK", 5) == 0)
                            {
                                // printf("Client send me:%s\n", buffer);
                                char *victim;
                                char *sender; // who send this BLOCK command
                                strtok(buffer, " ");
                                victim = strtok(NULL, " ");
                                sender = strtok(NULL, " ");
                                block blk;
                                strcpy(blk.host_name, get_hostname(victim, list).c_str());
                                strcpy(blk.ip, victim);
                                blk.port_num = get_port(victim, list);
                                for (auto &c : list)
                                {
                                    if ((strcmp(sender, c.ip) == 0))
                                    {
                                        c.block_list.push_back(blk);
                                        break;
                                    }
                                }
                            }
                            else if (strncmp(buffer, "UNBLOCK", 5) == 0)
                            {
                                char *victim;
                                char *sender; // who send this BLOCK command
                                strtok(buffer, " ");
                                victim = strtok(NULL, " ");
                                sender = strtok(NULL, " ");
                                int pos = -1;
                                for (auto &c : list)
                                {
                                    if ((strcmp(sender, c.ip) == 0))
                                    {
                                        for (auto &b : c.block_list)
                                        {
                                            /* find the victim client in sender's block list*/
                                            if ((strcmp(victim, b.ip) == 0))
                                            {
                                                pos++;
                                                // printf("Found in block list\n");
                                                break;
                                            }
                                            pos++;
                                        }
                                        // printf("Before unblock block list size: %d\n", c.block_list.size());
                                        c.block_list.erase(c.block_list.begin() + pos);
                                        // printf("After unblock block list size: %d\n", c.block_list.size());
                                        break;
                                    }
                                }
                            }
                            else if (strncmp(buffer, "BROADCAST ", 10) == 0)
                            {
                                char *from_c;
                                char *msg;
                                strtok(buffer, " ");
                                from_c = strtok(NULL, " "); //sender ip
                                msg = strtok(NULL, "\0");   //message
                                for (auto &client : list)
                                {
                                    if ((strcmp(from_c, client.ip) == 0))
                                    {
                                        client.num_msg_sent++;
                                    }
                                    if ((strcmp(from_c, client.ip) != 0) && (!is_blocked(client.ip, from_c, list)))
                                    {
                                        // cse4589_print_and_log("[RELAYED:SUCCESS]\n");
                                        // cse4589_print_and_log("msg from:%s, to:255.255.255.255\n[msg]:%s\n", from_c, msg);
                                        // cse4589_print_and_log("[RELAYED:END]\n");
                                        message s_msg;
                                        strcpy(s_msg.from_ip, from_c);
                                        strcpy(s_msg.rcv_ip, client.ip);
                                        strcpy(s_msg.msg, msg);
                                        if (client.status == "logged-out")
                                        {
                                            client.num_msg_rcv++;
                                            client.msg_buffer.push_back(s_msg);
                                            // c.msg_buffer.insert(c.msg_buffer.begin(), 1, s_msg);
                                        }
                                        else if (client.status == "logged-in")
                                        {
                                            cse4589_print_and_log("[RELAYED:SUCCESS]\n");
                                            cse4589_print_and_log("msg from:%s, to:255.255.255.255\n[msg]:%s\n", from_c, msg);
                                            cse4589_print_and_log("[RELAYED:END]\n");
                                            client.num_msg_rcv++;
                                            /* 实时发送到client那边 */
                                            char *send_msg = (char *)malloc(sizeof(char) * 1024);
                                            memset(send_msg, '\0', 1024);
                                            strcat(send_msg, from_c);
                                            strcat(send_msg, " ");
                                            strcat(send_msg, msg);
                                            if (send(client.sock_idx, send_msg, strlen(send_msg), 0) < 0)
                                                perror("Failed to respond back.");
                                            free(send_msg);
                                        }
                                    }
                                }
                            }
                        }
                        free(buffer);
                    }
                }
            }
        }
    }
    // server_output.close();
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <cmath>
#include <algorithm>
#include "common.h"

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        cout << "./router <router location file> <topology conf file> <router_id>" << endl;
        exit(EXIT_FAILURE);
    }

    //parse files
    router_map table;
    table.map_init(argv[1]);
    //init of the router
    router_vector client;
    int router_id = atoi(argv[3]);
    int vector_size = table.size;
    client.init(argv[2], table.id, router_id);

    cout << "the router ip is " << table.m[router_id].router_ip << endl
         << "ther router port is " << table.m[router_id].router_port << endl;

    //使用的时候应当是从1开始小于等于vector_size
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // create socket address
    struct sockaddr_in recv_addr;
    memset(&recv_addr, 0, sizeof(struct sockaddr_in));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_addr.s_addr = INADDR_ANY;
    //recv_addr means the router which this router will send message to it

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(table.m[router_id].router_port);
    if (inet_pton(AF_INET, table.m[router_id].router_ip, &address.sin_addr) <= 0)
    {
        perror("address failed");
        exit(EXIT_FAILURE);
    }
    if (bind(sock, (struct sockaddr *)&address, sizeof(struct sockaddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    socklen_t len = sizeof(sockaddr_in);
    //recv the ip and port from agent
    char buffer[1024]; //其实不会传输多少数据的
    Message *head = (Message *)buffer;
    cout << "waiting for the message from the agent" << endl;
    recvfrom(sock, buffer, 1024, 0, (struct sockaddr *)&address, &len);
    char agent_ip[20];
    int agent_port = head->port;
    strcpy(agent_ip, head->ip);
    cout << "the agent_ip is " << agent_ip << endl
         << "the agent_port is " << agent_port << endl;
    struct sockaddr_in agent_addr;
    memset(&agent_addr, 0, sizeof(struct sockaddr_in));
    agent_addr.sin_family = AF_INET;
    agent_addr.sin_port = htons(agent_port);
    if (inet_pton(AF_INET, agent_ip, &agent_addr.sin_addr) <= 0)
    {
        perror("address failed");
        exit(EXIT_FAILURE);
    }
    //wait for the connect of the agent and recv the command
    while (true)
    {
        //接受来自agent的消息
        recvfrom(sock, buffer, 1024, 0, (struct sockaddr *)&address, &len);
        Message *message = (Message *)buffer;
        //dv
        if (message->command == 1)
        {
            usleep(1000);
            bool changed = false;
            //只有改动过才会给其他的router发数据
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(sock, &fds);
            int tem = 1;
            while (tem)
            {
                changed = client.dv();
                //有变化就咬给所有的neighbor发消息
                if (changed)
                {
                    //init message
                    memset(message, 0, sizeof(Message));
                    message->send_id = router_id;
                    client.get_vector(message);
                    message->command = 7;
                    for (int i = 0; i < client.nei_id.size(); i++)
                    {
                        //init of recv_addr
                        int pos_id = client.nei_id[i];
                        recv_addr.sin_port = htons(table.m[pos_id].router_port);
                        if (inet_pton(AF_INET, table.m[pos_id].router_ip, &recv_addr.sin_addr) <= 0)
                        {
                            perror("address failed");
                            exit(EXIT_FAILURE);
                        }
                        memcpy(buffer, message, sizeof(Message));
                        cout << "the vector message will send to " << table.m[pos_id].router_port
                             << " " << table.m[pos_id].router_ip << endl;
                        for (int i = 0; i < vector_size; i++)
                            cout << message->_vector[i] << ",";
                        cout << endl;
                        sendto(sock, buffer, sizeof(Message), 0, (struct sockaddr *)&recv_addr, len);
                    }
                }
                else
                {
                    cout << "dv waiting while" << endl;
                    client.print();
                }

                tem = select(sock + 1, &fds, NULL, NULL, &timeout);
                if (tem == -1)
                {
                    cout << "unexpected error" << endl;
                    exit(EXIT_FAILURE);
                }
                else if (tem == 0)
                {
                    cout << "the dv algorithm finished" << endl;
                }
                else
                {
                    memset(message, 0, sizeof(Message));
                    recvfrom(sock, buffer, sizeof(Message), 0, (struct sockaddr *)&address, &len);
                    message = (Message *)buffer;
                    cout << "the router " << message->send_id << "'s vector is changed to" << endl;
                    for (int i = 0; i < vector_size; i++)
                        cout << message->_vector[i] << ",";
                    cout << endl;
                    client.change_vector(message);
                }
            }
            //send ack to agent
            message->send_id = router_id;
            message->command = 6;
            sendto(sock, buffer, sizeof(Message), 0, (struct sockaddr *)&agent_addr, len);
        }
        //update
        else if (message->command == 2)
        {
            if (message->send_id == -1)
                client.change_topy(message);
            else
                cout << "error message" << endl;
        }
        //show
        else if (message->command == 3)
        {
            if (message->send_id == -1)
            {
                memcpy(buffer, message, sizeof(Message));
                memset(message, 0, sizeof(Message));
                message->send_id = router_id;
                message->len = 0;
                message->command = 2;
                client.get_next(message);
                sendto(sock, buffer, sizeof(Message), 0, (struct sockaddr *)&agent_addr, len);
                message->len = 1;
                client.get_vector(message);
                sendto(sock, buffer, sizeof(Message), 0, (struct sockaddr *)&agent_addr, len);
            }
            else
                cout << "error message" << endl;
        }
        else if (message->command == 4)
        {
            if (message->send_id == -1)
            {
                client.vector_zero();
            }
            else
                cout << "error message" << endl;
        }
        else
        {
            cout << message->command << " " << message->len << endl;
        }
    }
    return 0;
}
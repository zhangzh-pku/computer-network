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
#include <fstream>
#include <cstring>

using namespace std;

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        cout << "./agent <router location file> " << endl;
        exit(EXIT_FAILURE);
    }
    char *filename = argv[1];
    char ip[10] = "127.0.0.1";
    int port = 9000;
    //config agent
    router_map server;
    server.map_init(filename);
    int vector_size = server.size;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    //init send sockaddr
    struct sockaddr_in send_addr;
    memset(&send_addr, 0, sizeof(sockaddr_in));
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &send_addr.sin_addr) <= 0)
    {
        perror("agent address failed");
        exit(EXIT_FAILURE);
    }
    if (bind(sock, (struct sockaddr *)&send_addr, sizeof(struct sockaddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // init recv sockaddr but some data will be change
    struct sockaddr_in recv_addr;
    recv_addr.sin_family = AF_INET;
    socklen_t len = sizeof(sockaddr_in);
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    //copy the message and ip to buffer
    Message *message = (Message *)buffer;
    message->command = 5;
    message->len = strlen(ip);
    message->send_id = -1;
    message->port = port;
    strcpy(message->ip, ip);
    /*
    cout << "the agent_ip is " << ip << endl
         << "the agent_port is " << port << endl;
    */
    //the first message is ip and port of the agent
    for (int i = 0; i < server.id.size(); i++)
    {

        int pos = server.id[i];
        if (inet_pton(AF_INET, server.m[pos].router_ip, &recv_addr.sin_addr) <= 0)
        {
            perror("address failed");
            exit(EXIT_FAILURE);
        }
        recv_addr.sin_port = htons(server.m[pos].router_port);
        /*
        cout << "the init message will send to " << server.m[pos].router_ip << " " << server.m[pos].router_port << endl
             << "and the message ip is " << ip << " port is " << message->port << endl;
        */
        sendto(sock, buffer, sizeof(Message), 0, (struct sockaddr *)&recv_addr, len);
    }
    //cout << "init has finished, please input the command" << endl;

    string dom;
    //read inputs from console
    while (cin >> dom)
    {
        if (dom == "dv")
        {
            memset(message, 0, sizeof(Message));
            message->command = 1;
            message->send_id = -1;
            memcpy(buffer, (char *)message, sizeof(Message));
            //send dv command to every router
            for (int i = 0; i < vector_size; i++)
            {
                int pos_id = server.id[i];
                if (inet_pton(AF_INET, server.m[pos_id].router_ip, &recv_addr.sin_addr) <= 0)
                {
                    perror("address failed");
                    exit(EXIT_FAILURE);
                }
                recv_addr.sin_port = htons(server.m[pos_id].router_port);
                sendto(sock, buffer, sizeof(Message), 0, (struct sockaddr *)&recv_addr, len); //给所有router发送dv指令
            }
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(sock, &fds);
            int tem = 1;
            int ack = 0;
            //每个router只会发一个ack，所以只要收到ack，计数+1即可
            while (tem)
            {
                tem = select(sock + 1, &fds, NULL, NULL, &timeout_agent);
                if (tem == -1)
                {
                    cout << "unexpected error" << endl;
                    exit(EXIT_FAILURE);
                }
                else if (tem == 0)
                    cout << "time out,please input the next command" << endl;
                else
                {
                    recvfrom(sock, buffer, sizeof(Message), 0, (struct sockaddr *)&send_addr, &len);
                    message = (Message *)buffer;
                    if (message->command == 6)
                        ack++;
                    else
                        cout << "error message from " << message->send_id << " and type is " << message->command << endl;
                    if (ack == vector_size)
                        break;
                }
            }
            //cout << "finished" << endl;
        }
        //update
        else if (dom[0] == 'u' && dom.length() >= 12)
        {
            int in_id, out_id, _len;
            int pos = 7;
            in_id = 0;
            while (dom[pos] != ',')
                in_id = in_id * 10 + dom[pos++] - 48;
            out_id = 0;
            pos++;
            while (dom[pos] != ',')
                out_id = out_id * 10 + dom[pos++] - 48;
            pos++;
            _len = 0;
            bool flag = (dom[pos] == '-');
            if (flag)
                pos++;
            while (pos != dom.length())
                _len = _len * 10 + dom[pos++] - 48;
            _len = flag ? -_len : _len;
            //因为消息是对称的 所以只要发给入度的边即可
            //init of message
            memset(message, 0, sizeof(Message));
            message->len = _len;
            message->in_id = in_id;
            message->out_id = out_id;
            message->send_id = -1;
            message->command = 2;
            memcpy(buffer, message, sizeof(Message));
            //init of recv_addr
            recv_addr.sin_port = htons(server.m[in_id].router_port);
            if (inet_pton(AF_INET, server.m[in_id].router_ip, &recv_addr.sin_addr) <= 0)
            {
                perror("address failed");
                exit(EXIT_FAILURE);
            }
            sendto(sock, buffer, sizeof(Message), 0, (struct sockaddr *)&recv_addr, len);
        }

        else if (dom[0] == 's' && dom.length() >= 6)
        {
            //init of command
            int in_id = 0;
            int pos = 5;
            while (pos != dom.length())
                in_id = in_id * 10 + dom[pos++] - 48;
            //init of message and send
            memset(message, 0, sizeof(Message));
            message->command = 3;
            message->send_id = -1;
            memcpy(buffer, message, sizeof(Message));
            recv_addr.sin_port = htons(server.m[in_id].router_port);
            if (inet_pton(AF_INET, server.m[in_id].router_ip, &recv_addr.sin_addr) <= 0)
            {
                perror("address failed");
                exit(EXIT_FAILURE);
            }
            sendto(sock, buffer, sizeof(Message), 0, (struct sockaddr *)&recv_addr, len);
            //receieve the message and parse it
            recvfrom(sock, buffer, sizeof(Message), 0, (struct sockaddr *)&send_addr, &len);
            message = (Message *)buffer;

            int jump[15] = {0};
            int value[15] = {0};
            //len=0 means the next jump
            //in the router I will send the next first
            if (message->len == 0)
            {
                memcpy(jump, message->_vector, sizeof(message->_vector));
            }
            else if (message->len == 1)
            {
                memcpy(value, message->_vector, sizeof(message->_vector));
            }
            else
            {
                cout << "error" << endl;
            }
            recvfrom(sock, buffer, sizeof(Message), 0, (struct sockaddr *)&send_addr, &len);
            if (message->len == 0)
            {
                memcpy(jump, message->_vector, sizeof(message->_vector));
            }
            else if (message->len == 1)
            {
                memcpy(value, message->_vector, sizeof(message->_vector));
            }
            else
            {
                cout << "error" << endl;
            }
            for (int i = 0; i < vector_size; i++)
            {
                if (value[i] < INF)
                    cout << "dest: " << server.id[i] << ", next: " << jump[i] << ", cost: " << value[i] << endl;
            }
            cout << endl;
        }
        else if (dom[0] == 'r' && dom.length() > 7)
        {
            int in_id = 0;
            int pos = 6;
            while (pos != dom.length())
            {
                in_id = in_id * 10 + dom[pos] - 48;
            }
            memset(message, 0, sizeof(Message));
            message->command = 4;
            message->send_id = -1;
            memcpy(buffer, message, sizeof(Message));
            recv_addr.sin_port = server.m[in_id].router_port;
            if (inet_pton(AF_INET, server.m[in_id].router_ip, &recv_addr) < 0)
            {
                perror("address failed");
                exit(EXIT_FAILURE);
            }
            sendto(sock, buffer, sizeof(Message), 0, (struct sockaddr *)&recv_addr, len);
            recvfrom(sock, buffer, sizeof(Message), 0, (struct sockaddr *)&send_addr, &len);
            message = (Message *)buffer;
        }
        else
        {
            cout << "unexpected input" << endl;
        }
    }
    return 0;
}
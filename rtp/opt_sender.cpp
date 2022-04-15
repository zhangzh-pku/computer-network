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
#include "util.h"
#include "rtp.h"
#include <string>
#include <queue>
#include <cmath>
#include <algorithm>
#include <queue>
#define SEND_BUFFER_SIZE 32768 // 32KB
#define MAX_BYTE_SIZE 1461
const std::string _mytype[4] = {"start", "end", "data", "ack"};
struct timeval timeout = {0, 500000}; //500ms延迟
void init_rtp(rtp_header_t *rtp)
{
    rtp->checksum = 0;
    rtp->length = 0;
    rtp->seq_num = 0;
    rtp->type = RTP_DATA;
}
bool send_single_message(uint16_t rtype, void *buffer, rtp_header_t *rtp, socklen_t len, int sock, sockaddr *receiver_addr, socklen_t *addrlen, char *message = NULL) //单一节点 单线程的输入输出直接封装为函数
{
    //rtype表示要发送的类型，其他数据与sendto函数相同
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    std::cout << _mytype[rtype] << " packet will be sent\n";
    init_rtp(rtp);
    rtp->type = rtype;
    if (rtype == RTP_START || rtype == RTP_END)
    {
        rtp->seq_num = 100 + rand();
    }
    else
    {
        rtp->seq_num = 0;
    }
    //复制start信息到缓冲区并发送
    memcpy(buffer, rtp, len);
    rtp->checksum = 0;
    rtp->checksum = compute_checksum(rtp, len);
    sendto(sock, (void *)buffer, len, 0, receiver_addr, *addrlen);
    int tem = select(sock + 1, &fds, NULL, NULL, &timeout);
    if (tem == 0)
    {
        if (rtype == RTP_START)
        {
            std::cout << "the " << _mytype[rtype] << " packet time out\n";
            return false;
        }
        else
        {
            return true;
        }
    }
    else if (tem == 1)
    {
        socklen_t receieve_bytes;
        receieve_bytes = recvfrom(sock, (void *)buffer, SEND_BUFFER_SIZE, 0, (struct sockaddr *)&receiver_addr, addrlen);
        if (receieve_bytes >= 0)
        {
            //std::cout << "received\n";
            rtp_header_t *recv_ack = (rtp_header_t *)buffer;
            if (recv_ack->type == RTP_ACK)
            {
                //std::cout << "type right\n";
                int pkt_checksum = recv_ack->checksum;
                recv_ack->checksum = 0;
                int computed_checksum = compute_checksum((void *)recv_ack, receieve_bytes);
                if (pkt_checksum == computed_checksum)
                {
                    std::cout << _mytype[rtype] << " ack received\n";
                    return true;
                }
                else
                {
                    std::cout << _mytype[rtype] << " ack is damaged\n";
                    return false;
                }
            }
        }
    }
    return false;
}

int sender(char *receiver_ip, char *receiver_port, int window_size, char *message)
{
    // create socket
    int sock = 0;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // create receiver address
    struct sockaddr_in receiver_addr;
    memset(&receiver_addr, 0, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(atoi(receiver_port));
    socklen_t receiver_addr_length = sizeof(sockaddr_in); //unsigned int
    // convert IPv4 or IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, receiver_ip, &receiver_addr.sin_addr) <= 0)
    {
        perror("address failed");
        exit(EXIT_FAILURE);
    }
    std::cout << "connected\n";
    //从文件夹中打开该文件 还是用c= =
    char buffer[SEND_BUFFER_SIZE];
    std::ifstream infile(message, std::ios::binary);
    //如果没有找到文件
    rtp_header_t *rtp = new rtp_header_t();
    if (!infile)
    {
        //只要send 文件名
        std::cout << "file not find, the filename will be sent\n";

        //发送start
        if (!send_single_message(RTP_START, buffer, rtp, sizeof(rtp_header_t), sock, (sockaddr *)&receiver_addr, &receiver_addr_length)) //发送start
        {
            std::cout << "ack receieve failed\n";
            exit(EXIT_FAILURE);
        }
        //发送filename
        rtp->seq_num = 1;
        memcpy(buffer + sizeof(rtp_header_t), message, sizeof(message));
        rtp->length = sizeof(message);
        if (!send_single_message(RTP_DATA, buffer, rtp, sizeof(rtp_header_t) + strlen(message), sock, (sockaddr *)&receiver_addr, &receiver_addr_length, message))
        {
            std::cout << "ack receieve failed\n";
            std::cout << "the data packet will be sent again\n";
        }
        //发送end
        if (!send_single_message(RTP_END, buffer, rtp, sizeof(rtp_header_t), sock, (sockaddr *)&receiver_addr, &receiver_addr_length))
        {
            std::cout << "ack receieve failed\n";
            std::cout << "the end packet will be sent again\n";
        }
        return 0;
    }
    //如果找到了文件
    infile.seekg(0, std::ios::end);
    std::cout << "the total len of the file is " << infile.tellg() << " bytes\n";
    socklen_t len = ceil((double)infile.tellg() / MAX_BYTE_SIZE); //严格的需要传输数据包
    char file_data[len][MAX_BYTE_SIZE + 5];                       //读入数据
    std::cout << len << " packets is needed" << std::endl;        //输出需要多少个包
    memset(file_data, 0, sizeof(file_data));
    socklen_t end_data = 0;
    infile.close();
    infile.open(message);
    for (int i = 0; i < len; i++)
    {
        infile.read(file_data[i], MAX_BYTE_SIZE); //这样有个好处是seq_num和endlen就一样了
        if (i + 1 == len)
        {
            end_data = infile.gcount();
        }
    }
    rtp = (rtp_header_t *)buffer;
    if (!send_single_message(RTP_START, buffer, rtp, sizeof(rtp_header_t), sock, (struct sockaddr *)&receiver_addr, &receiver_addr_length))
    {
        std::cout << "the end packet will be sent\n";
        send_single_message(RTP_END, buffer, rtp, sizeof(rtp_header_t), sock, (struct sockaddr *)&receiver_addr, &receiver_addr_length);
        exit(EXIT_FAILURE);
    }
    rtp->type = RTP_DATA;
    int seq = 0; //设置为seq_num 下一个需要被发送的最小值
    //need_size表示上次传输有顺序的ack的最大值，假设以1开始计数
    bool ack[window_size + 5]; //表示有没有收到所有的ack
    memset(ack, false, sizeof(ack));
    int send_size = 0;
    while (seq < len)
    {
        send_size = 0;
        for (int i = 0; (i < window_size) && (seq + i < len); i++)
        {
            if (!ack[i]) //表明没有收到该数据的ack
            {
                send_size++;
                rtp = (rtp_header_t *)buffer;
                rtp->seq_num = seq + i;
                rtp->length = ((seq + i + 1) == len) ? end_data : MAX_BYTE_SIZE;
                rtp->checksum = 0;
                memcpy(rtp + sizeof(rtp_header_t), file_data[seq + i], rtp->length);
                rtp->checksum = compute_checksum(rtp, sizeof(rtp_header_t) + rtp->length);
                sendto(sock, buffer, sizeof(rtp_header_t) + rtp->length, 0, (struct sockaddr *)&receiver_addr, receiver_addr_length);
                std::cout << "the " << rtp->seq_num << " packet is sent\n";
            }
        }
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        int total = 0;
        int tem = 1;
        while (total < send_size && tem != 0) //如果已经有need_size个文件，那就不需要再等
        {
            tem = select(sock + 1, &fds, NULL, NULL, &timeout);
            if (tem == -1)
            {
                std::cout << "unexpected case,break\n";
                exit(EXIT_FAILURE);
                break;
            }
            else if (tem == 0)
            {
                std::cout << "all of the ack packet we had just sent has been received,the next round will start\n";
                break;
            }
            else
            {
                int recv_len;
                recv_len = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&receiver_addr, &receiver_addr_length);
                rtp = (rtp_header_t *)buffer;
                int pkt_checksum = rtp->checksum;
                rtp->checksum = 0;
                int computed_checksum = compute_checksum(rtp, recv_len);
                if (pkt_checksum == computed_checksum) //这个packet是正确的
                {
                    if (!ack[rtp->seq_num - seq]) //针对error code3 防止ack重传
                    {
                        ack[rtp->seq_num - seq] = true;
                        total++;
                    }
                    std::cout << "the " << rtp->seq_num << " packet ack is received\n";
                }
                else
                {
                    std::cout << "the " << rtp->seq_num << " packet has a wrong ack\n";
                }
            }
        }
        bool full = true;
        for (int i = 0; (i < window_size) && (seq + i < len); i++)
        {
            if (!ack[i])
            {
                full = false;
            }
        }
        if (full)
        {
            std::cout << "all the packet for this window is sent,the next window will be sent\n";
            seq += window_size;
            memset(ack, false, sizeof(ack));
        }
    }
    if (send_single_message(RTP_END, buffer, rtp, sizeof(rtp_header_t), sock, (struct sockaddr *)&receiver_addr, &receiver_addr_length))
    {
        std::cout << "finished\n"
                  << "the program will close\n";
    }
    // close socket
    close(sock);
    return 0;
}

/*
 * main()
 * Parse command-line arguments and call sender function
*/
int main(int argc, char **argv)
{
    char *receiver_ip;
    char *receiver_port;
    int window_size;
    char *message;

    if (argc != 5)
    {
        fprintf(stderr, "Usage: ./sender [Receiver IP] [Receiver Port] [Window Size] [Message]");
        exit(EXIT_FAILURE);
    }
    receiver_ip = argv[1];
    receiver_port = argv[2];
    window_size = atoi(argv[3]);
    message = argv[4];
    return sender(receiver_ip, receiver_port, window_size, message);
}
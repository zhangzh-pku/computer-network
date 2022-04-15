#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <fstream>
#include "rtp.h"
#include "util.h"
#define maxlen 1461
#define RECV_BUFFER_SIZE 32768 // 32KB
struct timeval timeout = {20, 0};

int receiver(char *receiver_port, int window_size, char *file_name)
{

    char buffer[RECV_BUFFER_SIZE];

    // create socket file descriptor
    int sock;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // create socket address
    struct sockaddr_in address;
    memset(&address, 0, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(atoi(receiver_port));
    socklen_t address_len = sizeof(sockaddr_in);
    // bind socket to address
    if (bind(sock, (struct sockaddr *)&address, sizeof(struct sockaddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    int recv_size = 0;
    std::cout << "ready to receive\n";
    recv_size = recvfrom(sock, buffer, RECV_BUFFER_SIZE, 0, (struct sockaddr *)&address, &address_len);
    rtp_header_t *rtp = (rtp_header_t *)buffer;
    if (rtp->type != RTP_START)
    {
        std::cout << "the packet type is wrong, break\n";
        exit(EXIT_FAILURE);
    }
    int pkt_checksum = rtp->checksum;
    rtp->checksum = 0;
    int computed_checksum = compute_checksum(rtp, recv_size);
    if (computed_checksum != pkt_checksum)
    {
        std::cout << "the start checksum is wrong\n";
        exit(EXIT_FAILURE);
    }
    std::cout << "the start packet is sent,the ack will be sent\n"
              << "the start seq_num is " << rtp->seq_num << "\n";
    rtp->type = RTP_ACK;
    rtp->length = 0;
    rtp->checksum = 0;
    rtp->checksum = compute_checksum(rtp, sizeof(rtp_header_t));
    sendto(sock, buffer, sizeof(rtp_header_t), 0, (struct sockaddr *)&address, address_len);
    std::fstream out(file_name, std::ios::binary | std::ios::out);
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    int tem = 1;
    socklen_t seq = 0, need_seq = 0; //seq表示需要的seq_num,need_seq表示此时需要被发送的seq
    bool ack[window_size];
    memset(ack, false, sizeof(ack));
    char file_data[window_size + 2][1472 + 2];
    memset(file_data, 0, sizeof(file_data));
    while (tem)
    {
        tem = select(sock + 1, &fds, NULL, NULL, &timeout);
        if (tem == -1)
        {
            std::cout << "not your wrong\n";
            exit(EXIT_FAILURE);
        }
        else if (tem == 0) //理论上也不存在超时20秒的情况
        {
            std::cout << "time error\n"
                      << "break\n";
            exit(EXIT_FAILURE);
        }
        else
        {
            recv_size = recvfrom(sock, buffer, RECV_BUFFER_SIZE, 0, (struct sockaddr *)&address, &address_len);
            rtp = (rtp_header_t *)buffer;
            int pkt_checksum = rtp->checksum;
            rtp->checksum = 0;
            int computed_checksum = compute_checksum(rtp, recv_size);
            if (computed_checksum != pkt_checksum)
            {
                if (rtp->type == RTP_END)
                {
                    std::cout << "the end packet checksum is wrong,break\n";
                    return 0;
                }
                else
                {
                    std::cout << "the checksum of" << rtp->seq_num << " is wrong\n";
                }
            }
            else
            {
                if (rtp->type == RTP_END)
                {
                    std::cout << "all of the packet has been received\n";
                    rtp = (rtp_header_t *)buffer;
                    rtp->type = RTP_ACK;
                    rtp->length = 0;
                    rtp->checksum = 0;
                    rtp->checksum = compute_checksum(rtp, sizeof(rtp_header_t));
                    sendto(sock, buffer, sizeof(rtp_header_t), 0, (struct sockaddr *)&address, address_len);
                    out.close();
                    std::cout << "the file will close\n";
                    return 0;
                }
                else
                {
                    if (rtp->seq_num < seq) //这个数据已经发过ack并且存储过了
                    {
                        std::cout << "the " << rtp->seq_num << " packet is repeated\n";
                    }
                    else
                    {
                        if (rtp->seq_num - seq >= window_size) //舍弃这段数据
                        {
                            std::cout << "the " << rtp->seq_num << " packet is oversize\n";
                            continue;
                        }
                        else if (rtp->seq_num > seq) //此时出现了非顺序传输，并且在窗口内
                        {
                            if (ack[rtp->seq_num - seq]) //已经被传输过的数据
                            {
                                std::cout << "the " << rtp->seq_num << " packet is repeated\n";
                            }
                            else //第一次出现的数据先存到缓存中
                            {
                                std::cout << "the " << rtp->seq_num << " packet is received\n";
                                ack[rtp->seq_num - seq] = true;
                                memcpy(file_data[rtp->seq_num - seq], buffer, recv_size); //事实上这里存储的是packet
                                std::cout << "it is writed in file_data" << rtp->seq_num - seq << "\n";
                                rtp = (rtp_header_t *)buffer;
                                rtp->type = RTP_ACK;
                                rtp->checksum = 0;
                                rtp->length = 0;
                                rtp->seq_num = seq;
                                rtp->checksum = compute_checksum(rtp, sizeof(rtp_header_t));
                                std::cout << "the " << rtp->seq_num << " ack has been sent\n";
                                sendto(sock, buffer, sizeof(rtp_header_t), 0, (struct sockaddr *)&address, address_len); //发送ack
                            }
                            std::cout << ((rtp_header_t *)file_data[1])->seq_num << "??????" << std::endl;
                        }
                        else //此时得到的就是我们希望的数据
                        {
                            std::cout << "the " << rtp->seq_num << " packet is received\n";
                            ack[rtp->seq_num - seq] = true; //ack[0]
                            memcpy(file_data[0], buffer, recv_size);
            
                            while (((int)(need_seq - seq) < window_size) && ack[need_seq - seq])
                            {
                                need_seq++;
                            }
                            //从seq到need_seq这一堆数据都需要被写入硬盘，且做数据位移操作
                            for (int i = 0; i < (need_seq - seq); i++)
                            {
                                rtp = (rtp_header_t *)file_data[i];
                                out.write(file_data[i] + sizeof(rtp_header_t), rtp->length);
                                //########################################################
                                if (i + need_seq - seq < window_size)
                                {
                                    memcpy(file_data[i], file_data[need_seq - seq + i], sizeof(rtp_header_t) + rtp->length);
                                    ack[i] = ack[need_seq - seq + i];
                                }
                            }
                            memset(file_data[window_size - (need_seq - seq)], 0, sizeof(file_data[0]) * (need_seq - seq + 1));
                            memset(ack + window_size - (need_seq - seq), false, sizeof(bool) * ((need_seq - seq) + 1)); //后面的数据置为false
                            seq = need_seq;
                            rtp = (rtp_header_t *)buffer;
                            rtp->type = RTP_ACK;
                            rtp->seq_num = seq;
                            rtp->checksum = 0;
                            rtp->length = 0;
                            rtp->checksum = compute_checksum(rtp, sizeof(rtp_header_t));
                            std::cout << "the " << rtp->seq_num << " ack has been sent\n";
                            sendto(sock, buffer, sizeof(rtp_header_t), 0, (struct sockaddr *)&address, address_len);
                        }
                    }
                }
            }
        }
    }
    // close socket
    close(sock);
    return 0;
}

/*
 * main():
 * Parse command-line arguments and call receiver function
*/
int main(int argc, char **argv)
{
    char *receiver_port;
    int window_size;
    char *file_name;

    if (argc != 4)
    {
        fprintf(stderr, "Usage: ./receiver [Receiver Port] [Window Size] [File Name]\n");
        exit(EXIT_FAILURE);
    }

    receiver_port = argv[1];
    window_size = atoi(argv[2]);
    file_name = argv[3];
    return receiver(receiver_port, window_size, file_name);
}

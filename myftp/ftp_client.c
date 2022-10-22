#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "utils.c"

int sockfd = -1;
char buf[1024];

int main(int argc, char *argv[])
{
    init_headers();
    char cmd[10];
    while (1)
    {
        printf("#");
        scanf("%s", &cmd);
        if (cmd == "open")
        {
            char ip[20];
            int port;
            scanf("%s %d", &ip, &port);
            sockfd = open(ip, port);
            printf("connected");
        }
        if (cmd == "auth")
        {
            char name[20];
            char pswd[20];
            scanf("%s %d", &name, &pswd);
        }
        if (cmd == "ls")
        {
        }
        if (cmd == "quit")
        {
            quit();
        }
    }
}

int open(char ip[], int port)
{
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket error");
        exit(1);
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    int rfd = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if (rfd == -1)
    {
        perror("connect error");
        exit(1);
    }
    memcpy(buf, (void *)&OPEN_CONN_REPLY, sizeof(OPEN_CONN_REPLY));
    local_send(sockfd, (void *)buf, sizeof(OPEN_CONN_REQUEST));
    local_recv(sockfd, (void *)buf, sizeof(OPEN_CONN_REPLY));
    ftp_head tem;
    memcpy(&tem, buf, sizeof(OPEN_CONN_REPLY));
    if ((tem.m_protocol == OPEN_CONN_REPLY.m_protocol) && (tem.m_type == OPEN_CONN_REPLY.m_protocol) && (tem.m_status == 1))
    {
        print("connect succeed!");
    }
    return sockfd;
}

void auth(char user[], char pswd[])
{
    if (sockfd == -1)
    {
        perror("usage open ip port first");
        exit(1);
    }
    char payload[45];
    memcpy(payload, user, sizeof(char) * strlen(user));
    payload[strlen(user)] = ' ';
    memcpy(payload + strlen(user) + 1, pswd, sizeof(char) * strlen(pswd));
    payload[strlen(user) + strlen(pswd) + 1] = '\0';
    ftp_head tem = AUTH_REQUEST;
    tem.m_length = 13 + strlen(payload);
    memcpy(buf, (void *)&tem, sizeof(tem));
    memcpy(buf + sizeof(tem), payload, strlen(payload) + 1);
    int buflen;
    local_send(sockfd, (void *)buf, sizeof(tem) + strlen(payload) + 1);
    local_recv(sockfd, (void *)buf, sizeof(AUTH_REPLY));
    memcpy(&tem, buf, sizeof(AUTH_REPLY));
    if ((tem.m_type == AUTH_REPLY.m_type) && (tem.m_protocol == AUTH_REPLY.m_protocol))
    {
        if (tem.m_status == 1)
        {
            print("auth succeed");
        }
        else
        {
            // IDLE 状态
            print("auth error");
            quit();
        }
    }
}

void ls()
{

    ftp_head tem = LIST_REQUEST;
    memcpy(buf, (void *)&tem, sizeof(tem));
    local_send(sockfd, (void *)buf, sizeof(LIST_REQUEST));
    local_recv(sockfd, (void *)buf, sizeof(LIST_REPLY));
    char payload[2048];
    memcpy(payload, buf + sizeof(tem), tem.m_length);
    printf("%s", payload);
}

void quit()
{
    ftp_head tem = QUIT_REQUEST;
    memcpy(buf, (void *)&tem, sizeof(tem));
    local_send(sockfd, (void *)buf, sizeof(QUIT_REQUEST));
    local_recv(sockfd, (void *)buf, sizeof(QUIT_REPLY));
    memcpy(&tem, buf, sizeof(tem));
    if ((tem.m_protocol == QUIT_REPLY.m_protocol) && (tem.m_type == QUIT_REPLY.m_type))
    {
        print("quit succeed!");
    }
    close(sockfd);
    sockfd = -1;
}

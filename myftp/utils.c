#include <stdio.h>
typedef unsigned int uint32_t;
typedef unsigned char byte;
typedef unsigned char type;
#define MAGIC_NUMBER_LENGTH 6
#define PROTOCOL "0xe3myftp"
typedef unsigned char status;

typedef struct
{
    byte m_protocol[MAGIC_NUMBER_LENGTH]; /* protocol magic number (6 bytes) */
    type m_type;                          /* type (1 byte) */
    status m_status;                      /* status (1 byte) */
    uint32_t m_length;                    /* length (4 bytes) in Big endian*/
} __attribute__((packed)) ftp_head;

ftp_head OPEN_CONN_REQUEST;
ftp_head OPEN_CONN_REPLY;
ftp_head AUTH_REQUEST;
ftp_head AUTH_REPLY;
ftp_head LIST_REQUEST;
ftp_head LIST_REPLY;
ftp_head GET_REQUEST;
ftp_head GET_REPLY;
ftp_head FILE_DATA;
ftp_head PUT_REQUEST;
ftp_head PUT_REPLY;
ftp_head QUIT_REQUEST;
ftp_head QUIT_REPLY;

void init_headers()
{
    strcpy(OPEN_CONN_REQUEST.m_protocol, PROTOCOL);
    OPEN_CONN_REQUEST.m_type = 0xA1;
    OPEN_CONN_REQUEST.m_length = 12;
    strcpy(OPEN_CONN_REPLY.m_protocol, PROTOCOL);
    OPEN_CONN_REPLY.m_type = 0xA2;
    OPEN_CONN_REPLY.m_status = 1;
    OPEN_CONN_REPLY.m_length = 12;
    strcpy(AUTH_REQUEST.m_protocol, PROTOCOL);
    AUTH_REQUEST.m_type = 0xA3;
    strcpy(AUTH_REPLY.m_protocol, PROTOCOL);
    AUTH_REPLY.m_type = 0xA4;
    AUTH_REPLY.m_length = 12;
    strcpy(LIST_REQUEST.m_protocol, PROTOCOL);
    LIST_REQUEST.m_type = 0xA5;
    LIST_REQUEST.m_length = 12;
    strcpy(LIST_REPLY.m_protocol, PROTOCOL);
    LIST_REPLY.m_type = 0xA6;
    strcpy(GET_REQUEST.m_protocol, PROTOCOL);
    GET_REQUEST.m_type = 0xA7;
    strcpy(GET_REPLY.m_protocol, PROTOCOL);
    GET_REPLY.m_type = 0xA8;
    GET_REPLY.m_length = 12;
    strcpy(FILE_DATA.m_protocol, PROTOCOL);
    FILE_DATA.m_type = 0xFF;
    strcpy(PUT_REQUEST.m_protocol, PROTOCOL);
    PUT_REQUEST.m_type = 0xA9;
    strcpy(PUT_REPLY.m_protocol, PROTOCOL);
    PUT_REPLY.m_type = 0xAA;
    PUT_REPLY.m_length = 12;
    strcpy(QUIT_REQUEST.m_protocol, PROTOCOL);
    QUIT_REQUEST.m_type = 0xAB;
    QUIT_REQUEST.m_length = 12;
    strcpy(QUIT_REPLY.m_protocol, PROTOCOL);
    QUIT_REPLY.m_type = 0xAC;
    QUIT_REPLY.m_length = 12;
}
void local_send(int sock, void *buf, unsigned long size)
{
    int buflen = send(sock, buf, size, 0);
    if (buflen == 1)
    {
        perror("send failed");
        exit(1);
    }
}

void local_recv(int sock, void *buf, unsigned long size)
{
    int buflen = recv(sock, buf, size, 0);
    if (buflen == 1)
    {
        perror("recv failed");
        exit(1);
    }
}
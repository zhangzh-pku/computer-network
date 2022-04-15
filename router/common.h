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
#include <vector>
#include <fstream>
using namespace std;
#define INF 1000
#define MAXID 20

struct timeval timeout = {0, 5000};
struct timeval timeout_agent = {5, 0};

struct Message
{
    int send_id; //-1 agent else router_id
    int command; // 1 dv 2 update 3 show 4 reset 5 ip 6 ack 7 vector

    //menaningful for command = 5
    int port;
    //meaningful for command = 2 3 4
    int in_id;
    int out_id;
    int len; //len of ip or the value of new edge
    //meaningful for command = 5
    char ip[20];
    int _vector[10];
};

class router
//本质应当是个图的顶点
{
public:
    uint32_t router_port, router_id;
    char router_ip[20];
    //从一行字符串中解析一个router内容，形式严格为xxx.xxx.xxx.xxx,xxxxx,x
    router(string s)
    {
        int pos = 0;
        while (s[pos] != ',')
            pos++;
        for (int i = 0; i < pos; i++)
            router_ip[i] = s[i];
        router_ip[pos] = '\0';
        router_port = 0;
        pos++;
        while (s[pos] != ',')
            router_port = router_port * 10 + s[pos++] - 48;
        pos++;
        router_id = 0;
        while (pos != s.length())
            router_id = router_id * 10 + s[pos++] - 48;
    }
    router(router &r)
    {
        this->router_port = r.router_port;
        this->router_id = r.router_id;
        strcpy(this->router_ip, r.router_ip);
    }
    router()
    {
        memset(router_ip, 0, sizeof(router_ip));
        router_port = -1;
        router_id = -1;
    }
};

class router_map
{
public:
    router m[MAXID];
    int size;
    //for the router and the agent
    //message of router's port and i
    vector<int> id;
    void add_router(router s)
    {
        m[s.router_id] = s;
    }

    router_map()
    {
        size = 0;
        for (int i = 0; i < MAXID; i++)
            m[i] = router();
    }

    //parse the router config file
    void map_init(char *message)
    {
        fstream file;
        file.open(message, ios::in);
        if (!file.is_open())
        {
            cout << "the file path is wrong" << endl;
            exit(EXIT_FAILURE);
        }
        file >> size;
        if (size >= MAXID)
        {
            cout << "unexpected size of router,please reset it in the commom.h" << endl;
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < size; i++)
        {
            string tem;
            file >> tem;
            router s(tem);
            this->add_router(s);
            id.push_back(s.router_id);
        }
        /*
        for (int i = 0; i < size; i++)
            cout << id[i] << ",";
        cout << endl;
        */
    }
};

class router_vector
{
public:
    int router_id; //用于标示这是谁的vector
    //每个人都会知道所有的邻居信息
    int vect[MAXID][MAXID];
    //neighbor of this router
    //next单纯是一个队列
    int next[MAXID];
    //next[i]意义为到i个router的distance
    //当且仅当i是邻居时有意义
    int jump[MAXID];
    //jump[i]意义为到i个router的下一跳是谁
    //topy中为inf表示无链接
    vector<int> nei_id;
    int vector_size;
    //router_id of all routers
    vector<int> id;
    //parse the topology file
    bool is_change;
    void init(char *message, vector<int> _i, int r_id)
    {
        is_change = true;
        router_id = r_id;
        vector_size = _i.size();
        //init
        memset(jump, 0, sizeof(jump));
        jump[router_id] = router_id;
        for (int i = 0; i < MAXID; i++)
        {
            for (int j = 0; j < MAXID; j++)
            {
                vect[i][j] = INF;
                if (i == j)
                    vect[i][j] = 0;
            }
        }
        memset(next, INF, sizeof(next));
        fstream file;
        file.open(message, ios::in);
        if (!file.is_open())
        {
            cout << "the file is not discovered" << endl;
            exit(EXIT_FAILURE);
        }
        int n;
        file >> n;
        for (int i = 0; i < n; i++)
        {
            int in, out, len;
            char c;
            file >> in >> c >> out >> c >> len;
            //同时需要判断out是否是router_id的neighbor
            if (in == router_id && len != -1)
            {
                nei_id.push_back(out);
            }
            //为了逻辑的完整 将拓扑图的无链接设置为inf
            if (len != -1 && len < INF)
                vect[in][out] = len;
        }
        //init of id
        for (int i = 0; i < vector_size; i++)
        {
            id.push_back(_i[i]);
        }
        cout << "the router_id is" << router_id << endl;
        cout << "my neighbor is";
        for (int i = 0; i < nei_id.size(); i++)
        {
            cout << nei_id[i] << ",";
        }
        cout << endl;
        //init of next and jump
        for (int i = 0; i < nei_id.size(); i++)
        {
            int pos_id = nei_id[i];
            next[pos_id] = vect[router_id][pos_id];
            jump[pos_id] = pos_id;
        }
    }
    void print()
    {
        cout << "the neighbor is:" << endl;
        for (int i = 0; i < nei_id.size(); i++)
        {
            cout << nei_id[i] << ",";
        }
        cout << endl
             << "the vector is:" << endl;
        for (int i = 0; i < vector_size; i++)
        {
            int pos_id = id[i];
            cout << vect[router_id][pos_id] << ",";
        }
        cout << endl;
    }
    bool dv()
    {
        print();
        bool changed = false;
        int nei_size = nei_id.size();
        for (int i = 0; i < vector_size; i++)
        {
            if (id[i] == router_id)
                continue;
            int minnum = INF;
            int tem = -1;
            int pos_i = id[i];
            for (int i = 0; i < nei_size; i++)
            {
                int pos_id = nei_id[i];
                int dis = next[pos_id] + vect[pos_id][pos_i];
                if (dis < minnum)
                {
                    minnum = dis;
                    tem = pos_id;
                }
            }
            if (minnum != vect[router_id][pos_i])
            {
                changed = true;
            }
            vect[router_id][pos_i] = minnum;
            jump[pos_i] = tem;
        }
        /*
        cout << "in common.h the vector now is:" << endl;
        for (int i = 0; i < vector_size; i++)
        {
            cout << vect[router_id][id[i]] << ",";
        }
        cout << endl;
        */
        if (is_change)
        {
            is_change = false;
            return true;
        }
        return changed;
    }
    void get_next(Message *m)
    {
        //下一跳形成的队列
        for (int i = 0; i < vector_size; i++)
        {
            int pos_id = id[i];
            m->_vector[i] = jump[pos_id];
        }
    }
    void get_vector(Message *m)
    {

        for (int i = 0; i < vector_size; i++)
        {
            int pos_id = id[i];
            m->_vector[i] = vect[router_id][pos_id];
        }
    }
    void change_vector(Message *m)
    {

        for (int i = 0; i < vector_size; i++)
        {
            int pos_id = id[i];
            if (vect[m->send_id][pos_id] != m->_vector[i])
            {
                is_change = true;
                vect[m->send_id][pos_id] = m->_vector[i];
            }
        }
    }
    void vector_zero()
    {
        for (int i = 0; i < vector_size; i++)
            vect[router_id][i] = INF;
    }
    //once the topy is changed the topology must reset
    void change_topy(Message *m)
    {
        if (m->in_id != router_id)
        {
            cout << "wrong message from " << m->send_id << endl;
            return;
        }
        if (m->len == -1 || m->len > INF)
            m->len = INF;
        //delete neighbor
        vector<int>::iterator it;
        for (it = nei_id.begin(); it != nei_id.end(); it++)
        {
            if (*it == m->out_id)
                break;
        }
        if (m->len == INF && it != nei_id.end())
        {
            nei_id.erase(it);
            cout << "topy" << m->in_id << " " << m->out_id << "is erased" << endl;
        }

        else if (m->len != INF && it == nei_id.end())
            nei_id.push_back(m->out_id);
        vect[m->in_id][m->out_id] = m->len;
        next[m->out_id] = m->len;
        is_change = true;
        cout << "********" << endl;
        print();
        cout << "********" << endl;
    }
};

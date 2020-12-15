#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <set>
#include <vector>
#include <sstream>
#include <signal.h>

using namespace ::std;

#define PORTA 30997
#define PORTB 31997
#define TCPPORT 33997
#define UDPPORT 32997
#define BACKLOG 10 // how many pendign connections queue will hold

void get_countries(int);
void do_service(int);
string get_result(int, string, string);
set<string> country_list_a;
set<string> country_list_b;

vector<string> split(const string &str, const string &delim)
{
    vector<string> result;
    if (str == "")
        return result;
    char *str_temp = new char[str.length() + 1];
    strcpy(str_temp, str.c_str());
    char *delim_temp = new char[delim.length() + 1];
    strcpy(delim_temp, delim.c_str());

    char *tmp = strtok(str_temp, delim_temp);
    while (tmp)
    {
        string s1 = tmp;
        result.push_back(s1);
        tmp = strtok(NULL, delim_temp);
    }
    return result;
}

int main()
{

    signal(SIGCHLD, SIG_IGN);
    cout << "The Main server is up and running." << endl;

    get_countries(0);
    cout << "The Main server has received the country list from server A using UDP over port " << PORTA << endl;
    get_countries(1);
    cout << "The Main server has received the country list from server B using UDP over port " << PORTB << endl;

    //list the results of which country serverA/serverB is responsible for:
    cout << "Server A: " << endl;
    for (set<string>::iterator iter = country_list_a.begin(); iter != country_list_a.end(); ++iter)
        cout << *iter << endl;
    cout << "Server B: " << endl;
    for (set<string>::iterator iter = country_list_b.begin(); iter != country_list_b.end(); ++iter)
        cout << *iter << endl;

    //build socket
    int serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //bind the socket with ip and port
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(TCPPORT);

    //  bind : Address already in use
    int option = 1;
    if (setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int)) == -1)
    {
        perror("setsockopt");
        exit(1);
    }

    if (::bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind ");
        exit(1);
    }
    //listening
    if (listen(serv_sock, SOMAXCONN) < 0)
    {
        perror("listen ");
        exit(1);
    }
    //accpet the reqest
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);
    pid_t pid;

    while (true)
    {
        int accept_num = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        if (accept_num < 0)
        {
            perror("accept ");
            exit(1);
        }
        //fork a child socket
        pid = fork();
        if (pid == 0)
        {
            // child process
            close(serv_sock);
            do_service(accept_num);
            exit(0);
        }
        else
        {
            close(accept_num);
        }
    }
    close(serv_sock);
    return 0;
}

void do_service(int accept_num)
{
    char buffer[1000];
    int server_number = -1;
    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        string res = "";
        int n = recv(accept_num, buffer, 1000, 0);
        buffer[n] = '\0';

        if (n == 0)
        {
            printf("client close\n");
            break;
        }
        string buf(buffer);
        vector<string> recv = split(buf, " ");
        string country_name = recv[1];
        string user_id = recv[0];

        cout << "The Main server has received the request on User " << user_id << " in ( " << country_name << " ) from the client using TCP over port :" << TCPPORT << endl;

        //  check whether backend server A or B contains this country
        if (country_list_a.count(country_name))
        {
            server_number = 0;
        }
        else if (country_list_b.count(country_name))
        {
            server_number = 1;
        }

        if (server_number == -1)
        {
            cout << country_name << " does not show up in server A & B" << endl;
            res = "No_such_country";
        }
        else if (server_number == 0)
        {
            cout << country_name << " shows up in server A" << endl;
            cout << "The Main Server has sent request from User " << user_id << " to server A using UDP over port " << UDPPORT << endl;
            res = get_result(0, country_name, user_id);
        }
        else if (server_number == 1)
        {
            cout << country_name << " shows up in server B" << endl;
            cout << "The Main Server has sent request from User " << user_id << " to server B using UDP over port " << UDPPORT << endl;
            res = get_result(1, country_name, user_id);
        }

        if (res == "all_connected")
        {
            cout << "User " << user_id << " is already connected to all other users, no new recommendation" << endl;
        }
        else if (res == "No_such_user")
        {
            cout << "The Main server has received “User ID: Not found” from server <A/B>" << endl;
            cout << "The Main Server has sent error to client using TCP over " << TCPPORT << endl;
        }
        else if (res == "No_such_country")
        {

            cout << "The Main Server has sent “ " << country_name << ": Not found” to client1/2 using TCP over port " << TCPPORT << endl;
        }
        else
        {
            cout << "The Main Server has sent searching result(s) to client using TCP over port " << TCPPORT << endl;
        }

        send(accept_num, res.c_str(), strlen(res.c_str()), 0);
    }
}

//get countries list from server A or B
void get_countries(int server)
{

    // build UDP socket
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    //  permisson deny
    //int on = 1;
    //setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, &on, sizeof(on));
    if (udp_sock < 0)
    {
        perror("socket");
        exit(1);
    }

    // set address
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (server == 0)
    {
        serv_addr.sin_port = htons(PORTA);
    }
    else if (server == 1)
    {
        serv_addr.sin_port = htons(PORTB);
    }
    int len = sizeof(serv_addr);

    string send_buff = ("first_connection_" + to_string(server));
    char recv_buff[1000];

    int send_num = sendto(udp_sock, send_buff.c_str(), sizeof(send_buff), 0, (struct sockaddr *)&serv_addr, len);
    if (send_num < 0)
    {
        perror("sendto error in servermain!");
        exit(1);
    }

    int recv_num = recvfrom(udp_sock, recv_buff, sizeof(recv_buff), 0, (struct sockaddr *)&serv_addr, (socklen_t *)&len);
    if (recv_num < 0)
    {
        perror("recvfrom error in servermain!");
        exit(1);
    }
    recv_buff[recv_num] = '\0';

    //clear buffer area
    //memset(recv_buff 0, sizeof(recv_buff));

    string countries(recv_buff);

    vector<string> countries_list = split(countries, " ");
    //istringstream split(countries);
    //for (string c; getline(split, c, ' '); countries_list.push_back(c));

    //build contries' set
    for (string c : countries_list)
    {
        if (server == 0)
        {
            country_list_a.insert(c);
        }
        else if (server == 1)
        {
            country_list_b.insert(c);
        }
    }
    close(udp_sock);
}

// get recommendation result from corresponding server
string get_result(int flag, string country_name, string user_id)
{

    int clnt_udp_sock;
    struct sockaddr_in serv_addr;
    int buff_len = 200;

    clnt_udp_sock = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (flag == 0)
    {
        serv_addr.sin_port = htons(PORTA);
    }
    else if (flag == 1)
    {
        serv_addr.sin_port = htons(PORTB);
    }

    struct sockaddr *dst = (struct sockaddr *)&serv_addr;
    int fd = clnt_udp_sock;
    socklen_t len = sizeof(*dst);

    string sendbuf = country_name + " " + user_id;
    char buffer[buff_len];

    struct sockaddr_in src;
    sendto(fd, sendbuf.c_str(), buff_len, 0, dst, len);
    memset(buffer, 0, buff_len);
    recvfrom(fd, buffer, buff_len, 0, (struct sockaddr *)&src, &len);

    string res(buffer);
    return res;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <set>
#include <math.h>
#include <list>
#include <unordered_map>

#define PORTA 30997 //the UDP port of server A
#define UDPPORT 32997
#define BACKLOG 10 // how many pending connnections queue will be hold
#define LOCALHOST "127.0.0.1"
#define MAXDATASIZE 1000 // max number of bytes we can get at once

using namespace std;

int bind(int, const struct sockaddr *, socklen_t);
void build_graph();
string recommend(string, string);
unordered_map<string, unordered_map<int, set<int>>> country_map; // key is country_name, value is graph
set<string> country_set;
set<int> all_users;
int famous;

// adapt from beej tutorial page 39
// get sockaddr, IPv4 or IPv6

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

// split string str by delim
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
    cout << "The server A is up and running using UDP on port " << PORTA << endl;

    build_graph();

    // build socket
    int serv_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (serv_sock < 0)
    {
        perror("socket");
        exit(1);
    }

    // bind the socket with ip and port
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(PORTA);

    //  bind : Address already in use
    int option = 1;
    if (setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int)) == -1)
    {
        perror("setsockopt");
        exit(1);
    }

    int bind_num = ::bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (bind_num < 0)
    {
        perror("bind error:");
        exit(1);
    };

    char buff[1000];
    //UDP client
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);

    while (true)
    {
        string res = "";
        int n = recvfrom(serv_sock, buff, 1000, 0, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        if (n == 0)
        {
            printf("client close\n");
            break;
        }
        buff[n] = '\0';
        string req = buff;
        // get list of countries from backend server A and B
        if (req == "first_connection_0")
        {
            for (string coun : country_set)
            {
                res += coun;
                res += " ";
            }
            cout << "The server A has sent a country list to Main Server" << endl;
        }
        else
        {
            // extract country name and user ID from buff and return result.
            vector<string> info = split(req, " ");
            string country_name = info[0];
            string user_id = info[1];
            res = recommend(country_name, user_id);
        }
        sendto(serv_sock, res.c_str(), 1000, 0, (struct sockaddr *)&clnt_addr, clnt_addr_size);
    }

    close(serv_sock);
    return 0;
}

void build_graph()
{
    int count = -1;
    string country;

    //read data1.txt and build graph
    ifstream fin("data1.txt");
    string line;

    while (getline(fin, line))
    {
        vector<string> edges = split(line, " ");
        //country name in this line
        if (isdigit(edges[0][0]) == 0)
        {
            country = edges[0];
            country_set.insert(edges[0]);
            unordered_map<int, set<int>> graph;
            country_map[edges[0]] = graph;
            continue;
        }
        //userId in this line
        else
        {
            int id = stoi(edges[0]);
            all_users.insert(id);
            int curr_count = edges.size() - 1;
            // find the most popular user in this country
            if (curr_count > count)
            {
                count = curr_count;
                famous = id;
            }
            else if (curr_count == count && famous > id)
            {
                famous = id;
            }
            set<int> adj_list;
            for (int i = 1; i < edges.size(); i++)
            {
                string adj(edges[i]);
                if (adj.length() == 0)
                    continue;
                adj_list.insert(stoi(adj));
            }

            country_map[country][id] = adj_list;
        }
    }
}

string recommend(string country_name, string id)
{
    cout << "The server A has received request for finding possible friends of User" << id << " in " << country_name << endl;

    int user_id = stoi(id);
    if (all_users.count(user_id) == 0)
    {
        cout << user_id << " does not show up in " << country_name << endl;
        cout << "The server A has sent “User " << user_id << " not found” to Main Server" << endl;
        return "No_such_user";
    }
    //find result
    unordered_map<int, set<int>> curr_map = country_map[country_name]; //  get the map with current country

    cout << "The server A is searching possible friends for " << user_id << " ..." << endl;

    // check whether the input user has already connected with all users
    if (curr_map[user_id].size() + 1 == all_users.size())
    {
        return "all_connected";
    }

    unordered_map<int, set<int>>::iterator iter;
    iter = curr_map.begin();
    set<int> neighbors = curr_map[user_id]; // get the set of neighbors that are already connected with input user
    if (neighbors.size() == 0)
    {
        return to_string(famous);
    }

    int count = -1;
    int result = 0;
    int res;
    // count the most comman neighbors from uses that are not connecting to input user
    for (int user : all_users)
    {
        if (user == user_id)
            continue;
        set<int> neighbors1;
        int curr_count = 0; //  number of common neighbors
        if (neighbors.count(user) == 0)
        {
            neighbors1 = curr_map[user];
        }

        for (int user1 : neighbors1)
        {
            if (neighbors.count(user1) != 0)
            {
                curr_count++;
            }
        }
        //find a better result
        if (curr_count > count)
        {
            res = user;
            count = curr_count;
        }
        else if (curr_count == count && user < res)
        {
            res = user;
        }
    }

    result = (count == -1) ? famous : res;
    cout << "Here is the result: " << to_string(res) << " , count == " << count << endl;
    cout << "The server A has sent the result " << to_string(res) << " to Main Server" << endl;
    return to_string(result);
}
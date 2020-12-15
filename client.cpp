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

using namespace std;

#define TCPPORT 33997   // the port client will be connecting to
#define MAXDATASIZE 100 // max number of bytes we can get at once

int main()
{

    cout << "The client is up and running" << endl;
    //  create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    //  send a request to the server
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));           // Each byte is populated with a 0
    serv_addr.sin_family = AF_INET;                     // IPv4
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Specific IP address
    serv_addr.sin_port = htons(TCPPORT);                //   port#

    string country_name;
    string user_id;

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect ");
        exit(1);
    }

    while (true)
    {
        string sendbuf;
        cout << "Please enter the user ID: " << endl;
        cin >> user_id;
        sendbuf += user_id;
        sendbuf += " ";
        cout << "Please enter the Country Name: " << endl;
        cin >> country_name;
        sendbuf += country_name;
        cout << "Client1 has sent User" << user_id << " and " << country_name << " to Main Server using TCP" << endl;

        int len = strlen(sendbuf.c_str());
        if (send(sock, sendbuf.c_str(), len, 0) < 0)
        {
            perror("send ");
            exit(1);
        }
        //read data come from server
        char buffer[300];
        int n = read(sock, buffer, sizeof(buffer) - 1);
        buffer[n] = '\0';

        string res(buffer);

        if (strcmp(buffer, "No_such_user") == 0)
        {
            cout << "User " << user_id << " not found" << endl;
        }
        else if (strcmp(buffer, "No_such_country") == 0)
        {
            cout << "Country " << country_name << " not found" << endl;
        }
        else
        {
            cout << "Client1 has received results from Main Server: User " << res << " is possible friend of User " << user_id << " in " << country_name << endl;
        }
    }
    //  close socket
    close(sock);
    return 0;
}
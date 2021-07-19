#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "http.h"

char getRequest[1000] = {0};
char response[1000];
struct hostent *server;
struct sockaddr_in serveraddr;

int http_get_url(char* url)
{
    printf("GET %s\n", url);
    return 0;
}

int http_get(char* hostname, int port, char* request)
{
    sprintf(getRequest, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", request, hostname);

    server = gethostbyname(hostname);
    if (server == NULL)
    {
        printf("gethostbyname() failed\n");
        return 1;
    }
    else
    {
        // printf("\nserver - %s", server->h_addr_list[0]);
        printf("IP = ");
        unsigned int j = 0;
        while (server -> h_addr_list[j] != NULL)
        {
            printf("%d: %s ", j, inet_ntoa(*(struct in_addr*)(server -> h_addr_list[j])));
            j++;
        }
    }

    int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
     
    if (tcpSocket < 0)
    {
        printf("Error opening socket\n");
        return 2;
    }

    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
 
    bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
     
    serveraddr.sin_port = htons(port);
     
    if (connect(tcpSocket, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
    {
        printf("Error Connecting\n");
        close(tcpSocket);
        return 3;
    }

    if (send(tcpSocket, getRequest, strlen(getRequest), 0) < 0)
    {
        fprintf(stderr, "Error with send()");
        close(tcpSocket);
        return 4;
    }
             
    bzero(response, 1000);
     
    printf("-----RESPONSE----\n");
    
    recv(tcpSocket, response, 999, 0);
    
    printf("\n%s\n", response);
    
    printf("-----END-----\n");
     
    close(tcpSocket);

    return 0;
}

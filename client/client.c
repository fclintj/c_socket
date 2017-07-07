// client

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFSIZE 4096  

struct tcp_client {
    const char *address, *port;
    int sockfd;  
    struct addrinfo hints, *servinfo;
    int rv;
    char s[INET6_ADDRSTRLEN];
};

void *get_in_addr(struct sockaddr *sa);
void init_client(struct tcp_client *client);
void check_argv(int argc, char *argv[], struct tcp_client *client);
void recv_msg(int sockfd, char *buf);
void send_msg(int sockfd, char *buf);

int main(int argc, char *argv[]) {
    struct tcp_client client;
    check_argv(argc, argv, &client);
    init_client(&client);
    
    char recv_buff[BUFFSIZE];
    recv_msg(client.sockfd, recv_buff);
    printf("%s\n",recv_buff); 
    printf("Accepted\n");
    recv_msg(client.sockfd, recv_buff); 
    printf("%s\n",recv_buff); 
    close(client.sockfd);
    return 0;
}

void init_client(struct tcp_client *client){
    memset(&client->hints, 0, sizeof client->hints);
    client->hints.ai_family = AF_UNSPEC;
    client->hints.ai_socktype = SOCK_STREAM;

    if ((client->rv = getaddrinfo(client->address, client->port, &client->hints, &client->servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(client->rv));
        exit(1);
    }

    // loop through all the results and connect to the first we can
    struct addrinfo *p;
    for(p = client->servinfo; p != NULL; p = p->ai_next) {
        if ((client->sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(client->sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(client->sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        exit(2);
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            client->s, sizeof client->s);
    printf("Client: Connecting to %s\n", client->s);

    freeaddrinfo(client->servinfo); // all done with this structure
}

void check_argv(int argc, char *argv[], struct tcp_client *client) {
    if (argc != 3) {
        fprintf(stderr,"usage: client hostname/port\n");
        exit(1);
    }
    client->address = argv[1];
    client->port = argv[2];
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


void recv_msg(int sockfd, char *buf){
    int numbytes;
        if ((numbytes = recv(sockfd, buf, BUFFSIZE-1, 0)) == 0) {
            perror("recd");
            exit(1);
        }
    buf[numbytes]='\0';
}

void send_msg(int sockfd, char *message){
    int len = strlen(message);
    int x = strlen(message);
    printf("%d",x);
    send(sockfd, message, len, 0); 
}

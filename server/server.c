// server 
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

#define BACKLOG 10                      // incoming connections in queue
#define BUFFSIZE 4096

struct tcp_server {
    char *port; 
    int sockfd, new_fd;                 // socket file descriptor and new fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t in_addr_size;             // client's address size
    struct sigaction sa;                // handler to kill zombie 
    char s[INET6_ADDRSTRLEN];           // make longest string possible
    int addr_err;
};

int send_long(int s, char *buf);
int connect_to_client(struct tcp_server *server);
void sigchld_handler(int s);
void *get_in_addr(struct sockaddr *sa);
void init_server(struct tcp_server *server);  
void check_argv(int argc, char *argv[], struct tcp_server *server);
void send_msg(int sockfd, char *buf);
void recv_msg(int sockfd, char *buf);

int main(int argc, char *argv[]) {
    struct tcp_server server;
    check_argv(argc, argv, &server);
    init_server(&server);  

    char send_buff[BUFFSIZE] = "test";

    while(1) {
        if(connect_to_client(&server) == -1)
            continue;

        if (!fork()) { 
            close(server.sockfd); // socket file descriptor not needed

            send_long(server.new_fd, send_buff);

            close(server.new_fd);
            exit(0);
        }
        close(server.new_fd); 
    }

    return 0;
}

int send_long(int s, char *buf) {
    int len = strlen(buf);
    int total = 0;        // bytes sent
    int bytesleft = len; // bytes to send
    int n;

    while(total < len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    len = total; // return number actually sent here

    return (n == -1 ? -1 : 0); // return -1 on failure, 0 on success
} 

void sigchld_handler(int s) {
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    s++;
    errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void init_server(struct tcp_server *server) {
    memset(&server->hints, 0, sizeof server->hints);    // empty struct
    server->hints.ai_family = AF_UNSPEC;                // either ipv4 or ipv6
    server->hints.ai_socktype = SOCK_STREAM;            // tcp connection 
    server->hints.ai_flags = AI_PASSIVE;                // my IP address 

    if ((server->addr_err = getaddrinfo(NULL, server->port, &server->hints, &server->servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(server->addr_err));
        exit(1);
    }


    // loop through results and bind to the first
    struct addrinfo *p;
    for(p = server->servinfo; p != NULL; p = p->ai_next) {
        if ((server->sockfd = socket(p->ai_family, p->ai_socktype,
                        p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        int yes = 1;
        if (setsockopt(server->sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                    sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(server->sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(server->sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(server->servinfo);

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(server->sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    
    // reap dead processes
    server->sa.sa_handler = sigchld_handler; 
    sigemptyset(&server->sa.sa_mask);
    server->sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &server->sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

}

int connect_to_client(struct tcp_server *server) {
    server->in_addr_size = sizeof server->their_addr;
    server->new_fd = accept(server->sockfd, (struct sockaddr *)&server->their_addr, &server->in_addr_size);
    if (server->new_fd == -1) {
        perror("accept");

        return -1; 
    }

    inet_ntop(server->their_addr.ss_family,
            get_in_addr((struct sockaddr *)&server->their_addr),
            server->s, sizeof server->s);

    printf("server: got connection from %s\n", server->s);
    return 0; 
}

void check_argv(int argc, char *argv[], struct tcp_server *server) {
    if (argc != 2) {
        fprintf(stderr,"usage: client hostname/port\n");
        exit(1);
    }
    server->port = argv[1];
}

void send_msg(int sockfd, char *message){
    int len = strlen(message);
    int x = strlen(message);
    printf("%d",x);
    send(sockfd, message, len, 0); 
}

void recv_msg(int sockfd, char *buf){
    int numbytes;
        if ((numbytes = recv(sockfd, buf, BUFFSIZE-1, 0)) == 0) {
            perror("recd");
            exit(1);
        }
    buf[numbytes]='\0';
}



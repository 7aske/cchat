#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_S 1024
#define ADDR_S 32

// maximum number of clients
#define C_MAX 5

void panic(char*);
void* _handler(void*);
int start_server(int, struct sockaddr_in*);
int find_free_fd(int*);
void close_fd(int, int*);
void* _listener(void*);

static volatile __sig_atomic_t _running = 1;

static void handle_interupt(int);

int* active_socks;
pthread_t* threadpool;

int main(int argc, char* argv[])
{
    int server_sock, port;
    struct sockaddr_in server;
    char buf[BUF_S];
    char cmd;
    pthread_t listen_th;
    signal(SIGINT, handle_interupt);

    if (argc < 2) {
        printf("ERROR usage: <port>");
        exit(0);
    }

    bzero(&server, sizeof server);
    bzero(buf, BUF_S);

    port = atoi(argv[1]);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    // allocate memory to store active socket file descriptors
    active_socks = calloc(C_MAX, sizeof(int));

    // create server socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        panic("ERROR failed to create socket");
    }
    printf("SUCCESS socket created\n");

    // start server by binding the socket
    if (start_server(server_sock, &server) < 0) {
        panic("ERROR unable to start server");
    }
    printf("INFO server started on port %d\n", port);

    pthread_create(&listen_th, NULL, _listener, &server_sock);
    printf("main_socket 0x%x %d\n", &server_sock, server_sock);
    // request listening loop
    while (_running) {
        if ((cmd = getchar()) == 'q') {
            _running = 0;
        }
    }
    printf("INFO closing\n");
    // close(server_sock);
    shutdown(server_sock, SHUT_RDWR);
    pthread_join(listen_th, NULL);
    free(active_socks);
    printf("INFO closed\n");
    exit(0);
    return 0;
}
void* _listener(void* arg)
{
    int client_len, client_sock, current_sock;
    int* sock;
    char buf[BUF_S];
    pthread_t newthread = 0;
    struct sockaddr_in client;
    char addr[ADDR_S];

    bzero(&client, sizeof client);
    sock = (int*)arg;
    // threadpool = calloc(C_MAX, sizeof threadpool);
    // memset(threadpool, 0, sizeof threadpool);

    client_len = sizeof client;
    printf("_listener_socket 0x%x %d\n", sock, *sock);
    while (_running) {
        client_sock = accept(*sock, (struct sockaddr*)&client, (socklen_t*)&client);
        if (client_sock < 0) {
            printf("SOCKET SD - %d\n", client_sock);
            printf("ERROR failed to create client socket\n");
            break;
        }
        // parse client ip
        inet_ntop(AF_INET, &(client.sin_addr), addr, BUF_S);
        printf("INFO client connected ip: %s sock_fd: %d\n", addr, client_sock);

        current_sock = find_free_fd(active_socks);
        if (current_sock != -1) {
            active_socks[current_sock] = client_sock;

            for (int i = 0; i < C_MAX; i++) {
                printf("0x%x  fd: %d\n", &(active_socks[i]), active_socks[i]);
            }
            pthread_create(&newthread, NULL, _handler, &(active_socks[current_sock]));
            printf("INFO threadid %d\n", newthread);
        } else {
            strcpy(buf, "Server full :(");
            send(client_sock, buf, strlen(buf), 0);
            close(client_sock);
        }
    }
    for (int i = 0; i < C_MAX; i++) {
        int s = active_socks[i];
        if (s > 0) {
            printf("INFO sutting down sock_fd %d\n", s);
            shutdown(s, SHUT_RDWR);
        }
    }
    if (newthread != 0)
        pthread_join(newthread, NULL);
}
void* _handler(void* arg)
{
    int n;
    int* socket;
    char buf[BUF_S];
    memset(buf, 0, BUF_S);

    socket = (int*)arg;
    printf("_handler_socket 0x%x %d\n", socket, *socket);
    // s_data = (struct socket_data*)sockets;

    // listen for messages on the socket passed to the thread
    printf("INFO starting new thread for fd %d\n", *socket);
    while ((n = recv(*socket, buf, BUF_S, 0)) > 0) {
        printf("INFO %s\n", buf);

        // on sent message forward the message
        // to all active clients except to self
        for (int i = 0; i < C_MAX; i++) {
            if (active_socks[i] != *socket) {
                send(active_socks[i], buf, strlen(buf), 0);
                bzero(buf, BUF_S);
            }
        }
    }

    // on socket disconnects clear memory and free array spots
    if (n <= 0) {
        printf("INFO client disconnected - %d\n", *socket);
        close_fd(*socket, active_socks);
    }
}

// close file descriptor and refresh active socket
// array to allow for more clients
void close_fd(int fd, int* active_fds)
{
    for (int i = 0; i < C_MAX; active_fds++, i++) {
        if (fd == *active_fds) {
            printf("INFO closing fd %d\n", fd);
            close(fd);
            *active_fds = 0;
        }
        printf("0x%x %d\n", active_fds, *active_fds);
    }
}

// get the index of a free socket
int find_free_fd(int* active_fds)
{
    for (int i = 0; i < C_MAX; active_fds++, i++) {
        if (0 == *active_fds) {
            return i;
        }
    }
    return -1;
}

// initialize server by binding server socket to the address
int start_server(int sock, struct sockaddr_in* address)
{
    if (bind(sock, (struct sockaddr*)address, sizeof *address) < 0) {
        // panic("ERROR binding socket");
        return -1;
    }
    printf("SUCCESS socket bound\n");
    return listen(sock, C_MAX);
}

// print error and exit
void panic(char* msg)
{
    perror(msg);
    exit(0);
}
static void handle_interupt(int _)
{
    (void)_;
    _running = 0;
}

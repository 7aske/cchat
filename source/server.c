#include <netinet/in.h>
#include <arpa/inet.h>
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

void panic(char *);

void *_handler(void *);

int start_server(int, struct sockaddr_in *);

void *_listener(void *);

static volatile __sig_atomic_t _running = 1;

static void handle_interrupt(int);

typedef struct handler_data {
    pthread_t thread;
    int socket;
} handler_data_t;

void close_fdt(int *, handler_data_t *);

int find_free_fdt(handler_data_t *);

void clean_threads(handler_data_t *);

void shutdown_all(handler_data_t *);

handler_data_t *hd;

int main(int argc, char *argv[]) {
    int server_sock, port;
    struct sockaddr_in server;
    char buf[BUF_S];
    pthread_t listen_th;
    if (argc < 2) {
        printf("ERROR usage: <port>");
        exit(0);
    }
    signal(SIGINT, handle_interrupt);

    bzero(&server, sizeof server);
    bzero(buf, BUF_S);


    port = strtol(argv[1], NULL, 10);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        panic("ERROR failed to create socket");
    }
    printf("SUCCESS socket created\n");

    if (start_server(server_sock, &server) < 0) {
        panic("ERROR unable to start server");
    }
    printf("INFO server started on port %d\n", port);

    // allocate memory for threads and socket file descriptors
    hd = calloc(C_MAX, sizeof(handler_data_t));
    memset(hd, 0, C_MAX * sizeof(handler_data_t));
    pthread_create(&listen_th, NULL, _listener, &server_sock);

    // user input listening loop
    while (_running) {
        if ((getchar()) == 'q') {
            _running = 0;
        }
        // detach threads with cleared file descriptors
        clean_threads(hd);
    }
    printf("INFO closing\n");
    shutdown(server_sock, SHUT_RDWR);
    pthread_join(listen_th, NULL);
    free(hd);
    printf("INFO closed\n");
    exit(0);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat"

void *_listener(void *arg) {
    int client_sock, current_sock;
    int *sock;
    char buf[BUF_S];
    struct sockaddr_in client;
    char addr[ADDR_S];

    bzero(&client, sizeof client);
    sock = (int *) arg;

    // connection listening loop
    while (_running) {
        client_sock = accept(*sock, (struct sockaddr *) &client, (socklen_t *) &client);
        if (client_sock < 0) {
            printf("ERROR failed to create client socket\n");
            break;
        }
        // parse client ip
        inet_ntop(AF_INET, &(client.sin_addr), addr, BUF_S);
        printf("INFO client connected ip: %s sock_fd: %d\n", addr, client_sock);

        // get next available spot from memory
        current_sock = find_free_fdt(hd);
        if (current_sock != -1) {
            hd[current_sock].socket = client_sock;

            // create a thread that handles clients messages
            pthread_create(&(hd[current_sock].thread), NULL, _handler, &(hd[current_sock].socket));
            for (int i = 0; i < C_MAX; i++) {
                printf("SOCK - 0x%x  fd: %d\n", &hd[i].socket, hd[i].socket);
                printf("THRD - 0x%x  th: %d\n\n", &hd[i].thread, (int) hd[i].thread);
            }
        } else {

            strcpy(buf, "Server full :(");
            send(client_sock, buf, strlen(buf), 0);
            close(client_sock);
        }
    }
    // close all sockets and detach all threads
    shutdown_all(hd);
    return NULL;
}

#pragma clang diagnostic pop
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat"

void *_handler(void *arg) {
    int n;
    int *socket;
    char buf[BUF_S];
    memset(buf, 0, BUF_S);

    socket = (int *) arg;

    // listen for messages on the socket passed to the thread
    printf("INFO starting new thread for fd %d\n", *socket);
    while ((n = recv(*socket, buf, BUF_S, 0)) > 0) {
        printf("INFO %s\n", buf);
        if (n <= 0) {
            printf("ERROR bad rcv");
        }
        // on sent message forward the message
        // to all active clients except to self
        for (int i = 0; i < C_MAX; i++) {
            if (hd[i].socket != *socket) {
                send(hd[i].socket, buf, strlen(buf), 0);
                bzero(buf, BUF_S);
            }
        }
    }
    printf("INFO client disconnected - 0x%x\n", socket);
    close_fdt(socket, hd);
    return NULL;
}

#pragma clang diagnostic pop
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat"

void close_fdt(int *fd, handler_data_t *hdata) {
    for (int i = 0; i < C_MAX; hd++, i++) {
        if (fd == &(hd->socket)) {
            printf("INFO closing fd 0x%x and detaching thread 0x%x\n", fd, &hd->thread);
            shutdown(*fd, SHUT_RDWR);
            hd->socket = 0;
            pthread_detach(hd->thread);
            memset(&(hd->thread), 0, sizeof(pthread_t));
        }
        printf("SOCK: 0x%x fd: %d\n", &hdata->socket, hdata->socket);
        printf("THRD: 0x%x th: %d\n\n", &hdata->thread, (int) hdata->thread);
    }
}

#pragma clang diagnostic pop
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat"

void clean_threads(handler_data_t *hdata) {
    for (int i = 0; i < C_MAX; hdata++, i++) {
        if (hdata->socket == 0 && hdata->thread != 0) {
            printf("INFO detaching thread at 0x%x", &hdata->thread);
            pthread_detach(hdata->thread);
            memset(&(hdata->thread), 0, sizeof(pthread_t));
        }
    }
}

#pragma clang diagnostic pop
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat"

void shutdown_all(handler_data_t *hdata) {
    for (int i = 0; i < C_MAX; i++) {
        if (hdata[i].socket > 0) {
            printf("INFO shutting down sock_fd %d and thread 0x%x\n", hdata[i].socket, &hdata[i].thread);
            shutdown(hdata[i].socket, SHUT_RDWR);
        }
        if (hdata[i].thread > 0) {
            pthread_detach(hdata[i].thread);
            memset(&(hdata[i].thread), 0, sizeof(pthread_t));
        }
    }
}

#pragma clang diagnostic pop

int find_free_fdt(handler_data_t *hdata) {
    for (int i = 0; i < C_MAX; hdata++, i++) {
        if (hdata->socket == 0) {
            return i;
        }
    }
    return -1;
}

// initialize server by binding server socket to the address
int start_server(int sock, struct sockaddr_in *address) {
    if (bind(sock, (struct sockaddr *) address, sizeof *address) < 0) {
        return -1;
    }
    printf("SUCCESS socket bound\n");
    return listen(sock, C_MAX);
}

// print error and exit
void panic(char *msg) {
    perror(msg);
    exit(0);
}

// listener to catch CTRL+C (SIGINT)
static void handle_interrupt(int _) {
    (void) _;
    _running = 0;
}

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>

#define BUF_S 1024
#define NAME_S 32

static volatile __sig_atomic_t _running = 1;

static void _handle_interupt(int);

void panic(char*);
int get_rand_int();
void user_input(char*, int);
void prepend_username(char[], char[]);
void* _listener(void*);
int main(int argc, char* argv[])
{
    // random greeting messages
    char quitstr[] = "quit";
    char* greetings[] = {
        " is locked and loaded",
        " came prepared",
        " ordered vanilla latte",
        " is in a hurry"
    };
    int sock, n, port;
    struct sockaddr_in serv_addr;
    struct hostent* server;
    char buf[BUF_S];
    char name[NAME_S];

    pthread_t thread;

    if (argc != 3) {
        panic("ERROR usage: <server> <port>");
    }
    signal(SIGINT, _handle_interupt);

    bzero((char*)&serv_addr, sizeof(struct sockaddr_in));

    port = atoi(argv[2]);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        panic("ERROR failed to create socket");
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);

    if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
        panic("ERROR invalid server address");
    }

    // prompt user for his/her username
    printf("username>");
    user_input(name, NAME_S);

    n = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (n < 0) {
        panic("ERROR connecting to server");
    }

    // send a greeting message to server
    strcpy(buf, name);
    strcat(buf, greetings[get_rand_int() % 3]);
    n = send(sock, buf, strlen(buf), 0);
    if (n < 0) {
        printf("ERROR failed sending to server\n");
    }
    bzero(buf, BUF_S);

    // create a thread to listen to incoming messages
    n = pthread_create(&thread, NULL, _listener, &sock);
    if (n < 0) {
        panic("ERROR failed to create _listener thread");
    }

    // continiously prompt user for input to chat
    while (_running) {
        user_input(buf, BUF_S);
        if (strncmp(buf, quitstr, 4) == 0) {
            break;
        }
        // prepend username to message string
        prepend_username(buf, name);
        n = send(sock, buf, strlen(buf), 0);
        if (n < 0) {
            panic("ERROR sending to server");
        }
    }
    shutdown(sock, SHUT_RDWR);
    _running = 0;
    // pthread_detach(thread);

    pthread_join(thread, NULL);
    // pthread_exit(0);
    printf("exited\n");
}

// thread _listener started in start_routine()
void* _listener(void* data)
{
    int* sock = (int*)data;
    int n;

    char buf[BUF_S];

    // continiously listen for messages and print them to terminal
    while (_running) {
        n = recv(*sock, buf, BUF_S, 0);
        if (n < 0) {
            printf("ERROR receiving from server\n");
        }
        printf("\r>%s\n", buf);
        printf("\r>");
        bzero(buf, BUF_S);
    }
}

// short formatting of user input
void user_input(char* buf, int len)
{
    bzero(buf, len);
    fgets(buf, len, stdin);
    strtok(buf, "\n");
    printf(">");
}

// prepend username to message string
void prepend_username(char src[], char name[])
{
    char buf[BUF_S];
    bzero(buf, BUF_S);
    strcpy(buf, name);
    strcat(buf, ": ");
    strcat(buf, src);
    strcpy(src, buf);
}

// get random seeded integer
int get_rand_int()
{
    int seed = time(NULL);
    srand(seed);
    return rand();
}
static void _handle_interupt(int _)
{
    (void)_;
    _running = 0;
}
// print message and exit
void panic(char* msg)
{
    perror(msg);
    exit(1);
}

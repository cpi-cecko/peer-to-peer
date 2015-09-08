#include "../lib/unp.h"

#define PORT_MIN 11000
#define PORT_MAX 11010

#define END_SIGNAL "am-end"

/*
 * Homework:
 * rfc6335
 *
 * Notes:
 * 64-bitness, protocol independence
 */
void listener_task(int listen_port)
{
    int listenfd;
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(listen_port);

    Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

    Listen(listenfd, LISTENQ);

    printf("LISTENER: Bound to port [%d]\n", listen_port);

    while (1) {
        int connfd = Accept(listenfd, (SA *) NULL, NULL);

        int n;
        char message[1024];
        while ( (n = read(connfd, message, sizeof(message))) > 0 ) {
            message[n] = 0;
            printf("%s", message);
        }

        Close(connfd);
    }
}

/*
 * This is a P2P, LAN-based chat.
 * It starts listening on a predefined address, on a well-defined port.
 * It tries to bind on a list of addresses and returns -1 if it can't.
 * Each program instance is associated with a unique user id.
 */
int main(int argc, char *argv[])
{
    if (argc != 3)
        err_quit("usage: lan_chat <listen-port> <user-name>");

    int listen_port;
    listen_port = atoi(argv[1]);

    if (listen_port < PORT_MIN || listen_port > PORT_MAX)
        err_quit("Ports must be in range [%d:%d]", PORT_MIN, PORT_MAX);

    if (strlen(argv[2]) > 10)
        err_quit("The user_name must be at most 10 characters");

    /* 
     * The listener thread would bind to a port and print incoming messages
     * on screen.
     */
    int listenerpid;
    if ( (listenerpid = Fork()) == 0 ) {
        printf("MAIN: Spawning listener\n");
        listener_task(listen_port);
    }

    /* The main thread would connect to a peer and send user input */
    int sockfd;
    sockfd = Socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    /* 
     * Connect on localhost and try a list of ports.
     * Later, try connecting on several LAN addresses.
     */
    servaddr.sin_family = AF_INET;
    Inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);

    int current_port = PORT_MIN;
    do {
        servaddr.sin_port = htons(current_port);
        // printf("MAIN: Trying to connect to [%d]\n", current_port);
        current_port++;
        if (current_port > PORT_MAX) {
            current_port = PORT_MIN;
        } else if (current_port == listen_port) {
            current_port++;
        }
    } while (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0);
    
    printf("MAIN: Connected [%d]\n", current_port);

    char *user_name = argv[2];
    int name_len = strlen(user_name);
    while (1) {
        char message[1024];
        if (!fgets(message, sizeof(message), stdin) ||
                strncmp(message, END_SIGNAL, strlen(END_SIGNAL)) == 0)
            break;

        int msg_len = strlen(message);

        char *to_send = malloc(name_len + 2 + msg_len + 1);
        memcpy(to_send, user_name, name_len);
        memcpy(to_send + name_len, ": ", 2);
        memcpy(to_send + name_len + 2, &message, msg_len + 1);

        Write(sockfd, to_send, strlen(to_send));

        free(to_send);
    }

    exit(0);
}

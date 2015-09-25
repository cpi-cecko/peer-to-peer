/*
 * This version only permits chatting with a single peer on the same machine.
 * It finds peers by ports on localhost.
 * It uses a simple pkt-line-based protocol.
 * It doesn't check whether the port to which we try to bind is being used.
 *
 * Usage: lan_chat-v1 <listen-port> <user-id>
 */

/*
 * TODO:
 */
#include "../lib/unp.h"
#include "../lib/p2p.h"

#define PORT_MIN 11000
#define PORT_MAX 11010

#define END_SIGNAL "am-end"


int spawn_listener(int);
int connect_to_listener(int);
void message_loop(char*, int);

int main(int argc, char **argv)
{
    if (argc != 3)
        err_quit("usage: lan_chat <listen-port> <user-name>");

    int listen_port;
    listen_port = atoi(argv[1]);
    if (listen_port < PORT_MIN || listen_port > PORT_MAX)
        err_quit("Ports must be in range [%d:%d]", PORT_MIN, PORT_MAX);

    char *user_name;
    user_name = argv[2];
    if (strlen(user_name) > 10)
        err_quit("The user_name must be at most 10 characters");

    
    /*
     * Spawns a listener process which accepts any connection and outputs each
     * received message on the terminal.
     */
    int listenerpid = spawn_listener(listen_port);

    /*
     * Connects to a listener and waits for user input. On each new line input
     * by user, creates a message according to the protocol and sends it over
     * the socket.
     */
    int sockfd = connect_to_listener(listen_port);
    message_loop(user_name, sockfd);


    kill(listenerpid, SIGKILL);
    exit(0);
}


void listener_task(int);

int spawn_listener(int listen_port)
{
    int listenerpid;
    if ( (listenerpid = Fork()) == 0 ) {
        listener_task(listen_port);
        exit(0);
    }

    return listenerpid;
}


int find_peer(int, int);

int connect_to_listener(int listen_port)
{
    int sockfd;
    sockfd = Socket(AF_INET, SOCK_STREAM, 0);

    /*
     * This is the most crucial part of the peer-to-peer chat.
     *
     * The problem is, that in order to chat with someone, we must first find
     * him. In server-side solutions, we would contact a central server, bound
     * on a well-known port, and ask it to route each message we type to the
     * peer we are interested in.
     *
     * When we have no centralized entity, however, we must use the peer
     * infrastructure to send our messages to the desired location. One of the
     * simplest situations in which we can be is that peers are on the same
     * host, only bound on different ports. If we know the range of the ports,
     * we can iterate over it and try connecting to every (ip, port) pair until
     * success.
     *
     * Even though this situation is somewhat artificial it is simple-enough
     * for educational purposes and it enables us to test our system locally.
     */
    find_peer(sockfd, listen_port);

    return sockfd;
}


void message_loop(char *user_name, int sockfd)
{
    char message[1024];
    while (fgets(message, sizeof(message), stdin) &&
            strncmp(message, END_SIGNAL, strlen(END_SIGNAL)) != 0) {
        char *to_send = create_send_msg(message, user_name);

        Writen(sockfd, to_send, strlen(to_send));

        free(to_send);
    }
}


int bind_listener(int);

void listener_task(int listen_port)
{
    int listenfd = bind_listener(listen_port);

    while (1) {
        int connfd = Accept(listenfd, (SA *) NULL, NULL);

        char msg_len_hex[4];
        while (read(connfd, msg_len_hex, sizeof(msg_len_hex)) > 0) {
            unsigned int msg_len = hex_to_int(msg_len_hex);
            msg_len -= 5;
            char *message = malloc(msg_len);

            Readn(connfd, message, msg_len);
            message[msg_len] = 0;
            printf("%s\n", message);

            free(message);
        }

        Close(connfd);
    }
}

int find_peer(int sockfd, int listen_port)
{
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    Inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);

    int current_port = PORT_MIN;
    do {
        if (current_port > PORT_MAX)
            current_port = PORT_MIN;

        if (current_port == listen_port)
            current_port++;

        servaddr.sin_port = htons(current_port);
        current_port++;
    } while (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0);

    --current_port;
    return current_port;
}


int bind_listener(int listen_port)
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

    return listenfd;
}

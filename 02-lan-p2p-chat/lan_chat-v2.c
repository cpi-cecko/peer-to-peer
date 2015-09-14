/*
 * This version implements chatting with peers on the same subnet.
 * The subnet address is passed as an argument. We iterate over the
 * possible IPs in the subnet /24 and try to connect to each one.
 *
 * Usage: lan_chat-v1 <listen-port> <user-id>
 */

/*
 * TODO:
 * 1. Try to make it work with non-byte-bounded subnets.
 * 2. Try to make it work with IPv6 and be protocol-independent.
 */
#include "../lib/unp.h"

#define CHAT_PORT 11000

#define END_SIGNAL "am-end"


int spawn_listener(int);
int connect_to_listener(char*);
void message_loop(const char*, int);

int main(int argc, char **argv)
{
    if (argc != 3)
        err_quit("usage: lan_chat <subnet-address> <user-name>");

    char *subnet_address;
    subnet_address = argv[1];
    if (!inet_aton(subnet_address, NULL))
        err_quit("The subnet address must be a valid IPv4 address");

    char *user_name;
    user_name = argv[2];
    if (strlen(user_name) > 10)
        err_quit("The user_name must be at most 10 characters");

    
    /*
     * Spawns a listener process which accepts any connection and outputs each
     * received message on the terminal.
     */
    int listenerpid = spawn_listener(CHAT_PORT);

    /*
     * Connects to a listener and waits for user input. On each new line input
     * by user, creates a message according to the protocol and sends it over
     * the socket.
     */
    int sockfd = connect_to_listener(subnet_address);
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


int find_peer(int, char*);

int connect_to_listener(char *subnet_address)
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
    find_peer(sockfd, subnet_address);

    return sockfd;
}


char* create_send_msg(char*, const char*);

void message_loop(const char *user_name, int sockfd)
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

int find_peer(int sockfd, char *subnet_address)
{
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(CHAT_PORT);
    Inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);

    do {
    } while (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0);

    return -1;
}

char* create_send_msg(char *message, const char *user_name)
{
    chomp(message);

    int to_send_len = 4 + strlen(user_name) + 
                      2 + strlen(message) +
                      1;
    char hex_len[5];
    int_to_hex_4(to_send_len, hex_len);

    char *to_send = malloc(to_send_len);
    snprintf(to_send, to_send_len, "%s%s: %s", hex_len, user_name, message);

    return to_send;
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

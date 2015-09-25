/*
 * This version implements chatting with peers on the same subnet.
 * The subnet address is passed as an argument. We iterate over the
 * possible IPs in the subnet /24 and try to connect to each one.
 *
 * Usage: lan_chat-v2 <subnet-address> <user-name> <start-idx>
 */

/*
 * TODO:
 * 1. Try to make it work with non-byte-bounded subnets.
 * 2. Try to make it work with IPv6 and be protocol-independent.
 * 3. We don't have any authentication currently. Think and implement a clever
 *  solution of this problem.
 */
#include "../lib/unp.h"
#include "../lib/p2p.h"

#define CHAT_PORT 11000

#define END_SIGNAL "am-end"


static char *subnet_address;
static char *user_name;
static int start_idx;


int spawn_listener(int);
int connect_to_listener();
void message_loop(int);

int main(int argc, char **argv)
{
    if (argc < 3)
        err_quit("usage: lan_chat <subnet-address> <user-name> <start-idx>");

    subnet_address = argv[1];
    if (!inet_aton(subnet_address, NULL))
        err_quit("The subnet address must be a valid IPv4 address");

    user_name = argv[2];
    if (strlen(user_name) > 10)
        err_quit("The user_name must be at most 10 characters");

    if (argc == 4)
        start_idx = atoi(argv[3]);
    
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
    int sockfd = connect_to_listener();
    message_loop(sockfd);


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


int find_peer(int);

int connect_to_listener()
{
    int sockfd;
    sockfd = Socket(AF_INET, SOCK_STREAM, 0);

    /*
     * Now, we'll use a different algorithm to find our peers on a LAN.
     *
     * We will iterate over all the hosts in a given subnet, try to connect to
     * each one on a given port, and if the connection is successful, start 
     * chatting with them.
     */
    find_peer(sockfd);

    return sockfd;
}


void message_loop(int sockfd)
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

        // TODO: Put message parsing in function
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

int find_peer(int sockfd)
{
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(CHAT_PORT);

    /* Remove the last octet from the address */
    char *dot = strrchr(subnet_address, '.');
    dot[1] = 0;

    int i = start_idx;
    char try_address[strlen(subnet_address) + 4];
    do {
        snprintf(try_address, sizeof(try_address), "%s%d", subnet_address, i);
        printf("Trying %s\n", try_address);
        ++i;

        Inet_pton(AF_INET, try_address, &servaddr.sin_addr);
    } while (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0);

    printf("Bound!\n");

    return -1;
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

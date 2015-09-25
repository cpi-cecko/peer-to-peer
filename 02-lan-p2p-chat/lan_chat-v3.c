/*
 * This version implements chatting with peers on the same subnet.
 * The subnet address is passed as an argument. The peer-finding algorithm is
 * the same as in lan_chat-v2, except that now we're using UDP.
 *
 * Usage: lan_chat-v3 <subnet-address> <user-name>
 */

/*
 * TODO:
 * 1. listener_task is protocol-dependent. Remedy this issue.
 * 2. The peer can easily bind to its own address and port pair. Modify 
 *    find_peer to avoid this.
 * 3. Can we use UDP to find our peers and TCP to send our messages to them
 *    after that? What will be the use of this?
 * 4. Can we do the opposite?
 */
#include "../lib/unp.h"
#include "../lib/p2p.h"

#define CHAT_PORT 11001

#define END_SIGNAL "am-end"


struct sockaddr_in found_peer;
int start_idx;


int spawn_listener(int);
int connect_to_listener(char*);
void message_loop(const char*, int);

int main(int argc, char **argv)
{
    if (argc < 3)
        err_quit("usage: lan_chat <subnet-address> <user-name> <start-idx>");

    char *subnet_address;
    subnet_address = argv[1];
    if (!inet_aton(subnet_address, NULL))
        err_quit("The subnet address must be a valid IPv4 address");

    char *user_name;
    user_name = argv[2];
    if (strlen(user_name) > 10)
        err_quit("The user_name must be at most 10 characters");

    start_idx = 80;
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


struct sockaddr_in find_peer(int, char*);

int connect_to_listener(char *subnet_address)
{
    int sockfd;
    sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

    /*
     * Now, we'll use a different algorithm to find our peers on a LAN.
     *
     * We will iterate over all the hosts in a given subnet, try to connect to
     * one on a given port, and if the connection is successful, start 
     * chatting with them.
     */
    found_peer = find_peer(sockfd, subnet_address);

    return sockfd;
}


void message_loop(const char *user_name, int sockfd)
{
    char message[1024];
    while (fgets(message, sizeof(message), stdin) &&
            strncmp(message, END_SIGNAL, strlen(END_SIGNAL)) != 0) {
        char *to_send = create_send_msg(message, user_name);

        Sendto(sockfd, to_send, strlen(to_send), 0,
            (SA *) &found_peer, sizeof(found_peer));

        free(to_send);
    }
}

int bind_listener(int);

/*
 * TODO: It's not an orthogonal design: If I modify some part of the code to
 *       send a protocol message, the response must be handled here.
 */
void listener_task(int listen_port)
{
    int listenfd;
    struct sockaddr_in peeraddr;
    socklen_t peeraddr_len;

    listenfd = bind_listener(listen_port);
    peeraddr_len = sizeof(peeraddr);

    while (1) {
        char *message = 
            recv_message(listenfd, (SA *) &peeraddr, &peeraddr_len);

        message += 4; /* Skip length */

        if (strncmp(message, AUTH_CAN, strlen(AUTH_CAN)) == 0) {
            printf("Received AUTH_CAN\n");

            auth_accept(listenfd, (SA *) &peeraddr, peeraddr_len);

            printf("Sent auth accept\n");
        } else {
            printf("%s\n", message);
        }

        message -= 4; /* Unskip length */

        free(message);
    }
}


void update_try_address(char*, size_t, char*, int, struct sockaddr_in*);

struct sockaddr_in find_peer(int sockfd, char *subnet_address)
{
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    Setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in servaddr;
    socklen_t servaddr_len = sizeof(servaddr);
    bzero(&servaddr, servaddr_len);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(CHAT_PORT);

    /* Remove the last octet from the address */
    char *dot = strrchr(subnet_address, '.');
    dot[1] = 0;

    int i = start_idx;
    char try_address[strlen(subnet_address) + 4];
    int is_conn = 0;
    do {
        update_try_address(try_address, sizeof(try_address),
            subnet_address, i, &servaddr);
        printf("Trying %s\n", try_address);

        auth_request(sockfd, (SA *) &servaddr, servaddr_len);
        printf("Sent auth request\n");

        is_conn = auth_try_confirm(sockfd, (SA *) &servaddr, &servaddr_len);

        ++i;
    } while (!is_conn);

    printf("Bound!\n");

    return servaddr;
}

void update_try_address(char *try_address, size_t try_address_len,
                        char *subnet_address, int host, 
                        struct sockaddr_in *servaddr)
{
    snprintf(try_address, try_address_len, "%s%d", subnet_address, host);

    Inet_pton(AF_INET, try_address, &servaddr->sin_addr);
}


int bind_listener(int listen_port)
{
    int listenfd;
    listenfd = Socket(AF_INET, SOCK_DGRAM, 0);

    int opt = 1;
    Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
        (const char*)&opt, sizeof(opt));

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(listen_port);

    Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

    return listenfd;
}

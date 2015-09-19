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
 */
#include "../lib/unp.h"

#define CHAT_PORT 11001

#define END_SIGNAL "am-end"

#define AUTH_CAN "auth: CAN"
#define AUTH_OFC "auth: OFC"


struct sockaddr_in found_peer;


int spawn_listener(int);
int connect_to_listener(char*);
void message_loop(const char*, int);

int start_idx;

int main(int argc, char **argv)
{
    if (argc < 3)
        err_quit("usage: lan_chat <subnet-address> <user-name>");

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
     * each one on a given port, and if the connection is successful, start 
     * chatting with them.
     */
    found_peer = find_peer(sockfd, subnet_address);

    return sockfd;
}


void create_send_msg_static(const char*, char*);
char* create_send_msg(char*, const char*);

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
void auth_accept(int, SA*, socklen_t);
char* recv_message(int, SA*, socklen_t*);

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
            auth_accept(listenfd, (SA *) &peeraddr, peeraddr_len);
        } else {
            printf("%s\n", message);
        }

        message -= 4; /* Unskip length */

        free(message);
    }
}

void auth_accept(int sockfd, SA *peeraddr, socklen_t peeraddr_len)
{
    printf("Received AUTH_CAN\n");

    char auth_ofc[4 + strlen(AUTH_OFC) + 1];
    create_send_msg_static(AUTH_OFC, auth_ofc);
    unsigned int auth_ofc_len = strlen(auth_ofc);

    Sendto(sockfd, auth_ofc, auth_ofc_len, 0, peeraddr, peeraddr_len);

    printf("Sent %s\n", auth_ofc);
}

char* recv_message(int sockfd, 
                   SA *peeraddr, socklen_t *peeraddr_len)
{
    /*
     * Read the message length, without discarding the message.
     */
    char msg_len_hex[4];
    Recvfrom(sockfd, msg_len_hex, sizeof(msg_len_hex), MSG_PEEK,
        peeraddr, peeraddr_len);
    unsigned int msg_len = hex_to_int(msg_len_hex);

    char *message = malloc(msg_len);
    Recvfrom(sockfd, message, msg_len, 0, peeraddr, peeraddr_len);
    message[msg_len - 1] = 0;

    return message;
}

struct sockaddr_in find_peer(int sockfd, char *subnet_address)
{
    struct timeval tv;
    tv.tv_sec = 3;
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
        snprintf(try_address, sizeof(try_address), "%s%d", subnet_address, i);
        printf("Trying %s\n", try_address);
        ++i;

        Inet_pton(AF_INET, try_address, &servaddr.sin_addr);

        char auth_msg[4 + strlen(AUTH_CAN) + 1];
        create_send_msg_static(AUTH_CAN, auth_msg);
        Sendto(sockfd, auth_msg, strlen(auth_msg), 0, (SA *) &servaddr, servaddr_len);
        printf("Sent %s\n", auth_msg);

        char msg[4 + strlen(AUTH_OFC) + 1];
        if (recvfrom(sockfd, msg, sizeof(msg), 0, (SA *) &servaddr, &servaddr_len) > 0 &&
                strncmp(&msg[4], AUTH_OFC, strlen(AUTH_OFC)) == 0) {
            is_conn = 1;
        }
    } while (!is_conn);

    printf("Bound!\n");

    return servaddr;
}

void create_send_msg_static(const char *message, char *to_send)
{
    int to_send_len = 4 + strlen(message) + 1;
    char hex_len[5];
    int_to_hex_4(to_send_len, hex_len);

    snprintf(to_send, to_send_len, "%s%s", hex_len, message);
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
    listenfd = Socket(AF_INET, SOCK_DGRAM, 0);

    int opt = 1;
    Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(listen_port);

    Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

    return listenfd;
}

/*
 * This version uses UDP broadcast to find peers to whom we can chat to. It
 * sends a broadcast ``auth: CAN'' and creates a list of peers who have
 * responded with ``auth: OFC''. It then starts a group chat with them using
 * unicast UDP packets.
 *
 * It uses bind_address to disable self-reception of requests. This only works
 * for singlehomed hosts, though.
 *
 * Usage: lan_chat-v4 <broadcast-address> <user-name> <bind-address>
 */

/*
 * TODO:
 */
#include "../lib/unp.h"
#include <net/if.h>

#define CHAT_PORT 11001

#define CMD_END "am-end"
#define CMD_FIND "am-find"

#define AUTH_CAN "auth: CAN"
#define AUTH_OFC "auth: OFC"

#define MAX_PEERS 255


static int peer_count;
static struct sockaddr_in peers[MAX_PEERS];

static char *user_name;
static char *bind_addr;
static char *broadcast_address;

/* static char *iface; */


int connect_to_listener();
void message_loop(int);

/*
 * We're going to use I/O multiplexing here.
 * Handle the problem with stdio and select.
 */
int main(int argc, char **argv)
{
    if (argc < 4)
        err_quit("usage: lan_chat <broadcast-address> <user-name> <bind-address>");

    broadcast_address = argv[1];
    if (!inet_aton(broadcast_address, NULL))
        err_quit("The broadcast address must be a valid IPv4 address");

    user_name = argv[2];
    if (strlen(user_name) > 10)
        err_quit("The user_name must be at most 10 characters");

    bind_addr = argv[3];
    if (!inet_aton(bind_addr, NULL))
        err_quit("The bind address must be a valid IPv4 address");

    /*
    if (argc == 5)
        iface = argv[4];
        */
    
    /*
     * Find a group of peers to which we can chat to.
     */
    int sockfd = connect_to_listener();
    /*
     * Open a listening socket and do a `select` on it and on stdin. Handle the
     * listening socket like `listener_task` used to and handle the input like
     * before.
     */
    message_loop(sockfd);


    exit(0);
}


void find_peer(int);

int connect_to_listener()
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
    find_peer(sockfd);

    return sockfd;
}


char* create_send_msg(char*, const char*);
int bind_listener(int);
void auth_request(int, const SA*, socklen_t);
int auth_try_confirm(int, SA*, socklen_t*);
void auth_accept(int, const SA*, socklen_t);
char* recv_message(int, SA*, socklen_t*);

void message_loop(int sockfd)
{
    int listenfd;
    struct sockaddr_in peeraddr;
    socklen_t peeraddr_len;

    listenfd = bind_listener(CHAT_PORT);
    peeraddr_len = sizeof(peeraddr);

    fd_set rset;
    int maxfd = listenfd;
    FD_ZERO(&rset);

    printf("In message_loop\n");

    for ( ; ; ) {
        FD_SET(listenfd, &rset);
        FD_SET(fileno(stdin), &rset);
        Select(maxfd + 1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(listenfd, &rset)) { /* Received data */
            printf("Has data\n");
            char *message = 
                recv_message(listenfd, (SA *) &peeraddr, &peeraddr_len);

            if (bind_addr && 
                    strncmp(bind_addr,
                            Sock_ntop((SA *) &peeraddr, sizeof(peeraddr)),
                            strlen(bind_addr)) != 0) {
                printf("Received data\n");

                message += 4; /* Skip length */

                if (strncmp(message, AUTH_CAN, strlen(AUTH_CAN)) == 0) {
                    printf("Received AUTH_CAN\n");

                    auth_accept(listenfd, (const SA *) &peeraddr, peeraddr_len);
                    peeraddr.sin_port = htons(CHAT_PORT);
                    peers[peer_count++] = peeraddr;

                    printf("Sent auth accept\n");
                } else {
                    printf("%s\n", message);
                }

                message -= 4; /* Unskip length */
            }

            free(message);
        }

        if (FD_ISSET(fileno(stdin), &rset)) { /* Collect input for sending */
            char message[1024];
            /* TODO: handle Read errors */
            if (Read(fileno(stdin), message, sizeof(message))) {
                if (strncmp(message, CMD_END, strlen(CMD_END)) == 0) {
                    break;
                } else if (strncmp(message, CMD_FIND, strlen(CMD_FIND)) == 0) {
                    struct sockaddr_in reqaddr;
                    reqaddr.sin_family = AF_INET;
                    reqaddr.sin_port = htons(CHAT_PORT);
                    Inet_pton(AF_INET, broadcast_address, &reqaddr.sin_addr);

                    socklen_t reqaddr_len = sizeof(reqaddr);

                    auth_request(sockfd, (SA *) &reqaddr, reqaddr_len);
                    const int is_confirm =
                        auth_try_confirm(sockfd, (SA *) &reqaddr, &reqaddr_len);
                    if (is_confirm > 0) {
                        peers[peer_count++] = reqaddr;
                    } else {
                        err_sys("rcvfrom error");
                    }
                } else {
                    char *to_send = create_send_msg(message, user_name);

                    int i;
                    for (i = 0; i < peer_count; ++i) {
                        printf("Sending to %s\n", Sock_ntop((SA *) &peers[i], sizeof(peers[i])));
                        Sendto(sockfd, to_send, strlen(to_send), 0,
                                (SA *) &peers[i], sizeof(peers[i]));
                    }

                    free(to_send);
                }
            }
        }
    }
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

void create_send_msg_static(const char *message, char *to_send)
{
    int to_send_len = 4 + strlen(message) + 1;
    char hex_len[5];
    int_to_hex_4(to_send_len, hex_len);

    snprintf(to_send, to_send_len, "%s%s", hex_len, message);
}

/*
 * When a peer sends `auth: CAN' it asks for communication with another peer.
 * The other peer must send back `auth: OFC' to accept the connection.
 */
void auth_accept(int sockfd, const SA *peeraddr, socklen_t peeraddr_len)
{
    char auth_ofc[4 + strlen(AUTH_OFC) + 1];
    create_send_msg_static(AUTH_OFC, auth_ofc);

    Sendto(sockfd, auth_ofc, strlen(auth_ofc), 0, peeraddr, peeraddr_len);
}

char* recv_message(int sockfd, 
                   SA *peeraddr, socklen_t *peeraddr_len)
{
    /* Read the message length, without discarding the message. */
    char msg_len_hex[4];
    Recvfrom(sockfd, msg_len_hex, sizeof(msg_len_hex), MSG_PEEK,
        peeraddr, peeraddr_len);
    unsigned int msg_len = hex_to_int(msg_len_hex);

    /* Read the whole message, including the length */
    char *message = malloc(msg_len);
    Recvfrom(sockfd, message, msg_len, 0, peeraddr, peeraddr_len);
    message[msg_len - 1] = 0;

    return message;
}


static void finish_find(int);

void find_peer(int sockfd)
{
    struct sockaddr_in servaddr;
    socklen_t servaddr_len = sizeof(servaddr);
    bzero(&servaddr, servaddr_len);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(CHAT_PORT);
    Inet_pton(AF_INET, broadcast_address, &servaddr.sin_addr);

    const int on = 1;
    Setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));

    sigset_t sigset_alrm;
    Sigemptyset(&sigset_alrm);
    Sigaddset(&sigset_alrm, SIGALRM);

    Signal(SIGALRM, finish_find);

    struct sockaddr_in peeraddr;
    socklen_t peeraddr_len = sizeof(peeraddr);

    /*
     * Send auth_request and gather all responses which were received in the
     * next 5 seconds. These are the peers to which you can chat.
     */
    auth_request(sockfd, (const SA *) &servaddr, servaddr_len);

    /* TODO: fix the race condition by SIGALRM */
    alarm(5);
    for ( ; ; ) {
        const int is_confirm = 
            auth_try_confirm(sockfd, (SA *) &peeraddr, &peeraddr_len);
        if (is_confirm > 0) {
            /* Add the address to the array of peers */
            peers[peer_count++] = peeraddr;
        } else if (is_confirm == -1) {
            /* EINTR received */
            break;
        } else {
            err_sys("rcvfrom error");
        }
    }

    if (peer_count > 0) {
        printf("Found %d peers\n", peer_count);
    } else {
        printf("No one found\n");
    }
}

static void finish_find(int signo)
{
    return;
}

void auth_request(int sockfd, const SA *servaddr, socklen_t servaddr_len)
{
    char auth_msg[4 + strlen(AUTH_CAN) + 1];
    create_send_msg_static(AUTH_CAN, auth_msg);
    Sendto(sockfd, auth_msg, strlen(auth_msg), 0, servaddr, servaddr_len);
}

int auth_try_confirm(int sockfd, SA *servaddr, socklen_t *servaddr_len)
{
    char msg[4 + strlen(AUTH_OFC) + 1];
    int n = recvfrom(sockfd, msg, sizeof(msg), 0, servaddr, servaddr_len);
    if (n < 0) {
        if (errno == EINTR)
            return -1;
        else
            return 0;
    } else if (strncmp(&msg[4], AUTH_OFC, strlen(AUTH_OFC)) == 0) {
        return 1;
    }

    return 0;
}


int bind_listener(int listen_port)
{
    int listenfd;
    listenfd = Socket(AF_INET, SOCK_DGRAM, 0);

    int opt = 1;
    Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
        (const char*)&opt, sizeof(opt));

    /* We bind to interface in order not to receive broadcasted data on the lo
     * interface.
     */
    /*
    if (iface) {
        struct ifreq interface;
        strcpy(interface.ifr_ifrn.ifrn_name, iface);
        Setsockopt(listenfd, SOL_SOCKET, SO_BINDTODEVICE, 
            &interface, sizeof(interface));
    }
    */

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    /*
    if (bind_addr) {
        Inet_pton(AF_INET, bind_addr, &servaddr.sin_addr);
        printf("gonna bind to [%s]\n", bind_addr);
    }
    else
    */
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(listen_port);

    Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

    return listenfd;
}

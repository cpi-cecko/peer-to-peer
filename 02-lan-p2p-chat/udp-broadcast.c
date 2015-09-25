#include "unp.h"
#include "p2p.h"

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

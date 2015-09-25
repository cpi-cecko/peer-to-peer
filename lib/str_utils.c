#include "unp.h"
#include "p2p.h"


void int_to_hex_4(int num, char *hex)
{
    snprintf(hex, 5, "%04x", num);
    hex[4] = 0;
}

unsigned int hex_to_int(char *hex)
{
    unsigned int dec = 0;
    sscanf(hex, "%x", &dec);

    return dec;
}

void chomp(char *str)
{
    str[strcspn(str, "\r\n")] = 0;
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

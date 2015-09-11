#include "unp.h"


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

/*
 * This function checks if subnet_address is of the form x.x.x.x/y
 */
int is_valid_subnet_address(char *subnet_address)
{
}

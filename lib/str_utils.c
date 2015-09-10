#include "unp.h"


void int_to_hex_4(int num, char *hex)
{
    snprintf(hex, 4, "%04x", num);
    hex[4] = 0;
}

int hex_to_int(char *hex)
{
    int dec;
    sscanf(hex, "%x", dec);

    return dec;
}

#include "../lib/unp.h"

#define BEGIN_TEST \
    int main() \
    {

#define TEST(expr, msg) \
        if (!(expr)) { \
            printf("Test [%s] failed: %s\n", #expr, #msg); \
        } else { \
            printf("Test [%s] succeeded!\n", #expr); \
        }

#define END_TEST \
        return 0; \
    }


BEGIN_TEST

    TEST(!is_valid_subnet_address("128.0.1/13"),
            "No 4th octet");
    TEST(!is_valid_subnet_address(".0.1.3/10"),
            "Begins with dot");
    TEST(!is_valid_subnet_address("...."),
            "Only dots");
    TEST(!is_valid_subnet_address("/10"),
            "Only mask");
    TEST(!is_valid_subnet_address("10.10.13.53"),
            "Only address");
    TEST(!is_valid_subnet_address("256.14.15.300"),
            "Only address, above 255");
    TEST(!is_valid_subnet_address("13.13.13.10/35"),
            "Mask above 32");
    TEST(!is_valid_subnet_address("13.13.11.1/"),
            "Address, only slash");
    TEST(!is_valid_subnet_address("0.1.2"),
            "No mask, no 4 octets");
    TEST(!is_valid_subnet_address("/"),
            "Only slash");
    TEST(!is_valid_subnet_address(""),
            "Empty address");
    TEST(!is_valid_subnet_address("10"),
            "Only one number, no mask");
    TEST(!is_valid_subnet_address("10/25"),
            "Only one number, with mask");
    TEST(!is_valid_subnet_address("10.66"),
            "Only two numbers, no mask");
    TEST(!is_valid_subnet_address("10.66/22"),
            "Only two numbers, with mask");

END_TEST

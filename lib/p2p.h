#ifndef	__p2p_h
#define	__p2p_h


#define AUTH_CAN "auth: CAN"
#define AUTH_OFC "auth: OFC"


void int_to_hex_4(int, char*);
unsigned int hex_to_int(char*);
void chomp(char*);

char* create_send_msg(char*, const char*);
void create_send_msg_static(const char*, char*);
void auth_request(int, const SA*, socklen_t);
int auth_try_confirm(int, SA*, socklen_t*);
void auth_accept(int, const SA*, socklen_t);
char* recv_message(int, SA*, socklen_t*);


#endif	/* __unp_h */

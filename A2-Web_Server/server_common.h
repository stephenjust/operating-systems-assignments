#ifndef _H_SERVER_COMMON
#define _H_SERVER_COMMON

#include <pthread.h>

#define HTTP_200 HTTP/1.1 200 OK\n
#define HTTP_400 HTTP/1.1 400 Bad Request\n
#define HTTP_403 HTTP/1.1 403 Forbidden\n
#define HTTP_404 HTTP/1.1 404 Not Found\n
#define HTTP_500 HTTP/1.1 500 Internal Server Error\n

#define HTTP_CT Content-Type: text/html\n

typedef struct {
	char *path;
	unsigned int response_code;
	struct sockaddr_in *sockaddr;
	char request_line[1024];
	char resp_string[256];
	int content_length;
} request_t;

extern char* webroot;
extern pthread_mutex_t log_lock;

void usage();
void kidhandler(int signum);
void sighandler_setup();
void date_string(char* buffer, size_t buf_size);
void ip_addr_string(struct sockaddr_in* sock, char* buffer, size_t buf_size); 
void req_path_string(request_t *req, char* buffer, int buffer_len);
int bind_socket(struct sockaddr_in sockname, u_short port);
request_t parse_request(int sd);
int get_err_text(int resp_code, char* buffer, int buffer_len);
void send_headers(int sd, request_t *req);
char *get_response(request_t *req, int *req_len);
void log_request(request_t *req, char* logfile);
ssize_t send_response(int sd, char *buffer);
void do_request(int sd, struct sockaddr_in *client, char* logfile);

#endif

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "server_common.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))

char *webroot;
pthread_mutex_t log_lock;

void usage() {
	extern char * __progname;
	fprintf(stderr, "usage: %s portnumber webroot logfile\n", __progname);
	exit(1);
}

void kidhandler(int signum) {
	/* signal handler for SIGCHLD */
	waitpid(WAIT_ANY, NULL, WNOHANG);
}

void sighandler_setup() {
	struct sigaction sa;
	/*
	 * first, let's make sure we can have children without leaving
	 * zombies around when they die - we can do this by catching
	 * SIGCHLD.
	 */
	sa.sa_handler = kidhandler;
	sigemptyset(&sa.sa_mask);
	/*
	 * we want to allow system calls like accept to be restarted if they
	 * get interrupted by a SIGCHLD
	 */
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
		err(1, "sigaction failed");
}

void date_string(char* buffer, size_t buf_size) {
	time_t now = time(NULL);
	struct tm *t = gmtime(&now);
	strftime(buffer, buf_size, "%a %d %b %Y %H:%M:%S %Z", t);
}

void ip_addr_string(struct sockaddr_in* sock, char* buffer, size_t buf_size) {
	char *addr = inet_ntoa(sock->sin_addr);
	snprintf(buffer, buf_size, "%s", addr);
}

void req_path_string(request_t *req, char* buffer, int buffer_len) {
	snprintf(buffer, buffer_len, "%s/%s", webroot, req->path);
}

int bind_socket(struct sockaddr_in sockname, u_short port) {
	int sd;

	memset(&sockname, 0, sizeof(sockname));
	sockname.sin_family = AF_INET;
	sockname.sin_port = htons(port);
	sockname.sin_addr.s_addr = htonl(INADDR_ANY);
	sd=socket(AF_INET,SOCK_STREAM,0);
	if ( sd == -1)
		err(1, "socket failed");

	if (bind(sd, (struct sockaddr *) &sockname, sizeof(sockname)) == -1)
		err(1, "bind failed");

	if (listen(sd,3) == -1)
		err(1, "listen failed");

	return sd;
}

request_t parse_request(int sd) {
	char request[1024];
	char req_ln[256];
	request_t req;
	int start = 0;
	char *first_nl;
	char *token;
	char *path;

	strcpy(req.request_line, "--");
	strcpy(req.resp_string, "200 OK");
	req.response_code = 200;
	req.content_length = 0;

	/* Get request */
	if (read(sd, request, sizeof(request)) == -1) {
		fprintf(stderr, "Failed to read request\n");
		req.response_code = 500;
		return req;
	}
	
	/* Get first line of request */
	first_nl = strstr(request, "\n");
	strncpy(req_ln, request, min(sizeof(req_ln), (size_t)first_nl - (size_t)request));
	req_ln[first_nl - request] = '\0';
	/* remove carriage return if present */
	if (req_ln[first_nl - request - 1] == '\r')
		req_ln[first_nl - request - 1] = '\0';

	token = strtok(request, " ");
	start += strlen(token) + 1;
	if (strcmp("GET", token) != 0) {
		fprintf(stderr, "Invalid request: Missing GET token.\n");
		req.response_code = 400;
		return req;
	}
	path = strtok(NULL, " ");
	start += strlen(path) + 1;
	token = strtok(NULL, "\n");
	start += strlen(token) + 1;
	if (strncmp("HTTP/1.1", token, 8) != 0) {
		fprintf(stderr, "Invalid request: Missing HTTP/1.1 token. Got %s\n", token);
		req.response_code = 400;
		return req;
	}
	if (strstr(&request[start], "\n\n") == NULL && strstr(&request[start], "\r\n\r\n") == NULL) {
		fprintf(stderr, "Invalid request: Missing blank line.\n");
		req.response_code = 400;
		return req;
	}
	strncpy(req.request_line, req_ln, sizeof(req.request_line));
	req.path = path;
	return req;
}

int get_err_text(int resp_code, char* buffer, int buffer_len) {
	if (resp_code == 400) {
		strncpy(buffer, "<html><body>\n"
				"<h2>Malformed Request</h2>\n"
				"Your browser sent a request I could not understand.\n"
				"</body></html>\n", buffer_len);
	} else if (resp_code == 404) {
		strncpy(buffer, "<html><body>\n"
				"<h2>Document not found</h2>\n"
				"You asked for a document that doesn't exist. That is so sad.\n"
				"</body></html>\n", buffer_len);
	} else if (resp_code == 403) {
		strncpy(buffer, "<html><body>\n"
				"<h2>Permission Denied</h2>\n"
				"You asked for a document you are not permitted to see. "
				"It sucks to be you.\n"
				"</body></html>\n", buffer_len);
	} else if (resp_code == 500) {
		strncpy(buffer, "<html><body>\n"
				"<h2>Oops. That Didn't work</h2>\n"
				"I had some sort of problem dealing with your request. "
				"Sorry, I'm lame.\n"
				"</body></html>\n", buffer_len);
	} else {
		strncpy(buffer, "", buffer_len);
	}
	return strlen(buffer);
}

char *get_response(request_t *req, int* req_len) {
	struct stat s;
	char *buffer;
	char path_buffer[1024];
	int fp;

	if (req->response_code == 400) {
		strcpy(req->resp_string, "400");
		strcpy(req->request_line, "--");
		buffer = malloc(1024);
		*req_len = get_err_text(req->response_code, buffer, 1024);
		return buffer;
	}
	req_path_string(req, path_buffer, sizeof(path_buffer));
	if (stat(path_buffer, &s) == -1 || S_ISDIR(s.st_mode)) {
		if (errno == ENOENT) {
			req->response_code = 404;
			strcpy(req->resp_string, "404");
		} else if (errno == EACCES) {
			req->response_code = 403;
			strcpy(req->resp_string, "403");
		}
		if (S_ISDIR(s.st_mode)) {
			req->response_code = 403;
			strcpy(req->resp_string, "403");
		}
		buffer = malloc(1024);
		*req_len = get_err_text(req->response_code, buffer, 1024);
		return buffer;
	}
	
	if ((fp = open(path_buffer, O_RDONLY)) == -1) {
		req->response_code = 403;
		strcpy(req->resp_string, "403");
		buffer = malloc(1024);
		*req_len = get_err_text(req->response_code, buffer, 1024);
		return buffer;
	}
	buffer = malloc(s.st_size);
	read(fp, buffer, s.st_size);
	*req_len = s.st_size;
	return buffer;
}

void log_request(request_t *req, char* logfile) {
	int fd;
	char buffer[1024];
	char date_buffer[80];
	char ip_buffer[40];
	date_string(date_buffer, sizeof(date_buffer));
	ip_addr_string(req->sockaddr, ip_buffer, sizeof(ip_buffer));
	snprintf(buffer, sizeof(buffer),
			 "%s\t%s\t%s\t%s\n",
			 date_buffer, ip_buffer, req->request_line, req->resp_string);
	printf("%s\n", req->request_line);
	pthread_mutex_lock(&log_lock);
	if ((fd = open(logfile, O_WRONLY | O_APPEND | O_CREAT, 0644)) == -1) {
		printf("Failed to open log file for writing.\n");
	}
	if (write(fd, buffer, strlen(buffer)) == -1) {
		if (errno == EAGAIN) {
			printf("EAGAIN\n");
		} else if (errno == EWOULDBLOCK) {
			printf("EWOULDBLOCK\n");
		} else if (errno == EBADF) {
			printf("EBADF\n");
		}
		printf("Failed to write log file.\n");
	}
	close(fd);
	pthread_mutex_unlock(&log_lock);
}

void send_headers(int sd, request_t *req) {
	char date_buf[80];
	char buffer[1024];
	if (req->response_code == 200) {
		send_response(sd, "HTTP/1.1 200 OK\n");
	} else if (req->response_code == 400) {
		send_response(sd, "HTTP/1.1 400 Bad Request\n");
	} else if (req->response_code == 404) {
		send_response(sd, "HTTP/1.1 404 Not Found\n");
	} else if (req->response_code == 403) {
		send_response(sd, "HTTP/1.1 403 Forbidden\n");
	} else {
		send_response(sd, "HTTP/1.1 500 Internal Server Error\n");
	}
	date_string(date_buf, sizeof(date_buf));
    snprintf(buffer, sizeof(buffer), "Date: %s\n", date_buf);
	send_response(sd, buffer);
	send_response(sd, "Content-Type: text/html\n");
	snprintf(buffer, sizeof(buffer), "Content-Length: %d\n", req->content_length);
	send_response(sd, buffer);
	send_response(sd, "\n");
}

ssize_t send_response(int sd, char *buffer) {
	ssize_t written, w;
	/*
	 * write the message to the client, being sure to
	 * handle a short write, or being interrupted by
	 * a signal before we could write anything.
	 */
	w = 0;
	written = 0;
	while (written < strlen(buffer)) {
		w = write(sd, buffer + written,
		    strlen(buffer) - written);
		if (w == -1) {
			if (errno != EINTR)
				err(1, "write failed");
		}
		else
			written += w;
	}
	return written;
}

void do_request(int clientsd, struct sockaddr_in * client, char* logfile) {
	char* response;
	request_t req;
	int req_len;
	int written;

	req = parse_request(clientsd);
	response = get_response(&req, &req_len);
	req.content_length = req_len;
	send_headers(clientsd, &req);
	written = send_response(clientsd, response);
	free(response);
	req.sockaddr = client;
	if (req.response_code == 200) {
		sprintf(req.resp_string, "200 OK %d/%d", written, req.content_length);
	}
	log_request(&req, logfile);
}

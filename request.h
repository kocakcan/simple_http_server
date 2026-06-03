#ifndef REQUEST_H
#define REQUEST_H

#define MAX_HEADERS		32
#define MAX_HEADER_NAME		64
#define MAX_HEADER_VALUE	256
#define MAX_METHOD		8
#define MAX_PATH		256
#define MAX_VERSION		16
#define BUF_SIZE		4096

struct http_header {
	char name[MAX_HEADER_NAME];
	char value[MAX_HEADER_VALUE];
};

struct http_request {
	char method[MAX_METHOD];
	char path[MAX_PATH];
	char version[MAX_VERSION];
	struct http_header headers[MAX_HEADERS];
	int header_count;
};

int parse_request(const char *raw, struct http_request *req);

#endif	/* REQUEST_H */

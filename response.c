#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "response.h"

const char *status_text(int status) {
	switch (status) {
		case 200: return "OK";
		case 201: return "Created";
		case 204: return "No Content";
		case 400: return "Bad Request";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 500: return "Internal Server Error";
		default:  return "Unknown";
	}
}

int response_init(struct http_response *res,
		int status,
		const char *content_type,
		const char *body) {
	memset(res, 0, sizeof(*res));
	res->status = status;
	snprintf(res->content_type, sizeof(res->content_type),
			"%s", content_type ? content_type : "text/plain");
	if (body) {
		size_t len = strlen(body);
		if (len >= MAX_BODY_SIZE) return -1;
		memcpy(res->body, body, len);
		res->body_len = len;
	}
	return 0;
}

int response_send(int fd, const struct http_response *res) {
	char header_buf[512];
	int header_len = snprintf(header_buf, sizeof(header_buf),
		"HTTP/1.1 %d %s\r\n"
		"Content-Type: %s\r\n"
		"Content-Length: %zu\r\n"
		"Connection: close\r\n"
		"\r\n",
		res->status,
		status_text(res->status),
		res->content_type,
		res->body_len);
	if (header_len < 0) return -1;
	if (write(fd, header_buf, header_len) < 0) return -1;
	if (res->body_len > 0)
		if (write(fd, res->body, res->body_len) < 0) return -1;
	return 0;
}

int send_response(int fd, int status, const char *content_type, 
		const char *body) {
	struct http_response res;
	if (response_init(&res, status, content_type, body) < 0) return -1;
	return response_send(fd, &res);
}

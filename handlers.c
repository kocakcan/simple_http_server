#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "handlers.h"
#include "response.h"

void handle_root(int client_fd, const struct http_request *req) {
	(void)req;
	send_response(client_fd, 200, "text/plain", "Hello, World!\n");
}

void handle_about(int client_fd, const struct http_request *req) {
	(void)req;
	send_response(client_fd, 200, "text/plain", "Simple HTTP Server\n");
}

void handle_echo(int client_fd, const struct http_request *req) {
	size_t content_length = 0;

	for (int i = 0; i < req->header_count; ++i) {
		if (strcasecmp(req->headers[i].name, "Content-Length") == 0) {
			content_length = (size_t)atoi(req->headers[i].value);
			break;
		}
	}
	char msg[128];
	snprintf(msg, sizeof(msg),
		"Echo: received %zu bytes (full body reading in M5)\n",
		content_length);
	send_response(client_fd, 200, "text/plain", msg);
}

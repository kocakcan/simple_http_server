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
	if (req->body_len == 0) {
		send_response(client_fd, 200, "text/plain", "Echo: (empty"
			" body)\n");
		return;
	}
	char *body_copy = malloc(req->body_len + 1);
	if (!body_copy) {
		send_response(client_fd, 500, "text/plain", "500 Internal "
				" Server Error\n");
		return;
	}
	memcpy(body_copy, req->body, req->body_len);
	body_copy[req->body_len] = '\0';
	send_response(client_fd, 200, "text/plain", body_copy);
	free(body_copy);
}

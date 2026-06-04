#ifndef HANDLERS_H
#define HANDLERS_H

#include "request.h"

void handle_root(int client_fd, const struct http_request *req);
void handle_about(int client_fd, const struct http_request *req);
void handle_echo(int client_fd, const struct http_request *req);

#endif /* HANDLERS_H */

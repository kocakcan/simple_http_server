#ifndef ROUTER_H
#define ROUTER_H

#include "request.h"

/* a handler is just a function that receives the parsed request and client fd,
 * and is responsible for sending a response. */
typedef void (*handler_fn)(int client_fd, const struct http_request *req);

struct route {
	const char *method;
	const char *path;
	handler_fn handler;
};

/* router_dispatch: walk the route and call the matching handler. sends 404 / 4
 * 05 automatically if no match is found. */
void router_dispatch(int client_fd, const struct http_request *req,
		const struct route *routes, int route_count);

#endif /* ROUTER_H */

#include <string.h>
#include "router.h"
#include "response.h"

void router_dispatch(int client_fd, const struct http_request *req,
		const struct route *routes, int route_count) {
	int path_matched = 0;

	for (int i = 0; i < route_count; ++i) {
		if (strcmp(routes[i].path, req->path) != 0)
			continue;
		path_matched = 1;
		if (strcmp(routes[i].method, req->method) != 0)
			continue;
		routes[i].handler(client_fd, req);	/* exact match */
		return;
	}
	if (path_matched)
		send_response(
				client_fd, 
				405, 
				"text/plain", 
				"405 Method Not Allowed");
	else
		send_response(
				client_fd,
				404,
				"text/plain",
				"404 Not Found");
}

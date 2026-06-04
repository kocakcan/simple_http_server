#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "request.h"
#include "response.h"
#include "router.h"
#include "handlers.h"

#define PORT	7878
#define BACKLOG	10

static const struct route routes[] = {	/* route table */
	{ "GET", "/",		handle_root  },
	{ "GET", "/about",	handle_about },
	{ "POST", "/echo",	handle_echo  },
};

#define ROUTE_COUNT (int)(sizeof(routes) / sizeof(routes[0]))

int main(void) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) { perror("socket"); exit(EXIT_FAILURE); }

	int yes = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
		sizeof(yes)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	struct sockaddr_in addr = {0};
	addr.sin_family		= AF_INET;
	addr.sin_port		= htons(PORT);
	addr.sin_addr.s_addr	= htonl(INADDR_LOOPBACK);
	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	if (listen(sockfd, BACKLOG) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
	printf("Server listening on http://127.0.0.1:%d\n", PORT);
	while (1) {
		int client_fd = accept(sockfd, NULL, NULL);
		if (client_fd < 0) { perror("accept"); continue; }
		char buf[BUF_SIZE];
		ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
		if (n < 0) {
			perror("read");
			close(client_fd);
			continue;
		}
		if (n == 0) {
			close(client_fd);
			continue;
		}
		buf[n] = '\0';
		struct http_request req;
		if (parse_request(buf, &req) < 0) {
			fprintf(stderr, "Malformed request, sending 400\n");
			send_response(client_fd, 400, "text/plain", 
					"400 Bad Request");
			close(client_fd);
			continue;
		}
		printf("%s %s %s (%d headers)\n",
			req.method, req.path, req.version, req.header_count);
		router_dispatch(client_fd, &req, routes, ROUTE_COUNT);
		close(client_fd);
	}
	close(sockfd);
	return 0;
}

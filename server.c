#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT	7878
#define BACKLOG	10

int main() {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) { perror("socket"); exit(EXIT_FAILURE); }

	int yes = 1;	/* Allow immediate rebinding after restart */
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(yes)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	if (listen(sockfd, 10) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	printf("Server listening on http://127.0.0.1:%d\n", PORT);

	while (1) {
		int client_fd = accept(sockfd, NULL, NULL);
		if (client_fd < 0) { perror("accept"); continue; }
		printf("Got a connection! (fd=%d)\n", client_fd);

		char buf[4096];		/* read the request */
		ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
		if (n < 0) {
			perror("read");
			close(client_fd);
			continue;
		}
		buf[n] = '\0';
		printf("#---Request (%zd bytes)---#\n%s#---End---#\n", n, buf);
		const char *response =	/* build and send the response */
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: 13\r\n"
			"Connection: close\r\n"
			"\r\n"
			"Hello, World!\n";
		ssize_t sent = write(client_fd, response, strlen(response));
		if (sent < 0)
			perror("write");
		close(client_fd);
	}
	close(sockfd);			/* unreachable, but good practice */
	return 0;
}

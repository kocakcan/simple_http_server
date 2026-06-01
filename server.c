#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main() {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) { perror("socket"); exit(1); }

	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(7878);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		exit(1);
	}

	if (listen(sockfd, 10) < 0) {
		perror("listen");
		exit(1);
	}

	while (1) {
		int client_fd = accept(sockfd, NULL, NULL);
		if (client_fd < 0) { perror("accept"); continue; }
		printf("Got a connection!\n");
		close(client_fd);
	}
}

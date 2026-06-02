#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT			7878
#define BACKLOG			10
#define BUF_SIZE		4096
#define MAX_HEADERS		32
#define MAX_HEADER_NAME		64
#define MAX_HEADER_VALUE	256
#define MAX_METHOD		8
#define MAX_PATH		256
#define MAX_VERSION		16

struct http_header {
	char name[MAX_HEADER_NAME];
	char value[MAX_HEADER_VALUE];
};

struct http_request {
	char method[MAX_METHOD];			/* "GET", "POST", etc. */
	char path[MAX_PATH];				/* "/", "/hello/world" */
	char version[MAX_VERSION];			/* "HTTP/1.1" */
	struct http_header headers[MAX_HEADERS];
	int header_count;
};

int parse_request(const char *raw, struct http_request *req) {
	memset(req, 0, sizeof(*req));

	const char *line_end = strstr(raw, "\r\n");
	if (!line_end) return -1;	/* malformed: no CRLF */

	const char *first_space = strchr(raw, ' ');
	if (!first_space || first_space >= line_end) return -1;

	const char *second_space = strchr(first_space + 1, ' ');
	if (!second_space || second_space >= line_end) return -1;

	size_t method_len = first_space - raw;
	if (method_len >= MAX_METHOD) return -1;
	memcpy(req->method, raw, method_len);
	req->method[method_len] = '\0';

	size_t path_len = second_space - (first_space + 1);
	if (path_len >= MAX_PATH) return -1;
	memcpy(req->path, first_space + 1, path_len);
	req->path[path_len] = '\0';

	size_t version_len = line_end - (second_space + 1);
	if (version_len >= MAX_VERSION) return -1;
	memcpy(req->version, second_space + 1, version_len);
	req->version[version_len] = '\0';

	const char *cursor = line_end + 2;	/* skip past "\r\n" */

	while (1) {
		const char *next_crlf = strstr(cursor, "\r\n");
		if (!next_crlf) return -1;	/* malformed: no CRLF */
		if (next_crlf == cursor) break;	/* empty line (end of headers */
		/* too many headers */
		if (req->header_count >= MAX_HEADERS) return -1;
		const char *colon = memchr(cursor, ':', next_crlf - cursor);
		if (!colon) return -1;		/* malformed: header has no colon */

		size_t name_len = colon - cursor;	/* extract name */
		if (name_len >= MAX_HEADER_NAME) return -1;
		struct http_header *h = &req->headers[req->header_count];
		memcpy(h->name, cursor, name_len);
		h->name[name_len] = '\0';

		const char *value_start = colon + 1;
		while (value_start < next_crlf && *value_start == ' ')
			value_start++;

		size_t value_len = next_crlf - value_start;
		if (value_len >= MAX_HEADER_VALUE) return -1;
		memcpy(h->value, value_start, value_len);
		h->value[value_len] = '\0';
		req->header_count++;
		cursor = next_crlf + 2;		/* skip past "\r\n" */
	}
	return 0;
}

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

		char buf[BUF_SIZE];
		ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
		if (n < 0) {
			perror("read");
			close(client_fd);
			continue;
		}
		buf[n] = '\0';

		struct http_request req;
		if (parse_request(buf, &req) < 0) {
			fprintf(stderr, "Malformed request, sending 400\n");
			const char *bad =
				"HTTP/1.1 400 Bad Request\r\n"
				"Content-Length: 0\r\n"
				"Connection: close\r\n"
				"\r\n";
			write(client_fd, bad, strlen(bad));
			close(client_fd);
			continue;
		}

		printf("Method: %s | Path: %s | Version: %s | Headers: %d\n",
				req.method, req.path, req.version, req.header_count);
		for (int i = 0; i < req.header_count; ++i) {
			printf("  [%s] = [%s]\n",
			req.headers[i].name, req.headers[i].value);
		}
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

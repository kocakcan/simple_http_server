#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "read_request.h"

/* parse_content_length: find Content-Length value in raw headers. returns 0 if
 * header is absent (no body expected). */
static size_t parse_content_length(const char *raw, size_t header_end) {
	char *copy = malloc(header_end + 1);
	if (!copy) return 0;
	memcpy(copy, raw, header_end);
	copy[header_end] = '\0';

	char *p = copy;
	while (*p) {
		if (strncasecmp(p, "content-length:", 15) == 0) {
			size_t val = (size_t)atoi(p + 15);
			free(copy);
			return val;
		}
		p++;
	}
	free(copy);
	return 0;
}

/* read_full_request: reads from fd in a loop until:
 * 	1. we have seen \r\n\r\n (end of headers)
 * 	2. we have read Content-Length bytes of body after that
 * returns heap-allocated null-terminated buffer and NULL on any error. */
char *read_full_request(int fd) {
	size_t	capacity 	= BUF_SIZE;
	size_t	total	 	= 0;
	char	*buf	 	= malloc(capacity);
	if (!buf) return NULL;
	size_t 	header_end	= 0;	/* offset just past \r\n\r\n */
	size_t	content_length	= 0;	/* value from Content-Length header */

	while (1) {
		if (total >= capacity - 1) {
			size_t new_cap = capacity * 2;
			if (new_cap > MAX_RAW_SIZE) {
				fprintf(stderr, "read_full_request: request too
						large\n");
				free(buf);
				return NULL;
			}
			char *tmp = realloc(buf, new_cap);
			if (!tmp) { free(buf); return NULL; }
			buf = tmp;
			capacity = new_cap;
		}
		ssize_t n = read(fd, buf + total, capacity - total - 1);
		if (n < 0) { perror("read"); free(buf); return NULL; }
		if (n == 0) break;
		total += n;
		buf[total] = '\0';
		if (header_end == 0) {
			char *end = strstr(buf, "\r\n\r\n");
			if (end) {
				/* +4 past \r\n\r\n */
				header_end = (size_t)(end - buf) + 4;
				content_length = parse_content_length(
						buf, header_end);
				if (content_length > MAX_BODY_SIZE) {
					fprintf(stderr,
						"read_full_request: body too large"
						"(%zu)\n", content_length);
					free(buf);
					return NULL;
				}
			}
		}
		if (header_end > 0 && total >= header_end + content_length)
			break;
	}
	return buf;
}

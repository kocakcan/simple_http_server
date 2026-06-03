#include <string.h>
#include "request.h"

int parse_request(const char *raw, struct http_request *req) {
	memset(req, 0, sizeof(*req));

	const char *line_end = strstr(raw, "\r\n");
	if (!line_end) return -1;

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

	const char *cursor = line_end + 2;

	while (1) {
		const char *next_crlf = strstr(cursor, "\r\n");
		if (!next_crlf) return -1;
		if (next_crlf == cursor) break;
		if (req->header_count >= MAX_HEADERS) return -1;

		const char *colon = memchr(cursor, ':', next_crlf - cursor);
		if (!colon) return -1;

		size_t name_len = colon - cursor;
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
		cursor = next_crlf + 2;
	}
	return 0;
}

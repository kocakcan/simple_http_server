#ifndef RESPONSE_H
#define RESPONSE_H

#include <stddef.h>

#define MAX_STATUS_TEXT		64
#define MAX_CONTENT_TYPE	64
#define MAX_BODY_SIZE		4096

struct http_response {
	int	status;
	char	content_type[MAX_CONTENT_TYPE];
	char	body[MAX_BODY_SIZE];
	size_t	body_len;
};

/* response_init: populate a response struct. body may be NULL for bodyless res
 * ponses. returns -1 if body exceeds MAX_BODY_SIZE. */
int response_init(struct http_response *res,
		int status,
		const char *content_type,
		const char *body);

/* response_send: serialize the response struct and write it to fd. returns -1 
 * on write error. */
int response_send(int fd, const struct http_response *res);

/* send_response: build + send in one call for convenience */
int send_response(int fd, 
		int status, 
		const char *content_type,
		const char *body);

/* status_text: map a numeric code to its reason phrase. */
const char *status_text(int status);

#endif /* RESPONSE_H */

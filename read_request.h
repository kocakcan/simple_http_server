#ifndef READ_REQUEST_H
#define READ_REQUEST_H

#include "request.h"

#define MAX_BODY_SIZE	(1024 * 1024)			/* 1 MB body cap */
#define RAW_BODY_SIZE	(MAX_BODY_SIZE + BUF_SIZE)	/* headers + body */

/* read_full_request: reads a full http request from fd into a dynamically allo
 * cated buffer. handles partial reads and reads until Content-Length is staisf
 * ied. Returns a heap-allocated null-terminated string on success and NULL on 
 * error. */
char *read_full_request(int fd);

#endif /* READ_REQUEST_H */

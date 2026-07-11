#ifndef _LIBCLAW_H_
#define _LIBCLAW_H_

typedef struct {
    int   socket_fd;
    int   status_code;
    char* text;
} req_t;

extern int init_request(req_t *req, char* url);
extern void get_request(req_t *req, char* url, char* headers);
extern void post_request(req_t *req, char* headers, char* data);
#endif

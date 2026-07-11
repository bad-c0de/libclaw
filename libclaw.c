#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "libclaw.h"
#define BUFFER_SIZE 4096

typedef struct {
    char*   hostname;
    int     port;
    char*   path;
    bool    is_https;
    struct  addrinfo *res;
    SSL_CTX *ctx;
    SSL     *ssl;
} opts_t;

opts_t opts;

void parse_url   (char* url);
void parse_status(req_t *req, char* response);
void parse_text  (req_t *req, char* response);

void http_request (req_t *req, char* request, char** response);
void https_request(req_t *req, char* request, char** response);

int init_request(req_t *req, char* url)
{
    int socket_fd, status;
    char* port = malloc(6 * sizeof(char));
    struct addrinfo hints, *res;

    SSL_CTX *ctx;
    SSL *ssl;
    
    // Initialize SSL
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    const SSL_METHOD *method = TLS_client_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) { ERR_print_errors_fp(stderr); return -1; }

    // Parse the URL
    parse_url(url);
    sprintf(port, "%d", opts.port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    status = getaddrinfo(opts.hostname, port, &hints, &res);
    if (status < 0) { fprintf(stderr, "[!] getaddrinfo Error: %s\n", gai_strerror(status)) ;return -1; }
    
    // Create the Socket
    socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (socket_fd < 0) { perror("[!] Failed to create socket") ; return 1; }
    
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, socket_fd);
    SSL_set_tlsext_host_name(ssl, opts.hostname);

    req->socket_fd = socket_fd; // Store the Socket for later use
    opts.res = res;
    opts.ctx = ctx;
    opts.ssl = ssl;
    
    // Connect using the Socket
    status = connect(req->socket_fd, opts.res->ai_addr, opts.res->ai_addrlen);
    if (status < 0) { perror("[!] Failed to Connect"); return 1; }

    return 0;
}

void get_request(req_t *req, char* url, char* headers)
{
    char* request;
    char* response;
    int bytes;
    size_t used = 0;
    size_t capacity = 4096;
    size_t req_size;

    parse_url(url);
    
    // Use Provided Headers
    if (headers)
    {
        req_size = strlen(opts.path) + strlen(headers) + 17;
        request = malloc(req_size);
        snprintf(request, req_size, "GET %s HTTP/1.1\r\n%s", opts.path, headers);
    }

    // Use Default Headers
    else
    {
        char* default_headers = "User-Agent: requests\r\n"
                                "Accept: */*\r\n"
                                "Accept-Encoding: gzip, deflate, br\r\n"
                                "Connection: close\r\n\r\n";

        req_size = strlen(opts.path) + strlen(opts.hostname) + strlen(default_headers) + 25;
        request  = malloc(req_size);
        snprintf(request, req_size, "GET %s HTTP/1.1\r\nHost: %s\r\n%s", opts.path, opts.hostname, default_headers);
    }

    if (opts.is_https) { https_request(req, request, &response); }
    else { http_request(req, request, &response); }

    parse_status(req, response);
    parse_text(req, response);
    close(req->socket_fd);
}

void post_request(req_t *req, char* headers, char* data)
{
    char* request;
    char* response;
    size_t req_size;
    size_t data_size;

    if (headers)
    {
        req_size  = strlen(opts.path) + strlen(headers) + 18;
        data_size = strlen(data);
        request = malloc(req_size + data_size);
        snprintf(request, req_size + data_size, "POST %s HTTP/1.1\r\n%s%s", opts.path, headers, data);
    }
    
    else
    {
        char* default_headers = "User-Agent: requests\r\n"
                                "Content-Length:";

        req_size  = strlen(opts.path) + strlen(opts.hostname) + strlen(default_headers) + 26;
        data_size = strlen(data);
        request = malloc(req_size + data_size);
        snprintf(request, req_size + data_size, "POST %s HTTP/1.1\r\nHost: %s\r\n%s %zu\r\n\r\n%s", opts.path, opts.hostname, headers, data_size,data);
    }

    //connect(req->socket_fd, opts.res->ai_addr, opts.res->ai_addrlen);
    
    if (opts.is_https) { https_request(req, request, &response); }
    else { http_request(req, request, &response); }

    parse_status(req, response);
    parse_text(req, response);
    close(req->socket_fd);
}

void http_request(req_t *req, char* request, char** response)
{
    int bytes;
    size_t used = 0;
    size_t capacity = BUFFER_SIZE;
    (*response) = malloc(capacity);

    send(req->socket_fd, request, strlen(request), 0);

    while ((bytes = recv(req->socket_fd, (*response) + used, capacity - used, 0)) > 0)
    {
        used += bytes;
        if (used == capacity)
        {
            capacity *= 2;
            (*response) = realloc(*response, capacity);
        }
        (*response)[used] = '\0';
    }
}

void https_request(req_t *req, char* request, char** response)
{
    int bytes;
    size_t used = 0;
    size_t capacity = BUFFER_SIZE;
    (*response) = malloc(capacity);

    if (SSL_connect(opts.ssl) <= 0)
    {
        ERR_print_errors_fp(stderr);
        SSL_free(opts.ssl);
        close(req->socket_fd);
        SSL_CTX_free(opts.ctx);
        exit(-1);
    }

    SSL_write(opts.ssl, request, strlen(request));

    while ((bytes = SSL_read(opts.ssl, (*response) + used, capacity - used - 1)) > 0)
    {
        used += bytes;
        if (used >= capacity - 1)
        {
            capacity *= 2;
            (*response) = realloc(*response, capacity);
        }
    }
    (*response)[used] = '\0';

    SSL_shutdown(opts.ssl);
    SSL_free(opts.ssl);
    SSL_CTX_free(opts.ctx);
}

void parse_url(char* url)
{
    char* host_start;
    
    // Define protocol used
    if (strncmp(url, "https://", 8) == 0)
    {
        opts.port = 443;
        opts.is_https = true;
        host_start = url + 8; // skips https://
    }

    else if (strncmp(url, "http://", 7) == 0)
    {
        opts.port = 80;
        opts.is_https = false;
        host_start = url + 7; // skips http://
    }
    
    // Parse hostname, port and path
    char* port_delimiter = strchr(host_start, ':');
    char* path_delimiter = strchr(host_start, '/');
    
    // Define the end of the hostname based on delimiter
    size_t host_end = (port_delimiter) ? port_delimiter - host_start : path_delimiter - host_start;

    if (port_delimiter) { opts.port = atoi(port_delimiter + 1); }
    if (path_delimiter) { opts.path = path_delimiter; }
    
    // Allocate memory and copy the hostname
    opts.hostname = malloc(host_end * sizeof(char));
    strncpy(opts.hostname, host_start, host_end);
}

void parse_status(req_t *req, char* response)
{
    sscanf(response, "HTTP/%*s %3d %*s", &req->status_code);

}

void parse_text(req_t *req, char* response)
{
    req->text = strstr(response, "\r\n\r\n");

    if (req->text) { req->text += 4; }
}

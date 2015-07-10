/* 
 * File:   main.c
 * Author: dominik
 *
 * Created on 6 lipiec 2015, 13:18
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <uv.h>

#define DEFAULT_ADDRESS "127.0.0.1"
#define DEFAULT_PORT 1500

#define CHECK(r, msg) if (r) {                                                          \
    fprintf(stderr, "%s: [%s(%d): %s]\n", msg, uv_err_name((r)), r, uv_strerror((r)));  \
    exit(1);                                                                            \
}

void on_connect(uv_connect_t* req, int status);
void alloc_buffer(uv_handle_t *handle, size_t size, uv_buf_t *buf);
void recv_file(uv_stream_t* server, ssize_t nread, const uv_buf_t* buf);

int r;

int main(int argc, char * argv[]) {
    uv_loop_t* loop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
    r = uv_loop_init(loop);
    CHECK(r, "Loop init");

    uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    r = uv_tcp_init(loop, client);
    CHECK(r, "Client init");
    client->data = argv[1]; //file name

    struct sockaddr_in dest;
    uv_ip4_addr(DEFAULT_ADDRESS, DEFAULT_PORT, &dest);
    fprintf(stderr, "settings client: %s %d\n", inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));

    uv_connect_t* connect = (uv_connect_t*)malloc(sizeof(uv_connect_t));
    r = uv_tcp_connect(connect, client, (const struct sockaddr*)&dest, on_connect);
    CHECK(r, "Connect");
    
    r = uv_run(loop, UV_RUN_DEFAULT);
    CHECK(r, "Run");
    
    r = uv_loop_close(loop);
    CHECK(r, "Loop close");
    
    free(client);
    free(loop);
    return EXIT_SUCCESS;
}

void on_connect(uv_connect_t* req, int status) {
    CHECK(status, "New connection");
    
    uv_buf_t* buf = (uv_buf_t*)malloc(sizeof(uv_buf_t));
    *buf = uv_buf_init((char*)req->handle->data, strlen((char*)req->handle->data));
    
    uv_write_t* write_req = (uv_write_t*)malloc(sizeof(uv_write_t));    
    uv_write(write_req, req->handle, buf, 1, NULL);
    
    uv_read_start((uv_stream_t*)req->handle, alloc_buffer, recv_file);
}

void alloc_buffer(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
    *buf = uv_buf_init((char*)malloc(size), size);
}

void recv_file(uv_stream_t* server, ssize_t nread, const uv_buf_t* buf) {
    if (nread == UV_EOF) {
        uv_close((uv_handle_t*)server, NULL);
    } else if (nread > 0) {
        fprintf(stdout, "###\tread %ld bytes: %s\n", nread, buf->base);
    }
    if (nread == 0) free(buf->base);
}
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <uv.h>

#define DEFAULT_ADDRESS "127.0.0.1"
#define DEFAULT_PORT 1500
#define DEFAULT_BACKLOG 10

#define ERROR(msg, code) do {                                                         \
  fprintf(stderr, "%s: [%s: %s]\n", msg, uv_err_name((code)), uv_strerror((code)));   \
  assert(0);                                                                          \
} while(0);

uv_loop_t *loop;

void alloc_buffer(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
    *buf = uv_buf_init((char*) malloc(size), size);
}

void on_write(uv_write_t* req, int status) {
    if (status == -1) {
        fprintf(stderr, "error on_write");
        return;
    }
    
}

void on_connect(uv_connect_t* req, int status) {
    if (status == -1) {
        fprintf(stderr, "error on_connect");
        return;
    }
    
    char *message = "hello.txt";
    int len = strlen(message);
    
    char buffer[100];
    uv_buf_t buf = uv_buf_init(buffer, sizeof(buffer));
 
    buf.len = len;
    buf.base = message;
    
    uv_stream_t* tcp = req->handle;
    uv_write_t write_req;
 
    int buf_count = 1;
    uv_write(&write_req, tcp, &buf, buf_count, on_write);
}

int main() {
    loop = uv_default_loop();

    uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, client);

    uv_connect_t* connect = (uv_connect_t*)malloc(sizeof(uv_connect_t));

    struct sockaddr_in dest;
    uv_ip4_addr(DEFAULT_ADDRESS, DEFAULT_PORT, &dest);

    uv_tcp_connect(connect, client, (const struct sockaddr*)&dest, on_connect);
    
    return uv_run(loop, UV_RUN_DEFAULT);
}
/* 
 * File:   main.c
 * Author: dominik
 *
 * Created on 6 lipiec 2015, 13:18
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <uv.h>

#define DEFAULT_ADDRESS "127.0.0.1" //server address
#define DEFAULT_PORT 1500
#define BUFFER_SIZE 43776
#define LOG_ERROR_FILE "catchup_error.log"
#define LOG_INFO_FILE "catchup_info.log"

#define CHECK(r, msg) if (r) {                                                          \
    FILE *fd;                                                                           \
    fd = fopen(LOG_ERROR_FILE, "a");                                                    \
    if (fd)                                                                             \
        fprintf(fd, "%s: [%s(%d): %s]\n", msg, uv_err_name((r)), r, uv_strerror((r)));  \
    fclose(fd);                                                                         \
    fprintf(stderr, "%s: [%s(%d): %s]\n", msg, uv_err_name((r)), r, uv_strerror((r)));  \
    exit(EXIT_FAILURE);                                                                 \
}

#define LOG_INFO(msg) {                                                                 \
    time_t rawtime;                                                                     \
    time (&rawtime);                                                                    \
    char buffer[20];                                                                    \
    strftime(buffer,20,"%F %T",localtime(&rawtime));                                    \
    FILE *fd;                                                                           \
    fd = fopen(LOG_INFO_FILE, "a");                                                     \
    if (fd)                                                                             \
        fprintf(fd, "%s : %s\n", buffer, msg);                                          \
    fclose(fd);                                                                         \
}

void on_connect(uv_connect_t*, int);
void alloc_buffer(uv_handle_t*, size_t, uv_buf_t*);
void recv_file(uv_stream_t*, ssize_t, const uv_buf_t*);

int r;

int main(int argc, char * argv[]) {
    uv_loop_t* loop = malloc(sizeof (uv_loop_t));
    r = uv_loop_init(loop);
    CHECK(r, "Loop init");

    uv_tcp_t* client = malloc(sizeof (uv_tcp_t));
    r = uv_tcp_init(loop, client);
    CHECK(r, "Client init");
    client->data = argv[1]; //file name

    struct sockaddr_in dest;
    uv_ip4_addr(DEFAULT_ADDRESS, DEFAULT_PORT, &dest);
    fprintf(stderr, "settings client: %s %d\n", inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));

    uv_connect_t* connect = malloc(sizeof (uv_connect_t));
    r = uv_tcp_connect(connect, client, (const struct sockaddr*) &dest, on_connect);
    CHECK(r, "Connect");

    r = uv_run(loop, UV_RUN_DEFAULT);
    CHECK(r, "Run");

    free(connect);
    free(client);
    r = uv_loop_close(loop);
    CHECK(r, "Loop close");
    free(loop);

    return EXIT_SUCCESS;
}

void alloc_buffer(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
    (size > BUFFER_SIZE) ? size = BUFFER_SIZE : size;
    *buf = uv_buf_init((char*) malloc(size), size);
}

void on_connect(uv_connect_t* connect_req, int status) {
    CHECK(status, "New connection");

    //file name
    uv_buf_t* buf = malloc(sizeof (uv_buf_t));
    *buf = uv_buf_init((char*) connect_req->handle->data, strlen((char*) connect_req->handle->data));

    //send file name
    uv_write_t write_req;
    r = uv_write(&write_req, connect_req->handle, buf, 1, NULL);
    CHECK(r, "uv_write");

    uv_fs_t access_req;
    if (!uv_fs_access(connect_req->handle->loop, &access_req, connect_req->handle->data, O_RDWR, NULL)) {
        uv_fs_t unlink_req;
        r = uv_fs_unlink(connect_req->handle->loop, &unlink_req, connect_req->handle->data, NULL);
        CHECK(r, "unlink\n");
    }

    r = uv_read_start((uv_stream_t*) connect_req->handle, alloc_buffer, recv_file);
    CHECK(r, "uv_read_start");

    free(buf);
}

void recv_file(uv_stream_t* server, ssize_t nread, const uv_buf_t* buf) {
    uv_fs_t open_req;
    if (nread == UV_EOF) {
        uv_close((uv_handle_t*) server, NULL);
    } else if (nread > 0) {
        //fprintf(stdout, "nread %ld, buffer %ld\n%s\n", nread, buf->len, buf->base);

        char* data = malloc(sizeof (char)*nread);
        strncpy(data, buf->base, nread);
        uv_buf_t data_buf = uv_buf_init(data, nread);

        r = uv_fs_open(server->loop, &open_req, server->data, O_CREAT | O_WRONLY | O_APPEND, 0644, NULL);

        uv_fs_t write_req;
        uv_fs_write(server->loop, &write_req, open_req.result, &data_buf, 1, 0, NULL);

        uv_fs_t close_req;
        r = uv_fs_close(server->loop, &close_req, open_req.result, NULL);
        CHECK(r, "uv_fs_close");

    }
    if (nread == 0) {
        uv_close((uv_handle_t*) server, NULL);
    }
}
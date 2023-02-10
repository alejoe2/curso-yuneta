#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <uv.h>

uv_loop_t *loop;
struct sockaddr_in addr;

#define MAX_LEN_BUF 8

typedef struct
{
    uv_tcp_t handle;
    uv_buf_t buf;
    char data[100];
    FILE *file;
    int file_len;
    char file_size[100];
    char temporal[100];
    int file_name_len;
    char file_name[100];
    int size_header;
    int nread_total;
    int state;
    int total_save;
    uv_buf_t buf_temp;
    int nread_pos;
    int dif;
    int band;
    int dif_header;
} client_t;
/***************************************************************************
 *      Structures
 ***************************************************************************/

/*
 *  Used by main to communicate with parse_opt.
 */
#define MIN_ARGS 0
#define MAX_ARGS 0
struct arguments
{
    char *args[MAX_ARGS + 1]; /* positional args */
    int port;
    char *ip;
};

/***************************************************************************
 *              Prototypes
 ***************************************************************************/
static error_t parse_opt(int key, char *arg, struct argp_state *state);

/***************************************************************************
 *      Data
 ***************************************************************************/

// entry_point()

/* Program documentation. */
static char doc[] = "Listen in a tcp port and receive file";

/* A description of the arguments we accept. */
static char args_doc[] = "";

/*
 *  The options we understand.
 *  See https://www.gnu.org/software/libc/manual/html_node/Argp-Option-Vectors.html
 */
static struct argp_option options[] = {
    {0, 0, 0, 0, "Options:"},
    {"port", 'p', "PORT", 0, "set port to listen, default: 7000", 1},
    {"ip", 'i', "IP", 0, "set ip to listen, default: 127.0.0.1", 1},
    {0, 0, 0, 0, "Options Info. App:"},
    {"version", 'v', 0, 0, "Print program version", 1},
    {0},
};

/* Our argp parser. */
static struct argp argp = {
    options,
    parse_opt,
    args_doc,
    doc,
};

/********************************************************************
 *  Parse a single option
 ********************************************************************/
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    /*
     *  Var Infor App
     */
    const char *program_version = "1.0.0";
    // const char *program_bug_address = "echeverrialuish@hotmail.com";

    /*
     *  Get the input argument from argp_parse,
     *  which we know is a pointer to our arguments structure.
     */
    struct arguments *arguments = state->input;

    switch(key) {
    case 'p':
        if(arg) {
            arguments->port = atoi(arg);
        }
        break;
    case 'i':
        if(arg) {
            arguments->ip = arg;
        }
        break;
    case 'v':
        printf("%s\n", program_version);
        exit(0);
        break;

    case ARGP_KEY_ARG:
        if(state->arg_num >= MAX_ARGS) {
            /* Too many arguments. */
            argp_usage(state);
        }
        arguments->args[state->arg_num] = arg;
        break;

    case ARGP_KEY_END:
        if(state->arg_num < MIN_ARGS) {
            /* Not enough arguments. */
            argp_usage(state);
        }
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

void on_close(uv_handle_t *handle)
{
    free(handle);
}

/*****************************************************************
 *  alloc buffer
 *****************************************************************/
void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    char *base;
    base = (char *)calloc(1, suggested_size);
    if(!base)
        *buf = uv_buf_init(NULL, 0);
    else
        *buf = uv_buf_init(base, suggested_size);
}

void concat_buffers(uv_buf_t *dst, const uv_buf_t *src1, const uv_buf_t *src2, ssize_t nread_priv)
{
    memcpy(dst->base, src1->base, src1->len);
    memcpy(dst->base + src1->len, src2->base, nread_priv);
    dst->len = src1->len + nread_priv;
}

void read_8_byte(int *a, int *b, const uv_buf_t *src1)
{
    memcpy(a, src1->base, (MAX_LEN_BUF / 2));
    *a = ntohl(*a);
    printf("c->file_name_len: %d\n", *a);

    memcpy(b, src1->base + (MAX_LEN_BUF / 2), (MAX_LEN_BUF / 2));
    *b = ntohl(*b);
    printf("c->file_len: %d\n", *b);
}
void read_name_and_length(char *f_name, int *f_name_len, char *f_size, int *f_len, const uv_buf_t *src1, int *dif)
{
    memcpy(f_name, src1->base + *dif, *f_name_len);
    f_name[*f_name_len] = '\0';
    printf("file_name: %s\n", f_name);

    memcpy(f_size, src1->base + *f_name_len + *dif, *f_len);
    f_size[*f_len] = '\0';
    printf("file_size: %s\n", f_size);
}

/*******************************************************************
 *  On Read
 *******************************************************************/
void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf)
{

    client_t *c = (client_t *)client;
    printf("\n*****nread %ld \n", nread);

    if(nread > 0) {
        c->nread_total += nread;
        printf("c->nread_total: %i ***\n", c->nread_total);

        do {

            if(c->state == 0 && c->nread_total > 0) {

                printf("c->state == 0\n");

                if((c->nread_total) >= MAX_LEN_BUF) {
                    if(c->buf_temp.len > 0) {
                        if(c->buf_temp.len < c->nread_total) {
                            concat_buffers(&c->buf_temp, &c->buf_temp, buf, nread);
                            c->nread_total = c->buf_temp.len;
                            printf("c->buf_temp.len: %li ***\n", c->buf_temp.len);
                        }
                        read_8_byte(&c->file_name_len, &c->file_len, &c->buf_temp);
                    } else {
                        read_8_byte(&c->file_name_len, &c->file_len, buf);
                    }
                    c->nread_total -= 8;
                    c->state = 1;
                    printf("c->state == 0 OK ::%i\n", c->nread_total);
                    if(c->nread_total <= 0) {
                        c->buf_temp.len = 0;
                        c->dif_header = 0;
                        c->band = 0;
                    }

                } else {
                    if(c->buf_temp.len <= 0) {
                        c->buf_temp.base = (char *)malloc(0);
                        c->buf_temp.len = 0;
                    }
                    c->dif_header = 8;
                    c->band = 1;
                    concat_buffers(&c->buf_temp, &c->buf_temp, buf, nread);
                    c->nread_total = c->buf_temp.len;
                    printf("c->buf_temp.len: %li ***\n", c->buf_temp.len);
                }
            }

            if(c->state == 1 && c->nread_total > 0) {

                printf("c->state == 1\n");

                if(c->nread_total >= (c->file_len + c->file_name_len)) {

                    if(c->buf_temp.len > 0) {
                        if(c->band > 1) {
                            concat_buffers(&c->buf_temp, &c->buf_temp, buf, nread);
                            c->nread_total = c->buf_temp.len;
                            printf("c->buf_temp.len: %li ***\n", c->buf_temp.len);
                        }
                        read_name_and_length(c->file_name, &c->file_name_len, c->file_size, &c->file_len, &c->buf_temp, &c->dif_header);
                    } else {
                        c->dif_header = nread - c->nread_total;
                        read_name_and_length(c->file_name, &c->file_name_len, c->file_size, &c->file_len, buf, &c->dif_header);
                    }

                    if(access(c->file_name, F_OK) == 0) {
                        printf("File %s exists. \n", c->file_name);
                        remove(c->file_name);
                        usleep(100000);
                    }
                    c->size_header = MAX_LEN_BUF + c->file_len + c->file_name_len;
                    c->file = fopen(c->file_name, "a");
                    c->state = 2;
                    c->nread_total = c->nread_total - c->file_name_len - c->file_len;
                    printf("c->state == 1 ::%i\n", c->nread_total);
                    c->dif_header += c->file_len + c->file_name_len;
                    if(c->nread_total <= 0) {
                        c->buf_temp.len = 0;
                        c->dif_header = 0;
                        c->band = 0;
                    }
                } else {
                    if(c->buf_temp.len <= 0) {
                        c->buf_temp.base = (char *)malloc(0);
                        c->buf_temp.len = 0;
                    }
                    if(c->band > 1) {
                        concat_buffers(&c->buf_temp, &c->buf_temp, buf, nread);
                        c->nread_total = c->buf_temp.len;
                        printf("c->buf_temp.len: %li ***\n", c->buf_temp.len);
                    }
                    c->band = 2;
                }
            }

            if(c->state == 2 && c->nread_total > 0) {
                printf("c->state == 2\n");
                printf("c->nread_total: %i\n", c->nread_total);

                if(c->buf_temp.len > 0) {

                    printf("1-WRITE\n");
                    fwrite(c->buf_temp.base + c->dif_header, 1, c->buf_temp.len, c->file);
                    c->total_save += c->buf_temp.len - c->dif_header;
                    if(c->total_save >= atoi(c->file_size)) {
                        c->dif = c->total_save - atoi(c->file_size);
                        memcpy(c->temporal, c->buf_temp.base + c->dif, c->buf_temp.len);
                        // printf("temporal: %s", c->temporal);

                        c->buf_temp.base = c->temporal;
                        c->buf_temp.len = atoi(c->temporal);
                        // concat_buffers(&c->buf_temp, &c->buf_temp, c->buf_temp.base + c->dif, c->buf_temp.len - c->dif);
                        c->nread_total = c->buf_temp.len;
                        printf("c->buf_temp.len: %li ***\n", c->buf_temp.len);
                    }
                } else {

                    printf("2d-WRITE\n");
                    fwrite(buf->base + (nread - c->nread_total), 1, nread, c->file);
                    c->total_save += c->nread_total;
                    if(c->total_save >= atoi(c->file_size)) {

                        c->dif = c->total_save - atoi(c->file_size);
                        memcpy(c->temporal, buf->base + (nread - c->nread_total) + c->dif, nread - c->dif);
                        // printf("temporal: %s\n", c->temporal);

                        c->buf_temp.base = c->temporal;
                        c->buf_temp.len = atoi(c->temporal);
                        // concat_buffers(&c->buf_temp, &c->buf_temp, buf->base + (nread - c->nread_total) + c->dif, nread - c->dif - (nread - c->nread_total));
                        c->nread_total = c->buf_temp.len;
                        printf("c->buf_temp.len: %li ***\n", c->buf_temp.len);
                    }
                }

                printf("c->total_rec: %i\n", c->total_save);

                if(c->total_save >= atoi(c->file_size)) {
                    fclose(c->file);
                    c->dif = c->total_save - atoi(c->file_size);
                    printf("c->dif: %i\n", c->dif);
                    c->band = 0;
                    if(c->buf_temp.len > 0) {
                        c->nread_pos = atoi(c->buf_temp.base);
                    } else {
                        c->nread_pos = atoi(buf->base);
                    }
                    c->total_save = 0;
                    c->state = 0;
                    c->nread_total = c->dif;

                } else {
                    printf("Continuar");
                    c->nread_total = 0;
                    c->buf_temp.len = 0;
                }
            }
        } while(c->dif > 0);

        // uv_read_stop(client);
        // uv_close((uv_handle_t *)client, on_close);
        // uv_read_start((uv_stream_t *)&client, alloc_buffer, on_read);

        printf("\n");

    } else {
        c->state = 0;
        c->nread_total = 0;
        free(buf->base);
        if(nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        uv_close((uv_handle_t *)client, on_close);
        return;
    }

    if(buf->base) {
        free(buf->base);
    }
}

/*****************************************************************
 *  get peername sockname
 *****************************************************************/

void get_peer_and_sock(uv_tcp_t *client)
{
    char client_ip[17] = {0};
    int client_port = 0;
    struct sockaddr_in client_addr;
    int len = sizeof(client_addr);
    uv_tcp_getpeername(client, (struct sockaddr *)&client_addr, &len);
    uv_inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    client_port = ntohs(client_addr.sin_port);
    printf("Peer name: %s:%d\n", client_ip, client_port);

    len = sizeof(addr);
    uv_tcp_getsockname(client, (struct sockaddr *)&addr, &len);
    char server_ip[17] = {0};
    int server_port = 0;
    uv_inet_ntop(AF_INET, &addr.sin_addr, server_ip, sizeof(server_ip));
    server_port = ntohs(addr.sin_port);
    printf("Sock name: %s:%d\n", server_ip, server_port);
}

/*****************************************************************
 *  On Conection
 *****************************************************************/
void on_connect(uv_stream_t *server, int status)
{
    if(status < 0) {
        fprintf(stderr, "Connect error %s\n", uv_strerror(status));
        return;
    }

    client_t *client = (client_t *)malloc(sizeof(client_t));

    uv_tcp_init(loop, &client->handle);

    if(uv_accept(server, (uv_stream_t *)client) == 0) {
        client->handle.data = client;
        get_peer_and_sock(&client->handle);
        uv_read_start((uv_stream_t *)&client->handle, alloc_buffer, on_read);

    } else {
        uv_close((uv_handle_t *)client, NULL);
    }
}

/*****************************************************************
 *                      Main
 *****************************************************************/
int main(int argc, char *argv[])
{
    struct arguments arguments;

    /*Default values*/
    memset(&arguments, 0, sizeof(arguments));
    arguments.port = 7000;
    arguments.ip = "127.0.0.12";

    /*Parse arguments*/
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    loop = uv_default_loop();

    uv_tcp_t server;
    uv_tcp_init(loop, &server);

    uv_ip4_addr(arguments.ip, arguments.port, &addr);

    uv_tcp_bind(&server, (const struct sockaddr *)&addr, 0);
    int r = uv_listen((uv_stream_t *)&server, 128, on_connect);
    if(r) {
        fprintf(stderr, "Listen error %s\n", uv_strerror(r));
        return 1;
    }
    return uv_run(loop, UV_RUN_DEFAULT);
}

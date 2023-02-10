#include <argp.h>
#include <ghelpers.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/***************************************************************************
 *      Prototypes
 ***************************************************************************/
void get_peer_and_sock(uv_tcp_t *client);
void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);
static error_t parse_opt(int key, char *arg, struct argp_state *state);
void on_close(uv_handle_t *handle);

/***************************************************************************
 *      Structures
 *      4 + 4 + filename + filecontent
 ***************************************************************************/
typedef enum
{
    ST_WAIT_HEADER,
    ST_WAIT_FILENAME,
    ST_WAIT_CONTENT
} state_t;

#pragma pack(1)
typedef struct header_s
{
    uint32_t *filename_length;
    uint32_t file_length;
} header_t;
#pragma pack()

typedef struct client_s
{
    header_t header;
    char filename[256];

    GBUFFER *gbuf_header;
    GBUFFER *gbuf_filename;
    GBUFFER *gbuf_content;

    uv_tcp_t *uv;
    int fp;
    state_t state;
} client_t;

/***************************************************************************
 *      Data
 ***************************************************************************/
uv_loop_t *loop;
struct sockaddr_in addr;

#define programfile "listenerfile"

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

/*******************************************************************
 *  New client
 *******************************************************************/
int new_client(uv_tcp_t *uv_tcp)
{
    client_t *client = malloc(sizeof(client_t));
    if(!client) {
        log_error(LOG_OPT_TRACE_STACK, "%s: No memory", programfile);
        // uv_close(uv_tcp); // TODO
        return -1;
    }
    memset(client, 0, sizeof(client_t));

    uv_tcp->data = client;
    client->uv = uv_tcp;

    client->gbuf_header = gbuf_create(sizeof(header_t), sizeof(header_t), 0, 0);
    if(!client->gbuf_header) {
        // log_error TODO desconectar
        return -1;
    }

    get_peer_and_sock(uv_tcp);
    uv_read_start((uv_stream_t *)uv_tcp, alloc_buffer, on_read);

    return 0;
}

/*******************************************************************
 *  Free client
 *******************************************************************/
void free_client(client_t *client)
{
    // TODO revisar cuando hay que liberar free(buf->base);

    if(client->fp) {
        close(client->fp);
        client->fp = 0;
    }
    if(client->gbuf_header) {
        GBUF_DECREF(client->gbuf_header)
    }
    if(client->gbuf_content) {
        GBUF_DECREF(client->gbuf_content)
    }
    if(client->gbuf_filename) {
        GBUF_DECREF(client->gbuf_filename)
    }

    free(client);
}

/*******************************************************************
 *  On Read
 *******************************************************************/
int process_data(client_t *client, char *bf, size_t bflen)
{
    while(bflen > 0) {
        switch(client->state) {
        case ST_WAIT_HEADER: {
            size_t written = gbuf_append(client->gbuf_header, bf, bflen);
            bflen -= written;
            bf += written;

            if(gbuf_totalbytes(client->gbuf_header) == sizeof(header_t)) {
                client->header.filename_length = gbuf_get(client->gbuf_header, sizeof(uint32_t));
                *client->header.filename_length = htonl(*client->header.filename_length);

                memmove(
                    &client->header.file_length,
                    gbuf_get(client->gbuf_header, sizeof(uint32_t)),
                    sizeof(uint32_t));
                client->header.file_length = htonl(client->header.file_length);

                client->gbuf_filename = gbuf_create(
                    *client->header.filename_length,
                    *client->header.filename_length,
                    0,
                    0);
                if(!client->gbuf_filename) {
                    // TODO log_error
                    return -1;
                }

                client->gbuf_content = gbuf_create(4 * 1024, client->header.file_length, 0, 0);
                if(!client->gbuf_content) {
                    // TODO log_error
                    return -1;
                }

                client->state = ST_WAIT_FILENAME;
            }
        } break;

        case ST_WAIT_FILENAME: {
            size_t written = gbuf_append(client->gbuf_filename, bf, bflen);
            bflen -= written;
            bf += written;
            if(gbuf_totalbytes(client->gbuf_filename) == *client->header.filename_length) {
                // TODO create file
                client->fp = open(client->filename, O_TRUNC);

                client->state = ST_WAIT_CONTENT;
            }
        } break;

        case ST_WAIT_CONTENT: {
            size_t written = gbuf_append(client->gbuf_content, bf, bflen);
            bflen -= written;
            bf += written;
            if(gbuf_totalbytes(client->gbuf_content) == client->header.file_length) {
                // TODO create file
                write(
                    client->fp,
                    gbuf_cur_rd_pointer(client->gbuf_content),
                    client->header.file_length);
                close(client->fp);
                client->fp = -1;

                GBUF_DECREF(client->gbuf_content)
                GBUF_DECREF(client->gbuf_filename)
                gbuf_clear(client->gbuf_header);

                client->state = ST_WAIT_HEADER;
            }
        } break;
        }
    }

    return 0;
}

/*******************************************************************
 *  On Read
 *******************************************************************/
void on_read(uv_stream_t *uv_stream, ssize_t nread, const uv_buf_t *buf)
{
    client_t *client = uv_stream->data;

    if(nread == UV_EOF) {
        /* El final de los datos se ha alcanzado */
        uv_read_stop(uv_stream);
        return;
    }

    if(nread < 0) {
        /* Error */
        fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        uv_close((uv_handle_t *)uv_stream, on_close);
        return;
    }

    if(nread > 0) {
        if(process_data(client, buf->base, nread) < 0) {
            uv_close((uv_handle_t *)uv_stream, on_close);
        }
    }

    //    free(buf->base);
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
 *
 *****************************************************************/
void on_close(uv_handle_t *handle)
{
    client_t *client = handle->data;
    free_client(client);
    free(handle);
}

/*****************************************************************
 *  On Connection
 *****************************************************************/
void on_connect(uv_stream_t *server, int status)
{
    if(status < 0) {
        fprintf(stderr, "Connect error %s\n", uv_strerror(status));
        return;
    }

    uv_tcp_t *uv_tcp = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, uv_tcp);

    if(uv_accept(server, (uv_stream_t *)uv_tcp) == 0) {
        if(new_client(uv_tcp) < 0) {
            // log_error() TODO
        }

    } else {
        uv_close((uv_handle_t *)server, on_close);
    }
}

/*****************************************************************
 *                      Main
 *****************************************************************/
int main(int argc, char *argv[])
{
    struct arguments arguments;

    init_ghelpers_library(programfile);

    /*Default values*/
    memset(&arguments, 0, sizeof(arguments));
    arguments.port = 7000;
    arguments.ip = "127.0.0.1";

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

    uv_run(loop, UV_RUN_DEFAULT);

    end_ghelpers_library();
}

#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <uv.h>

uv_loop_t *loop;
struct sockaddr_in addr;
FILE *file;
static int file_name_len = 0;
static int file_len = 0;
static char file_name[100];

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

/*******************************************************************
 *  On Read
 *******************************************************************/
void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf)
{

    // static int file_data_len = 0;
    // static char *file_data = NULL;

    if(nread == UV_EOF) {

        /* El final de los datos se ha alcanzado */
        uv_read_stop(client);
        return;
    }

    if(nread < 0) {
        /* Error */
        if(nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        uv_close((uv_handle_t *)client, NULL);
        return;
    }

    if(nread > 0) {

        if(file_name_len == 0) {
            /* se recibe de nombre de fichero*/
            memcpy(&file_name_len, buf->base, 4);
            file_name_len = ntohl(file_name_len);
            memcpy(file_name, buf->base + 4, file_name_len);
            file_name[file_name_len] = '\0';

            // printf("Nombre del archivo: %s\n", file_name);
            // printf("file_name_len: %i\n", file_name_len);

            /* Verificacion de existencia de archivo*/
            if(access(file_name, F_OK) == 0) {
                printf("File %s exists. \n", file_name);
                /* Send Confirmation. */
                uv_write_t *req = malloc(sizeof(uv_write_t));
                uv_buf_t buf = uv_buf_init("overwritingFile\n", 15);
                uv_write(req, (uv_stream_t *)client, &buf, 1, NULL);
                remove(file_name);
            } else {
                // printf("El archivo %s no existe\n", file_name);
                uv_write_t *req = malloc(sizeof(uv_write_t));
                uv_buf_t buf = uv_buf_init("createOK\n", 8);
                uv_write(req, (uv_stream_t *)client, &buf, 1, NULL);
            }

            file = fopen(file_name, "a");

        } else if(file_len == 0) {
            /* Se recibe tamaño de fichero*/
            memcpy(&file_len, buf->base, 4);
            file_len = ntohl(file_len);
            // file_data = (char *)malloc(file_len);
            // printf("file_len: %i \n", file_len);
        }

        if(file_len != 0) {
            /* Se guarda el fichero*/
            // FILE *file = fopen(file_name, "a");
            fwrite(buf->base, 1, nread, file);

            // printf("Data received: %ld bytes\n", nread);
        }

        if(file_len != 0 && buf->len > nread) {
            /* Se confirma fin de datos*/
            printf("file receive: %s\n", file_name);
            /* Send Confirmation. */
            uv_write_t *req = malloc(sizeof(uv_write_t));
            uv_buf_t buf = uv_buf_init("OK\n", 4);
            uv_write(req, (uv_stream_t *)client, &buf, 1, NULL);
            file_name_len = 0;
            file_len = 0;
            fclose(file);

            usleep(55000);
        }
    }
    free(buf->base);
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
 *  On Conection
 *****************************************************************/
void on_connect(uv_stream_t *server, int status)
{
    if(status < 0) {
        fprintf(stderr, "Connect error %s\n", uv_strerror(status));
        return;
    }

    uv_tcp_t *client = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, client);

    if(uv_accept(server, (uv_stream_t *)client) == 0) {

        get_peer_and_sock(client);
        uv_read_start((uv_stream_t *)client, alloc_buffer, on_read);

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
    return uv_run(loop, UV_RUN_DEFAULT);
}

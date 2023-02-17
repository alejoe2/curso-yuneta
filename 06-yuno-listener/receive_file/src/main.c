/****************************************************************************
 *          MAIN_RECEIVE_FILE.C
 *          receive_file main
 *
 *          Receive a file multiple times
 *
 *          Copyright (c) 2023 yuneta.
 *          All Rights Reserved.
 ****************************************************************************/
#include <yuneta.h>
#include "c_receive_file.h"
#include "c_decoder_receive_file.h"
#include "yuno_receive_file.h"

/***************************************************************************
 *                      Names
 ***************************************************************************/
#define APP_NAME        ROLE_RECEIVE_FILE
#define APP_DOC         "Receive a file multiple times"

#define APP_VERSION     "1.0.0"
#define APP_SUPPORT     "<yuneta@yuneta.io>"
#define APP_DATETIME    __DATE__ " " __TIME__

/***************************************************************************
 *                      Default config
 ***************************************************************************/
PRIVATE char fixed_config[]= "\
{                                                                   \n\
    'yuno': {                                                       \n\
        'yuno_role': '"ROLE_RECEIVE_FILE"',                         \n\
        'tags': ['app', 'citizen']                                  \n\
    }                                                               \n\
}                                                                   \n\
";
PRIVATE char variable_config[]= "\
{                                                                   \n\
    'environment': {                                                \n\
        'use_system_memory': true,                                  \n\
        'log_gbmem_info': true,                                     \n\
        'MEM_MIN_BLOCK': 512,                                       \n\
        'MEM_MAX_BLOCK': 209715200,             #^^  200*M          \n\
        'MEM_SUPERBLOCK': 209715200,            #^^  200*M          \n\
        'MEM_MAX_SYSTEM_MEMORY': 2147483648,    #^^ 2*G             \n\
        'console_log_handlers': {                                   \n\
            'to_stdout': {                                          \n\
                'handler_type': 'stdout',                           \n\
                'handler_options': 255                              \n\
            }                                                       \n\
        },                                                          \n\
        'daemon_log_handlers': {                                    \n\
            'to_file': {                                            \n\
                'handler_type': 'file',                             \n\
                'filename_mask': 'receive_file-W.log',          \n\
                'handler_options': 255                              \n\
            },                                                      \n\
            'to_udp': {                                             \n\
                'handler_type': 'udp',                              \n\
                'url': 'udp://127.0.0.1:1992',                      \n\
                'handler_options': 255                              \n\
            }                                                       \n\
        }                                                           \n\
    },                                                              \n\
    'yuno': {                                                       \n\
        'required_services': [],                                    \n\
        'public_services': [],                                      \n\
        'service_descriptor': {                                     \n\
        },                                                          \n\
        'trace_levels': {                                           \n\
            'Tcp0': ['connections'],                                \n\
            'TcpS0': ['listen', 'not-accepted', 'accepted']         \n\
        }                                                           \n\
    },                                                              \n\
    'global': {                                                     \n\
    },                                                              \n\
    'services': [                                                   \n\
        {                                                           \n\
            'name': 'receive_file',                                 \n\
            'gclass': 'Receive_file',                               \n\
            'default_service': true,                                \n\
            'autostart': true,                                      \n\
            'autoplay': false,                                      \n\
            'kw': {                                                 \n\
            },                                                      \n\
            'zchilds': [                                            \n\
            ]                                                       \n\
        }                                                           \n\
    ]                                                               \n\
}                                                                   \n\
";

/***************************************************************************
 *                      Register
 ***************************************************************************/
static void register_yuno_and_more(void)
{
    /*-------------------*
     *  Register yuno
     *-------------------*/
    register_yuno_receive_file();

    /*--------------------*
     *  Register service
     *--------------------*/
    gobj_register_gclass(GCLASS_DECODER_RECEIVE_FILE);
    gobj_register_gclass(GCLASS_RECEIVE_FILE);
}

/***************************************************************************
 *                      Main
 ***************************************************************************/
int main(int argc, char *argv[])
{
    /*------------------------------------------------*
     *  To trace memory
     *------------------------------------------------*/
#ifdef DEBUG
    static uint32_t mem_list[] = {0};
    gbmem_trace_alloc_free(0, mem_list);
#endif

    /*------------------------------------------------*
     *          Traces
     *------------------------------------------------*/
    gobj_set_gclass_no_trace(GCLASS_TIMER, "machine", TRUE);  // Avoid timer trace, too much information

// Samples
//     gobj_set_gclass_trace(GCLASS_IEVENT_CLI, "ievents2", TRUE);
//     gobj_set_gclass_trace(GCLASS_IEVENT_SRV, "ievents2", TRUE);
//     gobj_set_gclass_trace(GCLASS_RECEIVE_FILE, "messages", TRUE);

//     gobj_set_gobj_trace(0, "create_delete", TRUE, 0);
//     gobj_set_gobj_trace(0, "create_delete2", TRUE, 0);
//     gobj_set_gobj_trace(0, "start_stop", TRUE, 0);
//     gobj_set_gobj_trace(0, "subscriptions", TRUE, 0);
//     gobj_set_gobj_trace(0, "machine", TRUE, 0);
//     gobj_set_gobj_trace(0, "ev_kw", TRUE, 0);
//     gobj_set_gobj_trace(0, "libuv", TRUE, 0);

    /*------------------------------------------------*
     *          Start yuneta
     *------------------------------------------------*/
    helper_quote2doublequote(fixed_config);
    helper_quote2doublequote(variable_config);
    yuneta_setup(
        dbattrs_startup,
        dbattrs_end,
        dbattrs_load_persistent,
        dbattrs_save_persistent,
        dbattrs_remove_persistent,
        dbattrs_list_persistent,
        command_parser,
        stats_parser,
        0,
        0
    );
    return yuneta_entry_point(
        argc, argv,
        APP_NAME, APP_VERSION, APP_SUPPORT, APP_DOC, APP_DATETIME,
        fixed_config,
        variable_config,
        register_yuno_and_more
    );
}

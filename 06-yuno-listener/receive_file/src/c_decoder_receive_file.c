/***********************************************************************
 *          C_DECODER_RECEIVE_FILE.C
 *          Decoder_receive_file GClass.
 *
 *          Decoder multiple files
 *
 *          Copyright (c) 2023 yuneta.
 *          All Rights Reserved.
 ***********************************************************************/
#include <string.h>
#include "c_decoder_receive_file.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/

/***************************************************************************
 *              Structures
 ***************************************************************************/

/***************************************************************************
 *              Prototypes
 ***************************************************************************/


/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/

/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
/*-ATTR-type------------name----------------flag------------default---------description---------- */
SDATA (ASN_BOOLEAN,     "iamServer",        SDF_RD,         1,              "What side? server or client"),
SDATA (ASN_JSON,        "kw_connex",        SDF_RD,         0,              "Kw to create connex if client"),
SDATA (ASN_INTEGER,     "timeout_inactivity",SDF_WR,        10*1000,        "Timeout inactivity"),
SDATA (ASN_BOOLEAN,     "connected",        SDF_RD|SDF_STATS,0,             "Connection state. Important filter!"),
SDATA (ASN_OCTET_STR,   "on_open_event_name",SDF_RD,        "EV_ON_OPEN",   "Must be empty if you don't want receive this event"),
SDATA (ASN_OCTET_STR,   "on_close_event_name",SDF_RD,       "EV_ON_CLOSE",  "Must be empty if you don't want receive this event"),
SDATA (ASN_OCTET_STR,   "on_message_event_name",SDF_RD,     "EV_ON_MESSAGE","Must be empty if you don't want receive this event"),
SDATA (ASN_POINTER,     "user_data",        0,              0,              "user data"),
SDATA (ASN_POINTER,     "user_data2",       0,              0,              "more user data"),
SDATA (ASN_POINTER,     "subscriber",       0,              0,              "subscriber of output-events. If it's null then subscriber is the parent."),
SDATA_END()
};

/*---------------------------------------------*
 *      GClass trace levels
 *  HACK strict ascendent value!
 *  required paired correlative strings
 *  in s_user_trace_level
 *---------------------------------------------*/
enum {
    TRACE_TRAFFIC  = 0x0001,
    TRACE_MESSAGES = 0x0002,
};
PRIVATE const trace_level_t s_user_trace_level[16] = {
{"traffic",         "Trace traffic"},
{"messages",        "Trace messages"},
{0, 0},
};

/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef enum
{
    ST_WAIT_HEADER,
    ST_WAIT_FILENAME,
    ST_WAIT_CONTENT
} state_t;

#pragma pack(1)
typedef struct header_s {
    uint32_t filename_length;
    uint32_t file_length;
} header_t;
#pragma pack()

typedef struct _PRIVATE_DATA {
    hgobj timer;
    char iamServer;                     // What side? server or client
    TYPE_ASN_BOOLEAN *pconnected;

    int timeout_inactivity;
    const char *on_open_event_name;
    const char *on_close_event_name;
    const char *on_message_event_name;
    int inform_on_close;

    header_t header;
    char filename[256];
    GBUFFER *gbuf_header;
    GBUFFER *gbuf_filename;
    GBUFFER *gbuf_content;
    state_t state;

} PRIVATE_DATA;




            /******************************
             *      Framework Methods
             ******************************/




/***************************************************************************
 *      Framework Method create
 ***************************************************************************/
PRIVATE void mt_create(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    priv->pconnected = gobj_danger_attr_ptr(gobj, "connected");
    priv->iamServer = gobj_read_bool_attr(gobj, "iamServer");

    priv->timer = gobj_create(gobj_name(gobj), GCLASS_TIMER, 0, gobj);

    /*
     *  CHILD subscription model
     */
    hgobj subscriber = (hgobj)gobj_read_pointer_attr(gobj, "subscriber");
    if(!subscriber)
        subscriber = gobj_parent(gobj);
    gobj_subscribe_event(gobj, NULL, NULL, subscriber);

    /*
     *  Do copy of heavy used parameters, for quick access.
     *  HACK The writable attributes must be repeated in mt_writing method.
     */
    SET_PRIV(on_open_event_name,    gobj_read_str_attr)
    SET_PRIV(on_close_event_name,   gobj_read_str_attr)
    SET_PRIV(on_message_event_name, gobj_read_str_attr)
    SET_PRIV(timeout_inactivity,    gobj_read_int32_attr)

    if(!priv->iamServer) {
        json_t *kw_connex = gobj_read_json_attr(gobj, "kw_connex");
        JSON_INCREF(kw_connex);
        hgobj gobj_bottom = gobj_create(gobj_name(gobj), GCLASS_CONNEX, kw_connex, gobj);
        gobj_set_bottom_gobj(gobj, gobj_bottom);
    }
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    IF_EQ_SET_PRIV(timeout_inactivity,          gobj_read_int32_attr)
    END_EQ_SET_PRIV()
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    gobj_start(priv->timer);

    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    clear_timeout(priv->timer);
    gobj_stop(priv->timer);

    return 0;
}

/***************************************************************************
 *      Framework Method destroy
 ***************************************************************************/
PRIVATE void mt_destroy(hgobj gobj)
{
}




            /***************************
             *      Local Methods
             ***************************/




/*******************************************************************
 *  On Process Data
 *******************************************************************/
int process_data(hgobj gobj, GBUFFER *gbuf)
{
    size_t gbflen = gbuf_totalbytes(gbuf);
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(gobj_trace_level(gobj) & TRACE_MESSAGES) {
        trace_msg0("ENTRO st %d, recibo %d bytes", priv->state, (int)gbflen);
    }

    while(gbflen > 0) {
        switch(priv->state) {
            case ST_WAIT_HEADER:
            {
                size_t written = gbuf_append(
                        priv->gbuf_header,
                        gbuf_get(gbuf, sizeof(header_t)),
                        MIN(gbuf_freebytes(priv->gbuf_header), gbflen)
                );
                gbflen -= written;
                //gbuf += written;
                //trace_msg0("st %d, consumo %d, quedan %d", priv->state, (int)written, (int)gbflen);

                if(gbuf_totalbytes(priv->gbuf_header) == sizeof(header_t)) {

                    memmove(
                        &priv->header.filename_length,
                        gbuf_get(priv->gbuf_header, sizeof(uint32_t)),
                        sizeof(uint32_t)
                    );
                    priv->header.filename_length = htonl(priv->header.filename_length);

                    memmove(
                        &priv->header.file_length,
                        gbuf_get(priv->gbuf_header, sizeof(uint32_t)),
                        sizeof(uint32_t)
                    );
                    priv->header.file_length = htonl(priv->header.file_length);

                    priv->gbuf_filename = gbuf_create(
                        priv->header.filename_length,
                        priv->header.filename_length,
                        0,
                        0
                    );

                    if(!priv->gbuf_filename) {
                        trace_msg0("No memory for gbuf_filename");
                        return -1;
                    }

                    priv->gbuf_content = gbuf_create(
                        priv->header.file_length,
                        priv->header.file_length,
                        0,
                        0
                    );
                    if(!priv->gbuf_content) {
                        trace_msg0("No memory for gbuf_content");
                        return -1;
                    }
                    if(gobj_trace_level(gobj) & TRACE_MESSAGES) {
                        trace_msg0("Next state: ST_WAIT_FILENAME");
                    }
                    priv->state = ST_WAIT_FILENAME;
                }
            }
                break;

            case ST_WAIT_FILENAME:
            {
                size_t written = gbuf_append(
                    priv->gbuf_filename,
                    gbuf_get(gbuf, priv->header.filename_length),
                    MIN(gbuf_freebytes(priv->gbuf_filename), gbflen)
                );
                gbflen -= written;
                //gbuf += written;
                if(gobj_trace_level(gobj) & TRACE_MESSAGES) {
                    trace_msg0("st %d, consumo %d, quedan %d", priv->state, (int)written, (int)gbflen);
                }

                if(gbuf_totalbytes(priv->gbuf_filename) == priv->header.filename_length) {
                    memmove(
                        priv->filename,
                        gbuf_get(priv->gbuf_filename, priv->header.filename_length),
                        priv->header.filename_length
                    );

                    if(gobj_trace_level(gobj) & TRACE_MESSAGES) {
                        trace_msg0("Next state: ST_WAIT_CONTENT");
                    }
                    priv->state = ST_WAIT_CONTENT;
                }
            }
                break;

            case ST_WAIT_CONTENT:
            {
                size_t written = gbuf_append(
                    priv->gbuf_content,
                    gbuf_get(gbuf, MIN(gbuf_freebytes(priv->gbuf_content), gbflen)),
                    MIN(gbuf_freebytes(priv->gbuf_content), gbflen)
                );
                gbflen -= written;
                //gbuf += written;

                if(gobj_trace_level(gobj) & TRACE_MESSAGES) {
                    trace_msg0("st %d, consumo %d, quedan %d", priv->state, (int) written, (int) gbflen);
                }

                if(gbuf_totalbytes(priv->gbuf_content) == priv->header.file_length) {


                    gbuf_setlabel(priv->gbuf_content, priv->filename);
                    GBUF_INCREF(priv->gbuf_content);
                    json_t *kw_publish = json_pack("{s:s, s:s, s:I}",
                        "type", "file",
                        "filename", priv->filename,
                        "gbuffer", (json_int_t)(size_t)priv->gbuf_content
                    );
                    gobj_publish_event(gobj, "EV_ON_MESSAGE", kw_publish);

                    priv->gbuf_content = 0;
                    memset(priv->filename, 0, sizeof(priv->filename));
                    GBUF_DECREF(priv->gbuf_filename)
                    gbuf_clear(priv->gbuf_header);

                    if(gobj_trace_level(gobj) & TRACE_MESSAGES) {
                        trace_msg0("Next state: ST_WAIT_HEADER");
                    }

                    priv->state = ST_WAIT_HEADER;
                }
            }
                break;
        }
    }

    if(gobj_trace_level(gobj) & TRACE_MESSAGES) {
        trace_msg0("SALGO st %d, recibo %d bytes\n", priv->state, (int) gbflen);
    }

    return 0;
}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_connected(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    *priv->pconnected = 1;

    gobj_change_state(gobj, "ST_SESSION");

    priv->inform_on_close = TRUE;
    if(!empty_string(priv->on_open_event_name)) {
        gobj_publish_event(gobj, priv->on_open_event_name, 0);
    }

    set_timeout(priv->timer, priv->timeout_inactivity);

    priv->gbuf_header = gbuf_create(sizeof(header_t), sizeof(header_t), 0, 0);
    if(!priv->gbuf_header) {
        log_error(LOG_OPT_TRACE_STACK|LOG_OPT_EXIT_NEGATIVE,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_MEMORY_ERROR,
            "msg",          "%s", "No memory fo gbuf",
            NULL
        );
    }

    KW_DECREF(kw)
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_disconnected(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    *priv->pconnected = 0;

    clear_timeout(priv->timer);

    if(gobj_is_volatil(src)) {
        gobj_set_bottom_gobj(gobj, 0);
    }

    if(priv->inform_on_close) {
        priv->inform_on_close = FALSE;
        if(!empty_string(priv->on_close_event_name)) {
            gobj_publish_event(gobj, priv->on_close_event_name, 0);
        }
    }

    if(priv->gbuf_header) {
        GBUF_DECREF(priv->gbuf_header)
    }
    if(priv->gbuf_content) {
        GBUF_DECREF(priv->gbuf_content)
    }
    if(priv->gbuf_filename) {
        GBUF_DECREF(priv->gbuf_filename)
    }

    KW_DECREF(kw);
    return 0;
}

/********************************************************************
 *
 ********************************************************************/
PRIVATE int ac_rx_data(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    GBUFFER *gbuf = (GBUFFER *)(size_t)kw_get_int(kw, "gbuffer", 0, 0);

    set_timeout(priv->timer, priv->timeout_inactivity);

    if(gobj_trace_level(gobj) & TRACE_TRAFFIC) {
        log_debug_gbuf(LOG_DUMP_INPUT, gbuf, "%s", gobj_short_name(gobj));
    }

    process_data(gobj, gbuf);

    KW_DECREF(kw);
    return 0;
}

/********************************************************************
 *
 ********************************************************************/
PRIVATE int ac_send_message(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    // TODO format output message or directly send
    return gobj_send_event(gobj_bottom_gobj(gobj), "EV_TX_DATA", kw, gobj);
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_drop(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    gobj_send_event(gobj_bottom_gobj(gobj), "EV_DROP", 0, gobj);

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_timeout_inactivity(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    //gobj_send_event(gobj_bottom_gobj(gobj), "EV_DROP", 0, gobj);
    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *  Child stopped
 ***************************************************************************/
PRIVATE int ac_stopped(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    if(gobj_is_volatil(src)) {
        gobj_destroy(src);
    }
    JSON_DECREF(kw);
    return 0;
}


/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    // top input
    // bottom input
    {"EV_RX_DATA",          0,  0,  ""},
    {"EV_SEND_MESSAGE",     0,  0,  ""},
    {"EV_CONNECTED",        0,  0,  ""},
    {"EV_DISCONNECTED",     0,  0,  ""},
    {"EV_DROP",             0,  0,  ""},
    {"EV_TX_READY",         0,  0,  ""},
    {"EV_TIMEOUT",          0,  0,  ""},
    {"EV_STOPPED",          0,  0,  ""},
    // internal
    {NULL, 0, 0, ""}
};
PRIVATE const EVENT output_events[] = {
    {"EV_ON_OPEN",          0,  0,  ""},
    {"EV_ON_CLOSE",         0,  0,  ""},
    {"EV_ON_MESSAGE",       0,  0,  ""},
    {"EV_ON_FILE",          0,   0,  0},
    {NULL, 0, 0, ""}
};
PRIVATE const char *state_names[] = {
    "ST_DISCONNECTED",
    "ST_WAIT_CONNECTED",
    "ST_SESSION",
    NULL
};

PRIVATE EV_ACTION ST_DISCONNECTED[] = {
    {"EV_CONNECTED",        ac_connected,               0},
    {"EV_DISCONNECTED",     ac_disconnected,            0},
    {"EV_STOPPED",          ac_stopped,                 0},
    {0,0,0}
};
PRIVATE EV_ACTION ST_WAIT_CONNECTED[] = {
    {"EV_CONNECTED",        ac_connected,               0},
    {"EV_DISCONNECTED",     ac_disconnected,            "ST_DISCONNECTED"},
    {0,0,0}
};
PRIVATE EV_ACTION ST_SESSION[] = {
    {"EV_RX_DATA",          ac_rx_data,                 0},
    {"EV_SEND_MESSAGE",     ac_send_message,            0},
    {"EV_TIMEOUT",          ac_timeout_inactivity,      0},
    {"EV_TX_READY",         0,                          0},
    {"EV_DROP",             ac_drop,                    0},
    {"EV_DISCONNECTED",     ac_disconnected,            "ST_DISCONNECTED"},
    {0,0,0}
};

PRIVATE EV_ACTION *states[] = {
    ST_DISCONNECTED,
    ST_WAIT_CONNECTED,
    ST_SESSION,
    NULL
};

PRIVATE FSM fsm = {
    input_events,
    output_events,
    state_names,
    states,
};

/***************************************************************************
 *              GClass
 ***************************************************************************/
/*---------------------------------------------*
 *              Local methods table
 *---------------------------------------------*/
PRIVATE LMETHOD lmt[] = {
    {0, 0, 0}
};

/*---------------------------------------------*
 *              GClass
 *---------------------------------------------*/
PRIVATE GCLASS _gclass = {
    0,  // base
    GCLASS_DECODER_RECEIVE_FILE_NAME,
    &fsm,
    {
        mt_create,
        0, //mt_create2,
        mt_destroy,
        mt_start,
        mt_stop,
        0, //mt_play,
        0, //mt_pause,
        mt_writing,
        0, //mt_reading,
        0, //mt_subscription_added,
        0, //mt_subscription_deleted,
        0, //mt_child_added,
        0, //mt_child_removed,
        0, //mt_stats,
        0, //mt_command_parser,
        0, //mt_inject_event,
        0, //mt_create_resource,
        0, //mt_list_resource,
        0, //mt_update_resource,
        0, //mt_delete_resource,
        0, //mt_add_child_resource_link
        0, //mt_delete_child_resource_link
        0, //mt_get_resource
        0, //mt_state_changed,
        0, //mt_authenticate,
        0, //mt_list_childs,
        0, //mt_stats_updated,
        0, //mt_disable,
        0, //mt_enable,
        0, //mt_trace_on,
        0, //mt_trace_off,
        0, //mt_gobj_created,
        0, //mt_future33,
        0, //mt_future34,
        0, //mt_publish_event,
        0, //mt_publication_pre_filter,
        0, //mt_publication_filter,
        0, //mt_authz_checker,
        0, //mt_authzs,
        0, //mt_create_node,
        0, //mt_update_node,
        0, //mt_delete_node,
        0, //mt_link_nodes,
        0, //mt_link_nodes2,
        0, //mt_unlink_nodes,
        0, //mt_unlink_nodes2,
        0, //mt_get_node,
        0, //mt_list_nodes,
        0, //mt_shoot_snap,
        0, //mt_activate_snap,
        0, //mt_list_snaps,
        0, //mt_treedbs,
        0, //mt_treedb_topics,
        0, //mt_topic_desc,
        0, //mt_topic_links,
        0, //mt_topic_hooks,
        0, //mt_node_parents,
        0, //mt_node_childs,
        0, //mt_node_instances,
        0, //mt_save_node,
        0, //mt_topic_size,
        0, //mt_future62,
        0, //mt_future63,
        0, //mt_future64
    },
    lmt,
    tattr_desc,
    sizeof(PRIVATE_DATA),
    0,  // s_user_authz_level
    s_user_trace_level,
    0,  // cmds
    0,  // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_decoder_receive_file(void)
{
    return &_gclass;
}

/****************************************************************************
 *          YUNO_RECEIVE_FILE.H
 *          Receive_file yuno.
 *
 *          Copyright (c) 2023 yuneta.
 *          All Rights Reserved.
 ****************************************************************************/
#pragma once

#include <yuneta.h>

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************
 *              Constants
 ***************************************************************/
#define GCLASS_YUNO_RECEIVE_FILE_NAME "YReceive_file"
#define ROLE_RECEIVE_FILE "receive_file"

/***************************************************************
 *              Prototypes
 ***************************************************************/
PUBLIC void register_yuno_receive_file(void);

#ifdef __cplusplus
}
#endif

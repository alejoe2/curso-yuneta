/****************************************************************************
 *          C_RECEIVE_FILE.H
 *          Receive_file GClass.
 *
 *          Receive a file multiple times
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
#define GCLASS_RECEIVE_FILE_NAME "Receive_file"
#define GCLASS_RECEIVE_FILE gclass_receive_file()

/***************************************************************
 *              Prototypes
 ***************************************************************/
PUBLIC GCLASS *gclass_receive_file(void);

#ifdef __cplusplus
}
#endif

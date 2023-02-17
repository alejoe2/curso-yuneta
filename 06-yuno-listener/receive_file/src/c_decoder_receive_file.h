/****************************************************************************
 *          C_DECODER_RECEIVE_FILE.H
 *          Decoder_receive_file GClass.
 *
 *          Decoder multiple files
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
#define GCLASS_DECODER_RECEIVE_FILE_NAME "Decoder_receive_file"
#define GCLASS_DECODER_RECEIVE_FILE gclass_decoder_receive_file()

/***************************************************************
 *              Prototypes
 ***************************************************************/
PUBLIC GCLASS *gclass_decoder_receive_file(void);

#ifdef __cplusplus
}
#endif

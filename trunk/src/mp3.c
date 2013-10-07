/*
 *  Name:    mp3.c
 *
 *  Purpose: the encapsulation of Helix MP3 decoder
 *
 *  Created By:         Lipeng<runangaozhong@163.com>
 *  Created Date:       2013-10-05
 *
 *  ChangeList:
 *  Created in 2013-10-05 by Lipeng;
 *
 *  This document and the information contained in it is confidential and 
 *  proprietary to Unication Co., Ltd. The reproduction or disclosure, in 
 *  whole or in part, to anyone outside of Unication Co., Ltd. without the 
 *  written approval of the President of Unication Co., Ltd., under a 
 *  Non-Disclosure Agreement, or to any employee of Unication Co., Ltd. who 
 *  has not previously obtained written authorization for access from the 
 *  individual responsible for the document, will have a significant 
 *  detrimental effect on Unication Co., Ltd. and is expressly prohibited. 
 *  
 */
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "mp3dec.h"
#include "mp3.h"

/*========================================================
 *                  Macros, Variables
 *======================================================*/
#define MP3_AUDIO_BUF_SZ    (5 * 1024)  /* input buffer size */

#define MP3_DECODE_BUF_SZ   (2560)      /* output buffer size */

static uint8_t              mp3_fd_buffer[MP3_AUDIO_BUF_SZ];

/* 
 * double buffer used by MP3 decoder
 */
static int16_t              decode_buf0[MP3_DECODE_BUF_SZ];
static int16_t              decode_buf1[MP3_DECODE_BUF_SZ];
static volatile uint8_t     buf_switch = 0;

/*========================================================
 *          Private functions
 *======================================================*/
static int32_t mp3_decoder_fill_buffer(struct mp3_decoder *decoder) {
    uint32_t bytes_read;
    uint32_t bytes_to_read;

    if (decoder->bytes_left > 0) {
        memmove(decoder->read_buffer, decoder->read_ptr, decoder->bytes_left);
    }

    bytes_to_read = (MP3_AUDIO_BUF_SZ - decoder->bytes_left) & ~(512 - 1);

    bytes_read = decoder->fetch_data(decoder->fetch_parameter,
                                     (uint8_t *)(decoder->read_buffer + decoder->bytes_left),
                                     bytes_to_read);

    if (bytes_read != 0) {
        decoder->read_ptr = decoder->read_buffer;
        decoder->read_offset = 0;
        decoder->bytes_left = decoder->bytes_left + bytes_read;

        return 0;
    } else {
        /* can't read more data */

        return -1;
    }
}

/*========================================================
 *                  public functions
 *======================================================*/
void mp3_decoder_init(struct mp3_decoder *decoder) {
    /* init read session */
    decoder->read_ptr           = NULL;
    decoder->bytes_left         = 0;
    decoder->frames             = 0;


    decoder->read_buffer        = &mp3_fd_buffer[0];

    decoder->decoder            = MP3InitDecoder();

    buf_switch                  = 0;
}

void mp3_decoder_detach(struct mp3_decoder *decoder) {
    /* release mp3 decoder */
    MP3FreeDecoder(decoder->decoder);
}

struct mp3_decoder *mp3_decoder_create(void) {
    struct mp3_decoder *decoder;

    /* allocate object */
    decoder = (struct mp3_decoder *)malloc(sizeof(struct mp3_decoder));
    if (decoder != NULL) {
        mp3_decoder_init(decoder);
    }

    return decoder;
}

void mp3_decoder_delete(struct mp3_decoder *decoder) {
    /* de-init mp3 decoder object */
    mp3_decoder_detach(decoder);

    /* release this object */
    free(decoder);

    decoder = NULL;
}

int mp3_decoder_run(struct mp3_decoder *decoder) {
    int             err;
    int             i;

    int             outputSamps;

    int16_t         *buffer;

    if (decoder->read_ptr == NULL
        || decoder->bytes_left < 2 * MAINBUF_SIZE) {
        if (mp3_decoder_fill_buffer(decoder) != 0) {
            return -1;
        }
    }

    decoder->read_offset = MP3FindSyncWord(decoder->read_ptr, decoder->bytes_left);
    if (decoder->read_offset < 0) {
        /* outof sync, discard this data */

        decoder->bytes_left = 0;
        return 0;
    }

    decoder->read_ptr   += decoder->read_offset;
    decoder->bytes_left -= decoder->read_offset;
    if (decoder->bytes_left < 1024) {
        /* fill more data */
        if (mp3_decoder_fill_buffer(decoder) != 0) {
            return -1;
        }
    }

    /* get a decoder buffer */
    if (buf_switch == 0) {
        buffer      = decode_buf1;
        buf_switch  = 1;
    } else {
        buffer      = decode_buf0;
        buf_switch  = 0;
    }
    
    err = MP3Decode(decoder->decoder, &decoder->read_ptr,
                    (int *)&decoder->bytes_left, (short *)buffer, 0);

    decoder->frames++;

    if (err != ERR_MP3_NONE) {
        switch (err) {
            case ERR_MP3_INDATA_UNDERFLOW:
                decoder->bytes_left = 0;
                if (mp3_decoder_fill_buffer(decoder) != 0) {
                    return -1;
                }
                break;

            case ERR_MP3_MAINDATA_UNDERFLOW:
                /* do nothing - next call to decode will provide more mainData */
                break;

            default:
                /* unknown error: %d, left: %d\n", err, decoder->bytes_left */

                if (decoder->bytes_left > 0) {
                    decoder->bytes_left--;
                    decoder->read_ptr++;
                } else {
                    return -1;
                }
                break;
        }
    } else {
        /* no error */
        MP3GetLastFrameInfo(decoder->decoder, &decoder->frame_info);

        /* write to sound device */
        outputSamps = decoder->frame_info.outputSamps;
        if (outputSamps > 0) {
            if (decoder->frame_info.nChans == 1) {
                for (i = outputSamps - 1; i >= 0; i--) {
                    buffer[i * 2]       = buffer[i];
                    buffer[i * 2 + 1]   = buffer[i];
                }
                outputSamps *= 2;
            }

            /* call the callback funtion */
            decoder->output_cb(&decoder->frame_info, buffer, outputSamps);
        }
    }

    return 0;
}
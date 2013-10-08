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
#include "sonic.h"
#include "bpm.h"

/*========================================================
 *                  Macros, Variables
 *======================================================*/
#define MP3_AUDIO_BUF_SZ    (8 * 1024)  /* input buffer size */

//#define MP3_DECODE_BUF_SZ   (2560)      /* output buffer size */
#define MP3_DECODE_BUF_SZ   (4096)      /* output buffer size */

static uint8_t              mp3_fd_buffer[MP3_AUDIO_BUF_SZ];

/* 
 * double buffer used by MP3 decoder
 */
static int16_t              decode_buf0[MP3_DECODE_BUF_SZ];
static int16_t              decode_buf1[MP3_DECODE_BUF_SZ];
static volatile uint8_t     buf_switch = 0;

static int16_t              tmp_buf[MP3_DECODE_BUF_SZ];

static int                  cur_srate = 0;
static int                  cur_channel = 0;

volatile float              cur_ratio = 1;

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

int mp3_decoder_run_internal(struct mp3_decoder *decoder,
                             int16_t *buffer) {
    int             err;
    int             i;

    int             outputSamps;

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
        }

        return outputSamps;
    }

    return 0;
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

/*
 * ret: 0, decoder is running
 *      -1, some error occuerd, should stop the decoding
 */
int mp3_decoder_run(struct mp3_decoder *decoder) {
    int16_t         *buffer;
    int             len = 0;

    /* get a decoder buffer */
    if (buf_switch == 0) {
        buffer      = decode_buf1;
        buf_switch  = 1;
    } else {
        buffer      = decode_buf0;
        buf_switch  = 0;
    }
    
    if ((len = mp3_decoder_run_internal(decoder, buffer)) > 0) {
        /* call the callback funtion */
        decoder->output_cb(&decoder->frame_info, buffer, len);
        return 0;
    } else {
        return len;
    }
}

void mp3_set_speed(float speed) {
    cur_ratio = speed;
}

int mp3_decoder_run_pvc(struct mp3_decoder *decoder) {
    static sonicStream  stream = NULL;

    int16_t             *buffer;

    int                 len;
    static int          buf_pos = 0;

    /* get a decoder buffer */
    if (buf_switch == 0) {
        buffer      = decode_buf1;
        buf_switch  = 1;
    } else {
        buffer      = decode_buf0;
        buf_switch  = 0;
    }

reread:
    if (stream != NULL) {
        len = sonicReadShortFromStream(stream, &buffer[buf_pos],
                                       (MP3_DECODE_BUF_SZ - buf_pos) / cur_channel);
        if (len == (MP3_DECODE_BUF_SZ - buf_pos) / cur_channel) {
            /* call the callback funtion */
            decoder->output_cb(&decoder->frame_info, buffer, MP3_DECODE_BUF_SZ);
            buf_pos = 0;
            return 0;
        } else if (len > 0) {
            buf_pos += len * cur_channel;
        }
    }

    /* cannot get enough data, write some samples to Sonic */
    if ((len = mp3_decoder_run_internal(decoder, tmp_buf)) > 0) {
        /* call the callback funtion */
        if (stream == NULL
            || cur_srate != decoder->frame_info.samprate
            || cur_channel != 2) {

            cur_srate   = decoder->frame_info.samprate;
            cur_channel = 2;

            if (stream) sonicDestroyStream(stream);
            stream = sonicCreateStream(cur_srate, 2);
            if (stream == NULL) {
                return -1;
            }
        }
        sonicSetSpeed(stream, cur_ratio);
	    sonicWriteShortToStream(stream, tmp_buf, len / 2);
    } else if (len == -1) {
        /* some thing error, destroy the sonic stream */
        if (stream) {
            sonicDestroyStream(stream);
            stream = NULL;
        }
        buf_pos = 0;
        return -1;
    }

    goto reread;
}

/*
 * BPM detection
 */
int mp3_bpm_detect_run(struct mp3_decoder *decoder) {
    uint8_t         bpm_init_flag = 0;

    uint16_t        bpm_num_samples = 0;
    uint16_t        bpm_step = 0;

    uint16_t        bpm = 0;

    int             len;

    int             pos = 0;
    int             left = 0;

    while ((len = mp3_decoder_run_internal(decoder, tmp_buf)) != -1) {
        if (cur_srate != decoder->frame_info.samprate
            || cur_channel != decoder->frame_info.nChans
            || bpm_init_flag == 0) {

            cur_srate   = decoder->frame_info.samprate;
            cur_channel = 2;

            BPM_release();
            BPM_init(cur_srate, cur_channel);

            BPM_set_freq_band(0, 4000);
            bpm_num_samples = BPM_num_of_samples();
            bpm_step = bpm_num_samples * cur_channel;

            bpm_init_flag = 1;
        }

        left += len;
        for (pos = 0; pos < left - bpm_step; pos += bpm_step) {
            if (BPM_put_samples(&tmp_buf[pos], bpm_num_samples) == 1) {
                /* BPM detect is done, get the value */
                bpm             = BPM_get_bpm();

                /* release the BPM module */
                bpm_init_flag   = 0;
                BPM_release();
                return bpm;
            }
        }
        left -= pos;
    }
    
    return 0;
}

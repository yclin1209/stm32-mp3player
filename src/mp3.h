/*
 *  Name:    mp3.h
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

#ifndef _MP3_H_
#define _MP3_H_

struct mp3_decoder {
    /* mp3 information */
    HMP3Decoder     decoder;
    MP3FrameInfo    frame_info;
    uint32_t        frames;

    /* mp3 file descriptor */
    uint32_t        (*fetch_data)(void *parameter,
                                  uint8_t *buffer,
                                  uint32_t length);
    void            *fetch_parameter;

    /* mp3 read session */
    uint8_t         *read_buffer, *read_ptr;
    int32_t         read_offset;
    uint32_t        bytes_left;

    /* 
     * This is the output callback function.
     * It is called after each frame of MPEG audio data
     * has been completely decoded.
     * The purpose of this callback is
     * to output (or play) the decoded PCM audio.
     */
    uint32_t        (*output_cb)(MP3FrameInfo *header,
                                 int16_t *buffer,
                                 uint32_t length);
};

void mp3_decoder_init(struct mp3_decoder *decoder);
void mp3_decoder_detach(struct mp3_decoder *decoder);
struct mp3_decoder *mp3_decoder_create(void);
void mp3_decoder_delete(struct mp3_decoder *decoder);
int mp3_decoder_run(struct mp3_decoder *decoder);

void mp3_set_speed(float speed);
int mp3_decoder_run_pvc(struct mp3_decoder *decoder);

int mp3_bpm_detect_run(struct mp3_decoder *decoder);
#endif

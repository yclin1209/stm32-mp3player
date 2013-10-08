/*
 *  Name:    main.c
 *
 *  Purpose: the main function of MP3 Player
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
#include <string.h>

#include "main.h"
#include "core_cm4.h"
#include "stm32f4xx_conf.h"
#include "Audio.h"
#include "mp3dec.h"
#include "mp3.h"

/*========================================================
 *                  Macros, Variables
 *======================================================*/
#define BUTTON			    (GPIOA->IDR & GPIO_Pin_0)

USB_OTG_CORE_HANDLE         USB_OTG_Core;
USBH_HOST                   USB_Host;
volatile int			    enum_done = 0;

static volatile uint32_t    time_var1, time_var2;
static RCC_ClocksTypeDef    RCC_Clocks;

static FIL                  file;

static volatile uint8_t     audio_is_playing = 0;


/*========================================================
 *          Private functions
 *======================================================*/

/* Called by the audio driver when its DMA complete */
static void AudioCallback(void) {
    audio_is_playing = 0;
}

/* MP3 file read, provided to MP3 decoder */
static uint32_t fd_fetch(void *parameter, uint8_t *buffer, uint32_t length) {
    uint32_t read_bytes = 0;

	f_read((FIL *)parameter, (void *)buffer, length, &read_bytes);
    if (read_bytes <= 0) return 0;

    return read_bytes;
}


/*
 * MP3 player
 */

static uint32_t mp3_callback(MP3FrameInfo *header,
                             int16_t *buffer,
                             uint32_t length) {
    while (audio_is_playing == 1);

    audio_is_playing = 1;
    ProvideAudioBuffer(buffer, length);

    return 0;
}

static void play_mp3(char* filename) {
    struct mp3_decoder *decoder;

	if (FR_OK == f_open(&file, filename, FA_OPEN_EXISTING | FA_READ)) {
		/* Play mp3 */

		InitializeAudio(Audio44100HzSettings);
		SetAudioVolume(0xAF);
		PlayAudioWithCallback(AudioCallback);

        decoder = mp3_decoder_create();
        if (decoder != NULL) {
            decoder->fetch_data         = fd_fetch;
            decoder->fetch_parameter    = (void *)&file;
            decoder->output_cb          = mp3_callback;

            //while (mp3_decoder_run(decoder) != -1);
            while (mp3_decoder_run_pvc(decoder) != -1);

            /* delete decoder object */
            mp3_decoder_delete(decoder);
        }
        
        /* Re-initialize and set volume to avoid noise */
        InitializeAudio(Audio44100HzSettings);
        SetAudioVolume(0);
        
        /* reset the playing flag */
        audio_is_playing = 0;

        /* Close currently open file */
        f_close(&file);
    }
}


/*
 * play directory
 */
static const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

static FRESULT play_directory (const char* path, unsigned char seek) {
	FRESULT     res;
	FILINFO     fno;
	DIR         dir;

    /* This function is assuming non-Unicode cfg. */
	char        *fn; 
	char        buffer[200];
#if _USE_LFN
	static char lfn[_MAX_LFN + 1];

	fno.lfname  = lfn;
	fno.lfsize  = sizeof(lfn);
#endif


	res = f_opendir(&dir, path);
	if (res == FR_OK) {
		for (;;) {
            /* Read a directory item */
			res = f_readdir(&dir, &fno); 

            /* Break on error or end of dir */
			if (res != FR_OK || fno.fname[0] == 0) break; 

            /* Ignore dot entry */
			if (fno.fname[0] == '.') continue; 

        #if _USE_LFN
			fn = *fno.lfname ? fno.lfname : fno.fname;
        #else
			fn = fno.fname;
        #endif
			if (fno.fattrib & AM_DIR) {
                /* It is a directory */

			} else {
                /* It is a file. */
				sprintf(buffer, "%s/%s", path, fn);

				/* Check if it is an mp3 file */
				if (strcmp("mp3", get_filename_ext(buffer)) == 0) {

					/* Skip "seek" number of mp3 files... */
					if (seek) {
						seek--;
						continue;
					}

					play_mp3(buffer);
				}
			}
		}
	}

	return res;
}

/*========================================================
 *                  public functions
 *======================================================*/

/*
 * Main function. Called when startup code is done with
 * copying memory and setting up clocks.
 */
int main(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;

	/* SysTick interrupt each 1ms */
	RCC_GetClocksFreq(&RCC_Clocks);
	SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);

	/* GPIOD Peripheral clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	/* Configure PD12, PD13, PD14 and PD15 in output pushpull mode */
	GPIO_InitStructure.GPIO_Pin     = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	/* Initialize USB Host Library */
	USBH_Init(&USB_OTG_Core, USB_OTG_FS_CORE_ID, &USB_Host, &USBH_MSC_cb, &USR_Callbacks);

	for(;;) {
		USBH_Process(&USB_OTG_Core, &USB_Host);

		if (enum_done >= 2) {
			enum_done = 0;
			play_directory("", 0);
		}
	}
}

/*
 * Called by the SysTick interrupt
 */
void TimingDelay_Decrement(void) {
	if (time_var1) {
		time_var1--;
	}
	time_var2++;
}

/*
 * Delay a number of systick cycles (1ms)
 */
void Delay(volatile uint32_t nTime) {
	time_var1 = nTime;
	while(time_var1){};
}

/*
 * Dummy function to avoid compiler error
 */
void _init() {
}

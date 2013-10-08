/*
 *  "Beat per minute" detection
 *
 *  Ported from libzplay, which is a open source AUdio player
 *  library for WIN32 platform. Please refer to
 *  "http://libzplay.sourceforge.net/" to get the detail info
 *
 *  Copyright (C) 2013 Lipeng<runangaozhong@163.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "bpm_pri.h"

#include "bpm.h"

#include "debug.h"

#include "fft.h"

#ifdef CONFIG_DEBUG
static int                          debug = DEBUG_LEVEL_INFO;
#endif


/*========================================================
 *          The constant definition for this module
 *======================================================*/

/* number of fft points for analyse */
#define BPM_DETECT_FFT_POINTS       64      /* 64 */


/* buffer size in milliseconds */
#define BPM_BUFFER_SIZE             1000    /* 4000 */

/* window size in millisecods */
#define BPM_WINDOW_SIZE             500     /* 2000 */

/* overlap size in milliseconds */
#define BPM_OVERLAP_SUCCESS_SIZE    500     /* 2000 */
#define BPM_OVERLAP_FAIL_SIZE       875     /* 3500 */

/* number of subbands */
#define BPM_NUMBER_OF_SUBBANDS      8       /* 16 */

#define SUBBAND_SIZE                (BPM_DETECT_FFT_POINTS / (2 * BPM_NUMBER_OF_SUBBANDS))


#define BPM_HISTORY_TOLERANCE       1
#define BPM_HISTORY_HIT             5

/* left safe margim used by autocorrelation */
#define BPM_DETECT_MIN_MARGIN       1
/* right safe margin used by autocorrelation */
#define BPM_DETECT_MAX_MARGIN       1

#define BPM_DETECT_MIN_BPM          55
#define BPM_DETECT_MAX_BPM          200

/* 
 * (BPM_BUFFER_SIZE - BPM_WINDOW_SIZE) / 1000
 *  * (BPM_DETECT_MIN_BPM - BPM_DETECT_MIN_MARGIN - 1)
 */
#define OFFSET_FACTOR               26      
/*========================================================
 *          The internal structure
 *======================================================*/
struct subband_t{
    u16     BPM;
    u16     *BPM_history_hit; 
    u32     buffer_load;
    REAL    *buffer;
};

struct BPM_mod_t {
    u16     BPM;
    u32     hit;
    u32     sum;
};




/*========================================================
 *          The static definition for this module
 *======================================================*/

static u8               c_ready = 0;

static u16              c_low_limit_index = 0;
static u16              c_high_limit_index = BPM_NUMBER_OF_SUBBANDS;
static u16              c_band_size_index = BPM_NUMBER_OF_SUBBANDS;

static u32              c_buffer_size;
static u32              c_window_size;
static u32              c_overlap_success_size;
static u32              c_overlap_fail_size;
static u32              c_overlap_success_start;
static u32              c_overlap_fail_start;

/* number of subbands with detected BPM */
static u32              c_subband_BPM_detected;

static REAL             c_sqrt_FFT_points;

static u32              c_channel = 0;
static u32              c_sample_rate = 0;

/* subbands array */
static struct subband_t *c_pSubBand;

/* array of amplitudes returned from FFT analyse */
static REAL             *c_amplitudes;

/* array of samples sent to FFT analyse */

complex_t               *c_samples;

static REAL             *c_pfl_window;      /* FFT window */

static u16              *c_pn_offset;



/*========================================================
 *              The internal interfaces 
 *======================================================*/
int free_internal_mem() {
	if (c_amplitudes) {
		free(c_amplitudes);
		c_amplitudes = 0;
	}

	if (c_samples) {
		free(c_samples);
		c_samples = 0;
	}

	if (c_pfl_window) {
		free(c_pfl_window);
		c_pfl_window = 0;
	}

	if(c_pn_offset) {
		free(c_pn_offset);
		c_pn_offset = 0;
	}


	if (c_pSubBand) {
		unsigned int i;
		for (i = 0; i < BPM_NUMBER_OF_SUBBANDS; i++) {
			if (c_pSubBand[i].buffer) {
				free(c_pSubBand[i].buffer);
				c_pSubBand[i].buffer = 0;
            }

			if (c_pSubBand[i].BPM_history_hit) {
				free(c_pSubBand[i].BPM_history_hit);
				c_pSubBand[i].BPM_history_hit = 0;
            }

		}

		free(c_pSubBand);
		c_pSubBand = 0;
	}

	c_ready = 0;

	return 0;
}

static int allocate_internal_mem(void) {
    u32 i;

	c_amplitudes = (REAL *)malloc(BPM_DETECT_FFT_POINTS / 2 * sizeof(REAL));
	c_samples = (complex_t *)malloc(BPM_DETECT_FFT_POINTS * sizeof(complex_t));
	c_pfl_window = (REAL *)malloc(BPM_DETECT_FFT_POINTS * sizeof(REAL));
	c_pn_offset = (u16 *)malloc((BPM_DETECT_MAX_BPM + BPM_DETECT_MAX_MARGIN + 2) * sizeof(u16));

	if (c_amplitudes == 0 || c_samples == 0 || c_pfl_window == 0 || c_pn_offset == 0) {
		free_internal_mem();
		return -1;
	}

	memset(c_amplitudes, 0, BPM_DETECT_FFT_POINTS / 2 * sizeof(REAL));
	memset(c_samples, 0, BPM_DETECT_FFT_POINTS * sizeof(complex_t));
	memset(c_pfl_window, 0, BPM_DETECT_FFT_POINTS * sizeof(REAL));
	memset(c_pn_offset, 0, (BPM_DETECT_MAX_BPM + BPM_DETECT_MAX_MARGIN + 2) * sizeof(u16));

    c_pSubBand = (struct subband_t *)malloc(BPM_NUMBER_OF_SUBBANDS * sizeof(struct subband_t));
	if (c_pSubBand == 0) {
		free_internal_mem();
		return -1;
	}
	memset(c_pSubBand, 0, BPM_NUMBER_OF_SUBBANDS * sizeof(struct subband_t));

    for (i = 0; i < BPM_NUMBER_OF_SUBBANDS; i++) {
        c_pSubBand[i].buffer = (REAL *)malloc(c_buffer_size * sizeof(REAL));
        if (c_pSubBand[i].buffer == 0) {
		    free_internal_mem();
            return -1;
        }
        memset(c_pSubBand[i].buffer, 0, c_buffer_size * sizeof(REAL));

	    c_pSubBand[i].BPM_history_hit = (u16 *)malloc((BPM_DETECT_MAX_BPM + 2) * sizeof(u16));
		if (c_pSubBand[i].BPM_history_hit == 0) {
		    free_internal_mem();
            return -1;
		}

    }

    /* 
     * create COSINE window
     */
    for (i = 0; i < BPM_DETECT_FFT_POINTS; i++) {
        c_pfl_window[i] = (REAL)(sin((PI * i) / (BPM_DETECT_FFT_POINTS - 1.0))); 
    }
    DEBUG("c_pfl_window:\n");
    for (i = 1; i < BPM_DETECT_FFT_POINTS; i++) {
        if (i % 5 == 0) DEBUG("\n");
        DEBUG("%f, ", c_pfl_window[i]);
    }
    DEBUG("\n");


    c_ready = 1;

    return 0;
}



/*========================================================
 *          The interfaces provided to others
 *======================================================*/

/*
 * initialize of BPM detetion algorithm
 *
 * PARAMETERS:
 *     sample_rate
 *         Sample rate. Optimized for sample rate 44100 Hz.
 *
 *     channel
 *         Number of channels. Stereo is converted to mono.
 *
 * RETURN VALUES:
 *     0    - all OK, class initialized
 *     -1   - memory allocation error
 *
 * REMARKS:
 *     Call this function before you put samples.
 *     Also call this function before processing new song
 *     to clear internal buffers and initialize all to starting positions.
 *
 */
int BPM_init(u32 sample_rate, u32 channel) {
    u32 i;
    struct subband_t *subband;

    if (sample_rate == 0 || channel == 0) {
        return -1;
    }

    /*
     * Init the global constant variables
     * according to sample rate and channel
     */

    c_buffer_size = (sample_rate * BPM_BUFFER_SIZE)
                    / (1000 * BPM_DETECT_FFT_POINTS);
    if (c_buffer_size == 0) {
        c_buffer_size = 1;
    }

    c_window_size = (sample_rate * BPM_WINDOW_SIZE)
                    / (1000 * BPM_DETECT_FFT_POINTS);
    if (c_window_size == 0) {
        c_window_size = 1;
    }

    c_overlap_success_size = (sample_rate * BPM_OVERLAP_SUCCESS_SIZE)
                              / (1000 * BPM_DETECT_FFT_POINTS);
    if (c_overlap_success_size >= c_buffer_size) {
        c_overlap_success_size = c_buffer_size - 1;    
    }

    c_overlap_fail_size = (sample_rate * BPM_OVERLAP_FAIL_SIZE)
                           / (1000 * BPM_DETECT_FFT_POINTS);
    if (c_overlap_fail_size >= c_buffer_size) {
        c_overlap_fail_size = c_buffer_size - 1;    
    }

    c_overlap_success_start = c_buffer_size - c_overlap_success_size;
    c_overlap_fail_start    = c_buffer_size - c_overlap_fail_size;
    c_subband_BPM_detected  = 0;

    c_sqrt_FFT_points       = sqrt(BPM_DETECT_FFT_POINTS);
    DEBUG("c_sqrt_FFT_points: %f\n", c_sqrt_FFT_points);

    if (c_ready == 0
        || c_sample_rate != sample_rate
        || c_channel != channel) {

        if (allocate_internal_mem() < 0) {
            return -1;
        }

        c_sample_rate   = sample_rate;
        c_channel       = channel;

        for (i = BPM_DETECT_MIN_BPM - BPM_DETECT_MIN_MARGIN - 1;
             i <= BPM_DETECT_MAX_BPM + BPM_DETECT_MAX_MARGIN + 1;
             i++) {
            c_pn_offset[i] = (u16)(((double)(OFFSET_FACTOR * c_sample_rate)
                             / (double)(i * BPM_DETECT_FFT_POINTS)) + 0.5);
        }

    }

    for (i = 0; i < BPM_NUMBER_OF_SUBBANDS; i++) {
        subband                     = &c_pSubBand[i];
        subband->buffer_load        = 0;
        subband->BPM                = 0;
        memset(subband->BPM_history_hit, 0,
               (BPM_DETECT_MAX_BPM + 2) * sizeof(u16));
    }


    return 0;
}

/*
 * set the range of frequency band
 *
 * parameters:
 *     low_limit
 *        the low limit of freqency band
 *     high_limit
 *        the high limit of freqency band
 *
 * return values:
 *     return 0 if success, or return -1
 *
 * remarks:
 *     none
 *
 */
int BPM_set_freq_band(u32 low_limit, u32 high_limit) {
    /* calculate low index */
    double subband      = (double)c_sample_rate / (double)(BPM_NUMBER_OF_SUBBANDS * 2);

    c_low_limit_index   = (u16)((double)low_limit / (double)subband);
    c_high_limit_index  = (u16)((double)high_limit / (double)subband);
    if (c_high_limit_index > BPM_NUMBER_OF_SUBBANDS) {
        c_high_limit_index = BPM_NUMBER_OF_SUBBANDS;    
    }

    c_band_size_index = c_high_limit_index - c_low_limit_index;
    DEBUG("c_band_size_index: %d\n", c_band_size_index);

    return 0;
}

/*
 * get number of samples needed for put_samples function.
 *
 * PARAMETERS:
 *     None
 *
 * RETURN VALUES:
 *     number of samples needed for BPM_put_samples function.
 *
 * REMARKS:
 *     Use this function to get number of samples you need
 *     to feed with BPM_put_samples function
 *
 */
u32 BPM_num_of_samples(void) {
    return BPM_DETECT_FFT_POINTS;
}

/*
 * put samples into processing
 *
 * PARAMETERS:
 *     samples
 *         Pointer to buffer with samples. Supports only 16 bit samples
 *
 *     sample_num
 *         Number of samples in buffer.
 *
 * RETURN VALUES:
 *     1    - detecting is done, we have BPM, we don't need more data.
 *     0    - we need more data to detect BPM
 *
 * REMARKS:
 *     Call this function with new samples until function returns 1
 *     or you are out of data.
 *     If function returns 1, detection is done and we have BPM value,
 *     so we don't need more data.
 */
int BPM_put_samples(short *samples, u32 sample_num) {
    u16     bpm;
    u16     i, j, k;

    u32     history_hit;
    u32     avg;

    REAL    amp;
    REAL    re, im;
    REAL    instant_amplitude;

    REAL    max_correlation;
    REAL    correlation;

    struct subband_t *subband;

    DEBUG("%s\n", __func__);

#if 1
    /* 
     * create samples array
     * NOTE: I just treat samples as Q1.15, just like samples[i] / 32767.0
     */

    if (c_channel == 2) {
        /* convert to mono */
        for (i = 0; i < sample_num; i++) {
            c_samples[i].r = ((REAL)samples[2 * i] + (REAL)samples[2 * i + 1])
                             * c_pfl_window[i] / 2.0;
            c_samples[i].i = 0;
            DEBUG("%f, %d, %d, %f\n", c_pfl_window[i], samples[2 * i],
                                      samples[2 * i + 1], creal(samples[i]));
        }
    } else {
        for (i = 0; i < sample_num; i++) {
            c_samples[i].r = (REAL)samples[i] * c_pfl_window[i];
            c_samples[i].i = 0;
            DEBUG("%f, %d, %f\n", c_pfl_window[i], samples[i], c_samples[i].r);
        }
    }


    /* make fft */
    DEBUG("before fft transfer\n");
    for (i = 1; i < sample_num; i++) {
        if (i % 5 == 0) DEBUG("\n");
        DEBUG("%f, ", c_samples[i].r);
    }                     
    DEBUG("\n");

    fft(c_samples, BPM_DETECT_FFT_POINTS);

    DEBUG("after fft transfer\n");
    DEBUG("Real/Image:\n");
    for (i = 1; i < BPM_DETECT_FFT_POINTS / 2; i++) {
        if (i % 5 == 0) DEBUG("\n");
        DEBUG("%f/%f, ", c_samples[i].r,
                         c_samples[i].i);
    }
    DEBUG("\n");

    re = c_samples[BPM_DETECT_FFT_POINTS / 2].r;
    im = 0;
    DEBUG("re = %f\n", re);

    amp = sqrt(re * re + im * im) / c_sqrt_FFT_points;
    c_amplitudes[BPM_DETECT_FFT_POINTS / 2 - 1] = amp * amp;

    for (i = 1; i < BPM_DETECT_FFT_POINTS / 2; i++) {
        re = c_samples[i].r;
        im = c_samples[i].i;

        amp = sqrt(re * re + im * im) / c_sqrt_FFT_points;
        c_amplitudes[i - 1] = amp * amp;
    }

    for (i = c_low_limit_index; i < c_high_limit_index; i++) {
        subband = &c_pSubBand[i];

        if (subband->BPM) {     /* we have BPM, don't compute anymore */
            continue;
        }
    
        /* 
         * CREATE SUBBANDS
         */
        instant_amplitude = 0;

        for (j = i * SUBBAND_SIZE; j < (i + 1) * SUBBAND_SIZE; j++) {
            instant_amplitude += c_amplitudes[j];
        }

        instant_amplitude /= (REAL)SUBBAND_SIZE;

        /* 
         * FILL BUFFER
         */
        subband->buffer[subband->buffer_load] = instant_amplitude;

        subband->buffer_load++;
        if (subband->buffer_load < c_buffer_size) {
            continue;
        }


        /* calculate auto correlation of each BPM in range */
        bpm = 0;
        max_correlation = 0;

        for (j = BPM_DETECT_MIN_BPM - BPM_DETECT_MIN_MARGIN - 1;
             j <= BPM_DETECT_MAX_BPM + BPM_DETECT_MAX_MARGIN;
             j++) {
            correlation = 0;

            for (k = 0; k < c_window_size; k++) {
                correlation += (subband->buffer[k] * subband->buffer[k + c_pn_offset[j]]);
            }

            if (correlation >= max_correlation) {
                max_correlation = correlation;    
                bpm = j;
            }
        }

        /* mark bpm in history */
        if (bpm >= BPM_DETECT_MIN_BPM && bpm <= BPM_DETECT_MAX_BPM) {    
            history_hit = subband->BPM_history_hit[bpm]
                          + subband->BPM_history_hit[bpm - 1]
                          + subband->BPM_history_hit[bpm + 1];
            if (history_hit) {
                if (history_hit >= BPM_HISTORY_HIT) {
                    avg = (bpm * subband->BPM_history_hit[bpm]
                          + (bpm - 1) * subband->BPM_history_hit[bpm - 1]
                          + (bpm + 1) * subband->BPM_history_hit[bpm + 1])
                            / history_hit;
                    subband->BPM = avg;
                    c_subband_BPM_detected++;
                }
            }

            subband->BPM_history_hit[bpm]++;

            for (j = 0 ; j < c_overlap_success_size; j++) {
                subband->buffer[j] = subband->buffer[j + c_overlap_success_start];
            }

            subband->buffer_load = c_overlap_success_size;
            
        } else {
            for (j = 0 ; j < c_overlap_fail_size; j++) {
                subband->buffer[j] = subband->buffer[j + c_overlap_fail_start];
            }

            subband->buffer_load = c_overlap_fail_size;
        }
    }

    if (c_subband_BPM_detected == c_band_size_index) {
        return 1;
    }
#endif

    return 0;
}


/*
 * get BPM value
 *
 * PARAMETERS:
 *     None
 *
 * RETURN VALUES:
 *     BPM value.
 *     This value can be 0 if detection fails or song has no beat.
 *
 * REMARKS:
 *     There can be case that class can't detect beat value.
 *     Maybe is song too short, or has no beat, or is too much noise
 */
u32 BPM_get_bpm(void) {
    u16 BPM;
    u16 i, j;

    u32 size;
    u32 have;
    u32 max_hit;
    struct BPM_mod_t mod_value[BPM_NUMBER_OF_SUBBANDS];
    
    for (i = c_low_limit_index; i < c_high_limit_index; i++) {
        DEBUG("Subband: %02u   %u\n", i, c_pSubBand[i].BPM);
    }
    
    /* get BPM by calculating mod value */
    memset(mod_value, 0, BPM_NUMBER_OF_SUBBANDS * sizeof(struct BPM_mod_t));

    /* search unique values */
    size = 0;
    for (i = c_low_limit_index; i < c_high_limit_index; i++) {
        have = 0;
        /* check if this value is already in */
        for (j = 0; j < size; j++) {
            if(c_pSubBand[i].BPM != 0 && mod_value[j].BPM == c_pSubBand[i].BPM) {
                have = 1;
                break;
            }
        }

        /* we don't have value in, add new value if value is not 0 */
        if (have == 0 && c_pSubBand[i].BPM != 0) {
            mod_value[size].BPM = c_pSubBand[i].BPM;
            size++;
        }
    }

    /* calculate hit values and get value with maximal hit */
    max_hit = 0;
    BPM = 0;
    for (i = 0; i < size; i++) {
        for (j = c_low_limit_index; j < c_high_limit_index; j++) {
            if (mod_value[i].BPM == c_pSubBand[j].BPM) {
                mod_value[i].hit++;
                mod_value[i].sum += c_pSubBand[j].BPM;
            }

            if (mod_value[i].BPM + 1 == c_pSubBand[j].BPM) {
                mod_value[i].hit++;
                mod_value[i].sum += c_pSubBand[j].BPM;
            }

            if (mod_value[i].BPM - 1 == c_pSubBand[j].BPM) {
                mod_value[i].hit++;
                mod_value[i].sum += c_pSubBand[j].BPM;
            }

            if (mod_value[i].hit > max_hit) {
                max_hit = mod_value[i].hit;
                BPM = (u16)(((double)mod_value[i].sum / (double)mod_value[i].hit) + 0.5);
            }
        }
    }

    return BPM;
}


/*
 * de-initialize of BPM detetion algorithm
 *
 * PARAMETERS:
 *     None
 *
 * RETURN VALUES:
 *     None
 *
 * REMARKS:
 *     Call this function if you do not need the BPM detection algorythm
 *
 */
void BPM_release(void) {
    free_internal_mem();
}



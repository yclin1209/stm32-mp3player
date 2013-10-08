/*
 * "Beat per minute" detection
 * 
 * Ported from libzplay, which is a open source AUdio player
 * library for WIN32 platform. Please refer to
 * "http://libzplay.sourceforge.net/" to get the detail info
 *
 * Created By:         Lipeng <runangaozhong@163.com>
 * Created Date:       2013-08-29
 *
 * ChangeList:
 * Created in 2013-08-29 by Lipeng;
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _BPM_H_
#define _BPM_H_

#include "main.h"

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
int BPM_init(u32 sample_rate, u32 channel);

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
int BPM_set_freq_band(u32 low_limit, u32 high_limit);



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
 *     to feed with BOM_put_samples function
 *
 */
u32 BPM_num_of_samples(void);

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
 *     0    - detecting is done, we have BPM, we don't need more data.
 *     -1   - we need more data to detect BPM
 *
 * REMARKS:
 *     Call this function with new samples until function returns 0
 *     or you are out of data.
 *     If function returns 1, detection is done and we have BPM value,
 *     so we don't need more data.
 */
int BPM_put_samples(short *samples, u32 sample_num);

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
u32 BPM_get_bpm(void); 

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
void BPM_release(void);

#endif

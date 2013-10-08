/*
 *  Name:    fft.h
 *
 *  Purpose: The FFT thansfer
 *
 *  Created By:         Lipeng<runangaozhong@163.com>
 *  Created Date:       2013-06-20
 *
 *  ChangeList:
 *  Created in 2013-06-20 by Lipeng;
 *
 *  This document and the information contained in it is confidential and 
 *  proprietary to Unication Co., Ltd. The reproduction or disclosure, in 
 *  whole or in part, to anyone outside of Unication Co., Ltd. without the 
 *  written approval of the President of Unication Co., Ltd., under a 
 *  Non-Disclosure Agreement, or to any employee of Unication Co., Ltd. who 
 *  has not previously obtained written authorization for access from the 
 *  individual responsible for the document, will have a significant 
 *  detrimental effect on Unication Co., Ltd. and is expressly prohibited. 
 */
#ifndef _FFT_H_
#define _FFT_H_

typedef struct {
    float   r;
    float   i;
} complex_t;


/*
 * Purpose:
 *                     To fft (reference to the book - "the principle and application of DSP chip")
 * Input:
 *       *ptCompx_in    a complex group input this function, and after fft it will change
 *       len            the number of fft points
 *
 * Output:
 *               None
 * Return:
 *
 * None
 */
void fft(complex_t *ptCompx_in, int len);
void ifft(complex_t *ptCompx_in, int len);

#endif

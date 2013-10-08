/*
 *  Name:    fft.c
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

#include "fft.h"
#include <math.h>

#define FFT_LEN         (256)
#define FFT_HLEN        (128)

static inline complex_t complex_mul(complex_t a,
                                      complex_t b) {
    complex_t  c;
    c.r = a.r * b.r - a.i * b.i;
    c.i = a.r * b.i + a.i * b.r;

    return (c);
}

static inline complex_t complex_add(complex_t a,
                                      complex_t b) {
    complex_t  c;
    c.r = a.r + b.r;
    c.i = a.i + b.i;

    return (c);
}
 
static inline complex_t complex_sub(complex_t a,
                                      complex_t b) {
    complex_t  c;
    c.r = a.r - b.r;
    c.i = a.i - b.i;

    return (c);
}

void fft(complex_t *ptCompx_in, int len) {
    int        i, k, j, loop;
    int        loopCtl;                           /* use to control the number of loop */
    int        NumCtl;                            /* control the number of series */
    int        le, lei, ip, hlen; 
    complex_t  tCompxU, tCompxW, tCompxT;           /* three struct of complex */
    
    /* initialization */
    j  = 0;
    hlen = len / 2;
        
    /* index calculation */
    for (i = 0; i < len - 1; i++) {
        if (i < j) {
            tCompxT        = ptCompx_in[j];
            ptCompx_in[j]  = ptCompx_in[i];
            ptCompx_in[i]  = tCompxT;
        }
            
        k = hlen;               /* find the next reverse order of j */
        while (k <= j) {        /* this mean the highest bit of j is 1 */
            j = j - k;          /* change it to 0 */
            k = k / 2;
        }
        j = j + k;              /* change it from 0 to 1 */
    }
        
    /* calculation the number of loop */
    loopCtl = len;
    for (loop = 1; (loopCtl = loopCtl / 2) != 1; loop++) {
        ;
    }
        
    /* fft */   
    for (NumCtl = 1; NumCtl <= loop; NumCtl++) {
        loop         =  log2(len);
        le           = 2 << (NumCtl - 1);
        lei          = le / 2;
        tCompxU.r    = 1.0;
        tCompxU.i    = 0;

        tCompxW.r    = cos(M_PI / lei);
        tCompxW.i    = 0 - sin(M_PI / lei);
        
        for (j = 0; j <= lei - 1; j++) {
            for (i = j; i <= len - 1; i = i + le) {
                ip                  = i + lei;
                tCompxT             = complex_mul(ptCompx_in[ip], tCompxU);
                ptCompx_in[ip]      = complex_sub(ptCompx_in[i], tCompxT);
                ptCompx_in[i]       = complex_add(ptCompx_in[i], tCompxT);
            }
            tCompxU = complex_mul(tCompxU, tCompxW);
        }
    }
}

void ifft(complex_t *ptCompx_in, int len) {
    int i;
    float tmp;
    
    for (i = 0; i < len; i++) {
        tmp = ptCompx_in[i].r;
        ptCompx_in[i].r = ptCompx_in[i].i;
        ptCompx_in[i].i = tmp;
    }

    fft(ptCompx_in, len);

    for (i = 0; i < len; i++) {
        tmp = ptCompx_in[i].r;
        ptCompx_in[i].r = ptCompx_in[i].i / len;
        ptCompx_in[i].i = tmp / len;
    }
}

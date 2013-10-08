/*
 *  debug system
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

#ifndef _DEBUG_H_
#define _DEBUG_H_

/* 
 * debug control, you can switch on this (delete 'x' suffix)
 * to enable log output and assert mechanism
 */
#define CONFIG_DEBUGx

#ifdef CONFIG_DEBUG
    #include <stdio.h>

    /*===========================================
     *          internal definition
     *=========================================*/
    
    /* debug level,
     * if is DEBUG_LEVEL_DISABLE, on log is allowed output,
     * if is DEBUG_LEVEL_ERR, only AMBE_ERR is allowed output,
     * if is DEBUG_LEVEL_INFO, AMBE_ERR and AMBE_INFO are allowed output,
     * if is DEBUG_LEVEL_DEBUG, all log are allowed output,
     */
    enum debug_level {
        DEBUG_LEVEL_DISABLE = 0,
        DEBUG_LEVEL_ERR,
        DEBUG_LEVEL_INFO,
        DEBUG_LEVEL_DEBUG,
        DEBUG_LEVEL_TIME
    };

    #define ASSERT()\
    do {\
        printf(" %d  %s  %s",\
                __LINE__, __FUNCTION__, __FILE__);\
        while (1);\
    } while (0)

    #define ERR(...)\
    do {\
        if (debug >= DEBUG_LEVEL_ERR) {\
            printf(__VA_ARGS__);\
        }\
    } while (0)

    #define INFO(...)\
    do {\
        if (debug >= DEBUG_LEVEL_INFO) {\
            printf(__VA_ARGS__);\
        }\
    } while (0)

    #define DEBUG(...)\
    do {\
        if (debug >= DEBUG_LEVEL_DEBUG) {\
            printf(__VA_ARGS__);\
        }\
    } while (0)

    #define TIME()\
    do {\
        if (debug >= DEBUG_LEVEL_TIME) {\
            printf("%d\n", clock());\
        }\
    } while (0)
#else
    #define ASSERT()   do { } while (0)
    #define ERR(...)   do { } while (0)
    #define INFO(...)  do { } while (0)
    #define DEBUG(...) do { } while (0)
    #define TIME(...) do { } while (0)
#endif

#endif

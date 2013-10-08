/*
 * the private definition of "Beat per minute" detection
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

#ifndef _BPM_PRI_H_
#define _BPM_PRI_H_

#define REAL_IS_FLOAT

#ifdef REAL_IS_FLOAT
	typedef float               REAL;

	#define SIN(x)              sinf(x)
	#define COS(x)              cosf(x)

	#define CDFT_RECURSIVE_N    1024
#endif

#ifdef REAL_IS_DOUBLE
	typedef double              REAL;

	#define SIN(x)              sin(x)
	#define COS(x)              cos(x)

	#define CDFT_RECURSIVE_N    512
#endif

#ifdef REAL_IS_LONGDOUBLE
	typedef long double         REAL;

	#define SIN(x)              sinl(x)
	#define COS(x)              cosl(x)

	#define CDFT_RECURSIVE_N    512
#endif

#define PI                      3.1415926535897932384626433832795029L
#define PI2                     6.283185307179586476925286766559L

#endif

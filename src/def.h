/*
 * def.h
 *
 *  Created on: 2015年3月23日
 *      Author: wsf
 *
 */

#ifndef SRC_DEF_H_
#define SRC_DEF_H_
/*
 * system define
 */




/* Constants defined by configure.ac */
#define HAVE_INTTYPES_H 1
#define HAVE_STDINT_H 1

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
# ifndef _MSC_VER
# include <stdint.h>
# else
# include "stdint.h"
# endif
#endif

#define SERVER_ID         128
#define INVALID_SERVER_ID 18

//extern const float UT_REAL = 916.540649;
//extern const uint32_t UT_IREAL = 0x4465229a;


#endif /* SRC_DEF_H_ */

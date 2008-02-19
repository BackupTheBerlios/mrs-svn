/* 
 * If linux/types.h is already been included, assume it has defined
 * everything we need.  (cross fingers)  Other header files may have 
 * also defined the types that we need.
 */
#if (!defined(_STDINT_H) && !defined(_UUID_STDINT_H))
#define _UUID_STDINT_H

// change by MLH... no need to do extra work here.
#include "../HLib.h"

//typedef int8	int8_t;
typedef uint8	uint8_t;
//typedef int16	int16_t;
typedef uint16	uint16_t;
//typedef int32	int32_t;
typedef uint32	uint32_t;
//typedef int64	int64_t;
typedef uint64	uint64_t;

#endif

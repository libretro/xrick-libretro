#ifndef SDL_ENDIAN_H
#define SDL_ENDIAN_H

#include <retro_endianness.h>

//RETRO HACK
#include "SDL.h"

static INLINE     unsigned short SDL_Swap16(unsigned short x){
	unsigned short result= ((x<<8)|(x>>8)); 
return result;
}
static INLINE     unsigned SDL_Swap32(unsigned x){
	unsigned result= ((x<<24)|((x<<8)&0x00FF0000)|((x>>8)&0x0000FF00)|(x>>24));
 return result;
}

#define SDL_SwapLE16(X)	retro_le_to_cpu16(X)
#define SDL_SwapLE32(X) retro_le_to_cpu32(X)

#define SDL_SwapBE16(X) retro_be_to_cpu16(X)
#define SDL_SwapBE32(X) retro_be_to_cpu32(X)

#define SDL_LIL_ENDIAN	1234
#define SDL_BIG_ENDIAN	4321

#if RETRO_IS_BIG_ENDIAN
#define SDL_BYTEORDER SDL_BIG_ENDIAN
#else    
#define SDL_BYTEORDER SDL_LIL_ENDIAN         
#endif


#endif /* UAE_MACCESS_H */

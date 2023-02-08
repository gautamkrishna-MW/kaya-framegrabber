

#include "KYFGLib.h"
#include <opencv2/opencv.hpp>

#include <iostream>
#include <ctype.h>
#include <assert.h>
#include <signal.h>

/* Comment this to stop seeing debug messages */
#define DEBUG_BUILD

/* KAYA Specific defines */
#define MAXSTREAMS 2
#define MAXBUFFERS 16

/* Debug Messages */
#ifdef DEBUG_BUILD
#  define DEBUG(x) std::cerr << x << std::endl
#else
#  define DEBUG(x) do {} while (0)
#endif

/* Count the objects in an array */
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))

/* Assertion Check */
#define assertChk(msg,exprsn) assert(msg && exprsn)

/* Aligned Memory allocation for faster reads/writes */
void* _aligned_malloc(size_t size, size_t alignment)
{
    size_t pageAlign = size % 4096;
    if(pageAlign)
    {
        size += 4096 - pageAlign;
    }

#if(GCC_VERSION <= 40407)
    void * memptr = 0;
    posix_memalign(&memptr, alignment, size);
    return memptr;
#else
    return aligned_alloc(alignment, size);
#endif
}

/* Big-endian to Little-endian conversion */
uint32_t swap_uint32( uint32_t val )
{
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF ); 
    return (val << 16) | (val >> 16);
}

/* KAYA Error check and assert */
#define kayaErrchk(ans) { kayaAssert((ans), __FILE__, __LINE__); }
inline void kayaAssert(FGSTATUS code, const char *file, int line, bool abort=true)
{
   if (code != FGSTATUS_OK) 
   {
      fprintf(stderr,"KAYA Error: ID: 0x%x, FILE: %s, LINE: %d\n", code, file, line);
      fprintf(stderr,"Refer \"KYFG_ErrorCodes.h\" for more information on the error\n");
      if (abort) 
        exit(code);
   }
}

/* Ctrl+C handling */
volatile sig_atomic_t stop;
void inthand(int signum) {
    stop = 1;
}
//*****************************************************************************
// File ..................: heapdump.h
// Description ...........: Heap analysis
// Author ................: Peter Thoemmes
//-----------------------------------------------------------------------------
// It is important to determine the top of the STACK and the bottom of the HEAP
// during runtime, directly at the beginning of the code in main()...
//
//      int main(int argc,char* argv[])
//      {
//          init_heapdump();
//          ...
//
//-----------------------------------------------------------------------------
// Copyright (c) 2000-2018 Peter Thoemmes, Weinbergstrasse 3a, D-54441 Ockfen
//*****************************************************************************

//*****************************************************************************
// Include control (begin):
//*****************************************************************************

#ifndef HEAPDUMP_H_
#define HEAPDUMP_H_

//*****************************************************************************
// Header files:
//*****************************************************************************

#include "settings.h"

//*****************************************************************************
// Interface:
//*****************************************************************************

//-----------------------------------------------------------------------------
// Initialize this module:
//-----------------------------------------------------------------------------

extern "C" void init_heapdump(unsigned char verbose = 0);

//-----------------------------------------------------------------------------
// Get the fixed memory dimensions of STACK and HEAP:
//-----------------------------------------------------------------------------

extern "C" bool get_memory_dimenions(
                            size_t** max_addr, //max. virtual address
                            size_t** max_user_addr, //max user address
                            size_t** stack_top, //top of the stack
                            size_t** heap_bottom_chunk); //heap bottom chunk

//-----------------------------------------------------------------------------
// Get the heap bottom chunk:
//-----------------------------------------------------------------------------
// Returns pointer to heap bottom chunk or NULL if module was not initialized
//-----------------------------------------------------------------------------

extern "C" size_t* get_heap_bottom_chunk();

//-----------------------------------------------------------------------------
// Get the current contiguous HEAP (main arena) limit:
//-----------------------------------------------------------------------------
// Returns the full size of the contiguous heap
//-----------------------------------------------------------------------------

extern "C" size_t get_current_contiguous_heap_limit(
                                size_t** heap_top_end, //first invalid address
                                size_t** heap_top_chunk); //heap top chunk

//-----------------------------------------------------------------------------
// Get the chunk size of the allocated memory around a pointer:
//-----------------------------------------------------------------------------

extern "C" size_t get_allocated_chunk_size(void* mem_ptr);

//-----------------------------------------------------------------------------
// Get the payload size allocated behind a memory pointer:
//-----------------------------------------------------------------------------

extern "C" size_t get_allocated_payload_size(void* mem_ptr);

//-----------------------------------------------------------------------------
// Dump the total heap footprint:
//-----------------------------------------------------------------------------

extern "C" void dump_heap_footprint();

//-----------------------------------------------------------------------------
// Dump heap details for debugging:
//-----------------------------------------------------------------------------

extern "C" void dump_heap_details(
                        size_t* start_chunk, //e.g. get_chunk(void* mem_ptr)
                        size_t* heap_top_end); //first invalid address

//-----------------------------------------------------------------------------
// HEX dump of the heap:
//-----------------------------------------------------------------------------

extern "C" void dump_heap_hex(
                        size_t* start_chunk, //e.g. get_chunk(void* mem_ptr)
                        size_t* heap_top_end, //first invalid address
                        uint32 max_kb = 0);

//-----------------------------------------------------------------------------
// Raw heap dump:
//-----------------------------------------------------------------------------

extern "C" void dump_heap_raw(
                        size_t* start_chunk, //e.g. get_chunk(void* mem_ptr)
                        size_t* heap_top_end, //first invalid address
                        uint32 max_kb = 0);

//*****************************************************************************
// Internal interface:
//*****************************************************************************

#if !defined(_WIN32) && !defined(_WIN64)

    extern "C" size_t* get_arena(size_t* chunk_ptr); //or NULL
    extern "C" size_t* get_next_arena(size_t* ar_ptr); //or NULL
    extern "C" bool is_top_chunk(size_t* chunk_ptr);
    extern "C" size_t* get_heap_top_end(size_t* chunk_ptr); //or NULL
    extern "C" size_t* get_bottom_chunk(size_t* chunk_ptr); //or NULL
    extern "C" size_t get_all_bottom_chunks_of_arena(
                                 size_t* ar_top_chunk_ptr, //arena's top chunk
                                 size_t** baddr_arr_ptr, //ptr to size_t array
                                 size_t max_num_botts); //size of array

#endif

//*****************************************************************************
// Constants and definitions:
//*****************************************************************************

#if defined(__CYGWIN32__) || \
  defined(__CYGWIN__) || \
  defined(_WIN32) || \
  defined(i386) || \
  defined(__i386) || \
  defined(__i386__)
    #define SIZE_T_IS_4_BYTES
#endif

static const size_t KB__ = 1024;
static const size_t MB__ = KB__ * 1024;
static const size_t GB__ = MB__ * 1024;
#ifndef SIZE_T_IS_4_BYTES
    static const size_t TB__ = GB__ * 1024;
#endif

//Get a human readable memory size representation of a value:
#ifndef SIZE_T_IS_4_BYTES
    #define HUMAN_READABLE_MEM_SIZE__(byte_size) ( \
                byte_size > 99 * TB__ ? byte_size/TB__ : \
                    byte_size > 99 * GB__ ? byte_size/GB__ : \
                        byte_size > 99 * MB__ ? byte_size/MB__ : \
                            byte_size > 99 * KB__ ? byte_size/KB__ : \
                                byte_size)
#else
    #define HUMAN_READABLE_MEM_SIZE__(byte_size) ( \
                byte_size > 99 * MB__ ? byte_size/MB__ : \
                    byte_size > 99 * KB__ ? byte_size/KB__ : \
                        byte_size)
#endif

//Get a human readable memory size unit representation of a value:
#ifndef SIZE_T_IS_4_BYTES
    #define HUMAN_READABLE_MEM_UNIT__(byte_size) ( \
                    byte_size > 99 * TB__ ? "TB" : \
                        byte_size > 99 * GB__ ? "GB" : \
                            byte_size > 99 * MB__ ? "MB" : \
                                byte_size > 99 * KB__ ? "KB" : \
                                    "BYTES")
    #define HUMAN_READABLE_MEM_UNIT_2__(byte_size) ( \
                    byte_size > 99 * TB__ ? "TB" : \
                        byte_size > 99 * GB__ ? "GB" : \
                            byte_size > 99 * MB__ ? "MB" : \
                                byte_size > 99 * KB__ ? "KB" : \
                                    "BY")
#else
    #define HUMAN_READABLE_MEM_UNIT__(byte_size) ( \
                    byte_size > 99 * MB__ ? "MB" : \
                        byte_size > 99 * KB__ ? "KB" : \
                            "BYTES")
    #define HUMAN_READABLE_MEM_UNIT_2__(byte_size) ( \
                    byte_size > 99 * MB__ ? "MB" : \
                        byte_size > 99 * KB__ ? "KB" : \
                            "BY")
#endif

//Get a human readable representation of a char value:
inline char human_readable__(char c)
{
    return ((c >= 0x20) && (c < 0x80)) ? c : '.';
}

//-----------------------------------------------------------------------------
// Boundaries of the virtual memory of a process:
//-----------------------------------------------------------------------------
//
// Virtual memory segments are build like this:
//
//  On 64 bit OS, the user address space
//  ends at 128 TB:
//
//  +-------------------+ 0xffffffffffff 256 TB address limit on 64 bit OS
//  |   MAPPED KERNEL   |
//  |                   |
//  |                   |
//  |                   |
//  |                   |
//  /                   /
//  /                   /
//  |                   |
//  |                   |
//  |                   |
//  |                   |
//  |                   |
//  +-------------------+ 0x7fffffffffff 128 TB limit (131072 GB) on 64 bit OS
//  |    Guard Page     |
//  +-------------------+ 0x7fffffffefff Top of STACK on 64 bit OS
//  |      STACK        |
//  |                   |
//  |                   |
//  /                   /
//  /                   /
//  |                   |
//  |                   |
//  |                   |
//  |                   |
//  |                   |
//  |                   |
//
//
//  As an option on a 32 bit OS, the user
//  address space can be raised to 3 GB:
//
//  +-------------------+ 0x0000ffffffff 4 GB address limit on 32 bit OS
//  |   MAPPED KERNEL   |
//  /                   /
//  /                   /
//  |                   |
//  +-------------------+ 0x0000bfffffff 3 GB limit (increased) on 32 bit OS
//  |    Guard Page     |
//  +-------------------+ 0x0000bfffefff Top of STACK (increased 32 bit OS)
//  |      STACK        |
//  /                   /
//  /                   /
//
//
//  Typically on a 32 bit OS, the user
//  address space ends at 2 GB:
//
//  +-------------------+ 0x0000ffffffff 4 GB address limit on 32 bit OS
//  |   MAPPED KERNEL   |
//  /                   /
//  /                   /
//  |                   |
//  +-------------------+ 0x00007fffefff 2 GB limit on 32 bit OS
//  |    Guard Page     |
//  +-------------------+ 0x00007fffefff Top of STACK (32 bit OS)
//  |      STACK        |
//  |                   |
//  /                   /
//  /                   /
//
//
//  Below the STACK, which grows down, is
//  the HEAP located, which grows up:
//
//  |                   |
//  |   ^  ^  ^  ^  ^   |
//  +---|--|--|--|--|---+ Top of HEAP
//  |                   |
//  |                   |
//  |                   |
//  |       HEAP        |
//  |                   |
//  |                   |
//  |                   |
//  |                   |
//  +-------------------+ Bottom of HEAP
//  |      STATIC       |
//  |   Uninitialized   |
//  |      (BSS)        |
//  +-------------------+ Bottom of uninitialized STATIC data (BSS)
//  |      STATIC       |
//  |    Initialized    |
//  |      (DATA)       |
//  +-------------------+ Bottom of initialized STATIC data (constants)
//  |                   |
//  |    CODE (TEXT)    |
//  |                   |
//  |                   |
//  |                   |
//  +-------------------+ Bottom of the CODE
//  |                   |     Linux ELF 32 bit: 0x000008048000 (> 128 MB)
//  |                   |         Linux 64 bit: 0x000000400000 (= 4 MB)
//  |                   |       Windows 32 bit: 0x000000010000 (= 64 KB)
//  |                   |
//  +-------------------+ 0x000000000000 Bottom of virtual address space
//
//-----------------------------------------------------------------------------

#ifdef PAGESIZE
    #define PAGE ((size_t) PAGESIZE)
#else
    #define PAGE 4096UL
#endif

#define MAX_ADDR_256TB 0xffffffffffffUL
#define MAX_ADDR_128TB 0x7fffffffffffUL
#define MAX_ADDR_4GB       0xffffffffUL
#define MAX_ADDR_3GB       0xbfffffffUL
#define MAX_ADDR_2GB       0x7fffffffUL

#define STACK_TOP64       ((size_t*) (MAX_ADDR_128TB - PAGE)) //0x7fffffffefff
#define STACK_TOP32_3GB   ((size_t*) (MAX_ADDR_3GB - PAGE))   //0x0000bfffefff
#define STACK_TOP32_2GB   ((size_t*) (MAX_ADDR_2GB - PAGE))   //0x00007fffefff

//*****************************************************************************
// Definitions for platform specific heap management:
//*****************************************************************************

#if defined(_WIN32) || defined(_WIN64)

    //-------------------------------------------------------------------------
    // Structure of the chunks:
    //-------------------------------------------------------------------------
    //
    // *** RELEASE:
    //
    // chunk_ptr = mem_ptr-> +---------------------------+
    //                       |         Payload           |
    //                       |                           |
    //                       |                           |
    //                       |                           |
    //                       |                           |
    //                       |                           |
    //                       |                           |
    //                       |                           |
    //                       |                           |
    //                       |                           |
    //                       +---------------------------+
    //                       |        Meta data          | 4 bytes
    //      next_chunk_ptr-> +---------------------------+
    //                       |           ...             |
    //
    // *** DEBUG:
    //
    //           chunk_ptr-> +---------------------------+
    //                       |       DEBUG HEADER        |
    //                       |                           |
    //                       |                           |
    //                       |                           |
    //                       |                           |
    //                       |                           |
    //              mem_ptr->+---------------------------+
    //                       |         Payload           |
    //                       |                           |
    //                       |                           |
    //                       |                           |
    //                       |                           |
    //                       |                           |
    //                       +---------------------------+
    //                       |        Meta data          | 4 bytes
    //      next_chunk_ptr-> +---------------------------+
    //                       |           ...             |
    //
    //-------------------------------------------------------------------------

    #define nNoMansLandSize 4

    struct WinDgbHeader__
    {
        struct WinDgbHeader__* next;
        struct WinDgbHeader__* prev;
        char* filename;
        int line;
        #ifdef _WIN64 //for 16 byte alignment
            int block_use;
            size_t data_size;
        #else
            size_t data_size;
            int block_use;
        #endif
        long request;
        unsigned char gap[nNoMansLandSize];
    };

    static const size_t DBG_HDR_SIZE = sizeof(WinDgbHeader__);

    //Dump a chunk:
    inline void dump_chunk(size_t* p,size_t size)
    {
        if(!p)
            return;

        #ifdef _DEBUG
            WinDgbHeader__* chunk_ptr = (WinDgbHeader__*) p;

            unsigned char* mem_ptr = (unsigned char*)
                                                 (((char*) p) + DBG_HDR_SIZE);

            bool is_bottom_chunk =
                        (!chunk_ptr->next && !chunk_ptr->prev) ? true : false;
            bool is_top_chunk =
                ((((uint32)(chunk_ptr->next)) == 0xFEEEFEEE) &&
                   (((uint32)(chunk_ptr->prev)) == 0xFEEEFEEE)) ? true : false;


            printf(
                "  0x%08X +--------------------------- %u bytes%s%s\n"
                "  DEBUG HDR  | next = 0x%X\n"
                "    %2u bytes | prev = 0x%X\n"
                "             | ...\n"
                "  0x%08X +---------------------------\n"
                "  MEM        | %02X %02X %02X %02X | %c%c%c%c\n"
                "             | %02X %02X %02X %02X | %c%c%c%c\n"
                "             | ...\n"
                "%s",
                chunk_ptr,
                size,
                is_bottom_chunk ? " BOTTOM CHUNK" : "",
                is_top_chunk ? " TOP CHUNK" : "",
                chunk_ptr->next,
                DBG_HDR_SIZE,
                chunk_ptr->prev,
                mem_ptr,
                mem_ptr[0],
                mem_ptr[1],
                mem_ptr[2],
                mem_ptr[3],
                human_readable__(mem_ptr[0]),
                human_readable__(mem_ptr[1]),
                human_readable__(mem_ptr[2]),
                human_readable__(mem_ptr[3]),
                mem_ptr[4],
                mem_ptr[5],
                mem_ptr[6],
                mem_ptr[7],
                human_readable__(mem_ptr[4]),
                human_readable__(mem_ptr[5]),
                human_readable__(mem_ptr[6]),
                human_readable__(mem_ptr[7]),
                is_top_chunk ?
                        "             +---------------------------\n" : "");
        #else
            size_t* chunk_ptr = p;

            unsigned char* mem_ptr = (unsigned char*) chunk_ptr;

            bool is_bottom_chunk =
                   !(*((size_t*) &mem_ptr[0])) &&
                                    !(*((size_t*) &mem_ptr[4])) ? true : false;
            bool is_top_chunk =
                (*((size_t*) &mem_ptr[0]) == 0xFEEEFEEE) &&
                       (*((size_t*) &mem_ptr[0]) == 0xFEEEFEEE) ? true : false;

            printf(
                "  0x%08X +------------------- %u bytes%s%s\n"
                "  MEM        | %02X %02X %02X %02X | %c%c%c%c\n"
                "             | %02X %02X %02X %02X | %c%c%c%c\n"
                "             | ...\n"
                "%s",
                chunk_ptr,
                size,
                is_bottom_chunk ? " BOTTOM CHUNK" : "",
                is_top_chunk ? " TOP CHUNK" : "",
                mem_ptr[0],
                mem_ptr[1],
                mem_ptr[2],
                mem_ptr[3],
                human_readable__(mem_ptr[0]),
                human_readable__(mem_ptr[1]),
                human_readable__(mem_ptr[2]),
                human_readable__(mem_ptr[3]),
                mem_ptr[4],
                mem_ptr[5],
                mem_ptr[6],
                mem_ptr[7],
                human_readable__(mem_ptr[4]),
                human_readable__(mem_ptr[5]),
                human_readable__(mem_ptr[6]),
                human_readable__(mem_ptr[7]),
                is_top_chunk ?
                        "             +-------------------\n" : "");
        #endif
    }

    #ifdef _DEBUG

        //Get the memory pointer from the chunk pointer:
        inline void* get_mem_ptr(size_t* chunk_ptr)
        {
            if(!chunk_ptr)
                return (void*) 0;
            return (void*) (((char*) (chunk_ptr)) + DBG_HDR_SIZE);
        }

        //Get the chunk pointer from the memory pointer:
        inline size_t* get_chunk(void* mem_ptr)
        {
            if(!mem_ptr)
                return (size_t*) 0;
            return (size_t*) (((char*) (mem_ptr)) - DBG_HDR_SIZE);
        }

    #else

        //Get the memory pointer from the chunk pointer:
        inline void* get_mem_ptr(size_t* chunk_ptr)
        {
            if(!chunk_ptr)
                return (void*) 0;
            return (void*) chunk_ptr;
        }

        //Get the chunk pointer from the memory pointer:
        inline size_t* get_chunk(void* mem_ptr)
        {
            if(!mem_ptr)
                return (size_t*) 0;
            return (size_t*) mem_ptr;
        }

    #endif

#else

    //-------------------------------------------------------------------------
    // CHUNKS (from glibc's malloc.c):
    //-------------------------------------------------------------------------
    //
    // *** CHUNK of memory:
    //
    //              p-> +---------------------------+
    //                  | ...                       |
    //                  +---------------------------+
    //                  | size                |A|M|P| chunk_size = size & ~0x7
    //        mem_ptr-> +---------------------------+   Flags: P = 0x1
    //                  | ...                       |          M = 0x2 (mmap)
    //                  |                           |          A = 0x4
    //                  |                           |
    //                  |                           |
    //                  |                           |
    //                  |                           |
    //                  |                           |
    //           next-> +---------------------------+
    //                  | ...                       |
    //
    //      +-----------------------------------------------------------------+
    //      | mem_ptr = (size_t*) ((char*) p + 2 * sizeof(size_t))            |
    //      +-----------------------------------------------------------------+
    //      | chunk_size = (p->size & ~0x7)                                   |
    //      +-----------------------------------------------------------------+
    //      | prev_in_use = (p->size & 0x1) ? true : false                    |
    //      +-----------------------------------------------------------------+
    //      | is_mmapped = (p->size & 0x2) ? true : false                     |
    //      +-----------------------------------------------------------------+
    //      | in_thread_arena = (p->size & 0x4) ? true : false                |
    //      +-----------------------------------------------------------------+
    //      | if(is_mmapped)                                                  |
    //      | {                                                               |
    //      |     is_top = true                                               |
    //      | }                                                               |
    //      | else                                                            |
    //      | {                                                               |
    //      |     next = (size_t*) ((char*) p + chunk_size)                   |
    //      |     if(p < sbrk(0))                                             |
    //      |     {                                                           |
    //      |         top = sbrk(0)                                           |
    //      |     }                                                           |
    //      |     else                                                        |
    //      |     {                                                           |
    //                heapinf = (size_t*) ((size_t) p & ~(HEAP_MAX_SIZE - 1)) |
    //      |         top = (size_t*) (((char*) heapinf) + heapinf->size)     |
    //      |     }                                                           |
    //      |     is_top = (next == top) ? true : false                       |
    //      | }                                                               |
    //      +-----------------------------------------------------------------+
    //      | is_in_use = false                                               |
    //      | if(!is_top && !is_mmapped)                                      |
    //      | {                                                               |
    //      |     next = (size_t*) ((char*) p + chunk_size)                   |
    //      |     is_in_use = (next->size & 0x1) ? true : false               |
    //      | }                                                               |
    //      | else if(is_mmapped)                                             |
    //      | {                                                               |
    //      |     is_in_use = true                                            |
    //      | }                                                               |
    //      +-----------------------------------------------------------------+
    //
    //      The size of a chunk comes in bytes and is or'd with flags at the
    //      least significant bits:
    //
    //              P)revious in use (1 = previous chunk is in use)
    //              M)emory mapped (1 = chunk is mmap()-allocated)
    //              A)rena of thread (1 = heap arena allocated by a thread)
    //
    //      The pointer mem_ptr is the one that a malloc() returns after
    //      a successfully allocation of a chunk of memory.
    //      Starting from a heap bottom (bottom_chunk), every next chunk
    //      can be reached by adding the chunk size to the chunk pointer p.
    //      Only the next chunk tells by its P flag if a chunk is in use
    //      (P = 1) or free (P = 0).
    //
    //      Be aware that very big chunks are allocated with mmap() and
    //      those are not part of a contiguous, but standalone. The M flag
    //      (0x2) is set in the size field of those chunks.
    //
    //
    // *** ALLOCATED chunk (next->size & 0x1 == 0x1):
    //
    //              p-> +---------------------------+
    //                  | ...                       |
    //                  +---------------------------+
    //                  | size                |A|M|P|
    //        mem_ptr-> +---------------------------+-+
    //                  |                           | |
    //                  |                           | |
    //                  |                           | |
    //                  |        USED DATA          | |
    //                  |                           | |<--- DATA ----+
    //                  |                           | |              |
    //                  |                           | /              |
    //           next-> +---------------------------+                |
    //                  | ...                       |                |
    //                  +---------------------------+                |
    //                  | size                |A|M|1|--> P = 1 ------+
    //                  +---------------------------+
    //                  | ...                       |
    //
    //      +------------------------------------------------------+
    //      | payload_size = (p->size & ~0x7) - 2 * sizeof(size_t) |
    //      +------------------------------------------------------+
    //
    //      Allocated chunks are determined by the P flag (must be 1) of the
    //      following chunk. If the self-contained P flag is 0, meaning the
    //      previous chunk is a free one, then the first field tells the size
    //      of that previous chunk. Otherwise (when the previous chunk is in
    //      use) that field is undefined.
    //
    //
    // *** FREE chunk (next->size & 0x1 == 0x0):
    //
    //              p-> +---------------------------+
    //                  | ...                       |
    //                  +---------------------------+
    //                  | size                |A|M|P|
    //        mem_ptr-> +---------------------------+
    //                  | next_free                 |<---------------+
    //                  +---------------------------+                |
    //                  | prev_free                 |<---------------+
    //                  +---------------------------+                |
    //                  |                           |                |
    //                  |                           |                |
    //                  |                           |                |
    //                  |        FREE SPACE         |                |
    //                  |                           |                |
    //                  |                           |                |
    //                  |                           |                |
    //           next-> +---------------------------+                |
    //                  | ...                       |                |
    //                  +---------------------------+                |
    //                  | size                |A|M|0|--> P = 0 ------+
    //                  +---------------------------+
    //                  |                           |
    //                  |           DATA            |
    //                  | ...                       |
    //
    //     No two free chunks can be adjacent together, as this would become
    //     one single free chunk! So the previous and the following chunk are
    //     always in use, if existing! The potential payload size of a free
    //     chunk is calculated the same way as for the chunk in use. This
    //     is, because the meaning of the fields will go away as soon as the
    //     chunk becomes allocated by malloc().
    //
    //     The pointers next_free and prev_free do point to the other free
    //     chunks in a double-chained list of free chunks. This is important
    //     for heap management, as allocating a chunk out of the free ones,
    //     will lead to reassignment of that pointers and setting the P flag
    //     in the following chunk, which is always in use.
    //
    //
    // *** TOP chunk (next == top; always existing and always free):
    //
    //              p-> +---------------------------+
    //                  | ...                       |
    //                  +---------------------------+
    //                  | size                |A|M|P|
    //        mem_ptr-> +---------------------------+
    //                  | next_free                 |
    //                  +---------------------------+
    //                  | prev_free                 |
    //                  +---------------------------+
    //                  |                           |
    //                  |                           |
    //                  |                           |
    //                  |        FREE SPACE         |
    //                  |                           |
    //                  |                           |
    //                  |                           |
    //     next = top-> +---------------------------+
    //
    //     To determine, if a chunk is the top chunk, one needs to test if
    //     next is equal to the top of the heap (first invalid address) or
    //     not. The top chunk is always a free one and the only one in case
    //     no memory is allocated. (Remember that chunks, which have been
    //     allocated with mmap() are always standalone and always allocated.)
    //
    //     As no two free chunks can be adjacent together, the previous chunk
    //     is always in use, if existing (P = 1).
    //
    //
    // *** Continuous HEAP layout (not for mmap()-allocated chunks):
    //
    //   BOTTOM ------> +--------------+
    //                  | ...          |
    //                  +--------------+
    //           +<-----| size   |A|M|P| P = 1
    //           |      +--------------+
    //           |      |              |
    //           |      |     USED     |            USED
    //           '      |              |
    //            `---> +--------------+
    //                  | ...          |
    //                  +--------------+
    //           +<-----| size   |A|M|1| P = 1
    //           |      +--------------+
    //           |      | next_free    |
    //           |      +--------------+
    //           |      | prev_free    |
    //           |      +--------------+
    //           |      |              |
    //           |      |              |
    //           |      |     FREE     |            FREE
    //           |      |              |
    //           '      |              |
    //            `---> +--------------+
    //                  | ...          |
    //                  +--------------+
    //           +<-----| size   |A|M|0| P = 0
    //           |      +--------------+
    //           |      |              |
    //           |      |     USED     |            USED
    //           '      |              |
    //            `---> +--------------+
    //                  | ...          |
    //                  +--------------+
    //           +<-----| size   |A|M|1| P = 1
    //           |      +--------------+
    //           |      | next_free    |
    //           |      +--------------+
    //           |      | prev_free    |
    //           |      +--------------+
    //           |      |              |
    //           |      |              |
    //           |      |     FREE     |            FREE
    //           |      |              |
    //           '      |              |
    //            `---> +--------------+
    //                  | ...          |
    //                  +--------------+
    //           +<-----| size   |A|M|0| P = 0
    //           |      +--------------+
    //           |      |              |
    //           |      |     USED     |            USED
    //           '      |              |
    //   TOP -----`---> +--------------+ <-------- The TOP chunk of a single
    //                  | ...          |           HEAP is not necessarily
    //                  +--------------+           equal to the TOP chunk of
    //           +<-----| size   |A|M|1| P = 1     its ARENA!
    //           |      +--------------+
    //           |      | next_free    |
    //           |      +--------------+
    //           |      | prev_free    |
    //           |      +--------------+
    //           |      |              |
    //           |      |              |
    //           |      |     FREE     |            FREE
    //           |      |              |
    //           '      |              |
    //       top --`--> +--------------+
    //
    //-------------------------------------------------------------------------

    #define P__        (size_t) 0x1 //set = P)revious in use flag
    #define M__        (size_t) 0x2 //set = M)emory mapped flag
    #define A__        (size_t) 0x4 //set = A)rena flag (= not main arena)
    #define FLAGS_MASK (size_t) 0x7

    struct chunk_t
    {
        size_t overhead; //impl. dependent: previous size, previous data, ... 
        size_t size; //size of chunk in bytes (or'd with flags on 3 LSBs)
        struct chunk_t* next_free; //next free chunk in list of free chunks
        struct chunk_t* prev_free; //previous free chunk in list of free chunks
    };

    struct big_chunk_t
    {
        size_t overhead;
        size_t size;
        struct chunk_t* next_free;
        struct chunk_t* prev_free;
        struct chunk_t* next_free_size;
        struct chunk_t* prev_free_size;
    };

    #define MIN_CHUNK_SIZE (sizeof(chunk_t))

    #define SIZE_SZ (sizeof(size_t))

    #define MALLOC_ALIGN_MASK (2 * SIZE_SZ - 1)

    #ifndef DEFAULT_MMAP_THRESHOLD_MAX
        #ifndef SIZE_T_IS_4_BYTES
            #define DEFAULT_MMAP_THRESHOLD_MAX (4 * 1024 * 1024 * sizeof(long))
        #else
            #define DEFAULT_MMAP_THRESHOLD_MAX (512 * 1024)
        #endif
    #endif

    #ifndef HEAP_MAX_SIZE
        #ifdef DEFAULT_MMAP_THRESHOLD_MAX
            #define HEAP_MAX_SIZE (2 * DEFAULT_MMAP_THRESHOLD_MAX)
        #else
            #define HEAP_MAX_SIZE (1024 * 1024)
        #endif
    #endif

    #define MINSIZE (unsigned long) \
                (((MIN_CHUNK_SIZE+MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK))

    #define request2size(req) \
        (((req) + SIZE_SZ + MALLOC_ALIGN_MASK < MINSIZE) ? MINSIZE : \
                ((req) + SIZE_SZ + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)

    #define NBINS      128
    #define NSMALLBINS  64

    #define BINMAPSHIFT 5
    #define BITSPERMAP (1U << BINMAPSHIFT)
    #define BINMAPSIZE (NBINS / BITSPERMAP)

    #define SMALLBIN_WIDTH MALLOC_ALIGNMENT
    #define SMALLBIN_CORRECTION (MALLOC_ALIGNMENT > 2 * SIZE_SZ)
    #define MIN_LARGE_SIZE ((NSMALLBINS - SMALLBIN_CORRECTION)*SMALLBIN_WIDTH)

    #define fastbin(ar_ptr,idx) ((ar_ptr)->fastbinsY[idx])
    #define fastbin_index(sz) \
        ((((unsigned int)(sz)) >> (SIZE_SZ == 8 ? 4 : 3)) - 2)
    #define MAX_FAST_SIZE (80 * SIZE_SZ / 4)
    #define NFASTBINS (fastbin_index(request2size(MAX_FAST_SIZE))+1)

    #include <pthread.h>
    #define mutex_t pthread_mutex_t

    //Dump a chunk:
    inline void dump_chunk(size_t* p)
    {
        if(!p)
            return;

        size_t chunk_size = (size_t) (((chunk_t*) p)->size & ~FLAGS_MASK);
        bool prev_in_use = (((chunk_t*) p)->size & P__) ? true : false;
        bool is_mmapped = (((chunk_t*) p)->size & M__) ? true : false;
        bool thread_arena = (((chunk_t*) p)->size & A__) ? true : false;

        bool is_top = is_top_chunk(p);

        bool is_in_use = is_mmapped ? true : false;
        if(!is_top && !is_mmapped)
        {
            size_t* next = (size_t*) (((char*) p) + chunk_size);
            is_in_use = (((chunk_t*) next)->size & P__) ? true : false;
        }

        size_t payload_size = chunk_size - 2 * sizeof(size_t);

        if(is_in_use)
        {
            unsigned char* mem_ptr =
                        (unsigned char*) (((char*) p) + 2 * sizeof(size_t));

            printf(
                "%14p +-----------------------------------\n"
                "   %s | %02X %02X %02X %02X %02X %02X %02X %02X\n"
                "               +-----------------------------------\n"
                "               | chunk_size = %lu %s | AMP = %X%X%X\n"
                "               +-----------------------------------\n",
                p,
                is_top ? "(TOP CHUNK)" : "           ",
                (unsigned char) ((((chunk_t*) p)->overhead) >> 56),
                (unsigned char) ((((chunk_t*) p)->overhead) >> 48),
                (unsigned char) ((((chunk_t*) p)->overhead) >> 40),
                (unsigned char) ((((chunk_t*) p)->overhead) >> 32),
                (unsigned char) ((((chunk_t*) p)->overhead) >> 24),
                (unsigned char) ((((chunk_t*) p)->overhead) >> 16),
                (unsigned char) ((((chunk_t*) p)->overhead) >> 8),
                (unsigned char) (((chunk_t*) p)->overhead),
                HUMAN_READABLE_MEM_SIZE__(chunk_size),
                HUMAN_READABLE_MEM_UNIT__(chunk_size),
                thread_arena ? 0x1 : 0x0,
                is_mmapped ? 0x1 : 0x0,
                prev_in_use ? 0x1 : 0x0);

            if(payload_size >= 16)
            {
                printf(
                    "               | "
                        "%02X %02X %02X %02X %02X %02X %02X %02X | "
                            "%c%c%c%c%c%c%c%c\n"
                    "               | "
                        "%02X %02X %02X %02X %02X %02X %02X %02X | "
                            "%c%c%c%c%c%c%c%c\n",
                    mem_ptr[0],
                    mem_ptr[1],
                    mem_ptr[2],
                    mem_ptr[3],
                    mem_ptr[4],
                    mem_ptr[5],
                    mem_ptr[6],
                    mem_ptr[7],
                    human_readable__(mem_ptr[0]),
                    human_readable__(mem_ptr[1]),
                    human_readable__(mem_ptr[2]),
                    human_readable__(mem_ptr[3]),
                    human_readable__(mem_ptr[4]),
                    human_readable__(mem_ptr[5]),
                    human_readable__(mem_ptr[6]),
                    human_readable__(mem_ptr[7]),
                    mem_ptr[8],
                    mem_ptr[9],
                    mem_ptr[10],
                    mem_ptr[11],
                    mem_ptr[12],
                    mem_ptr[13],
                    mem_ptr[14],
                    mem_ptr[15],
                    human_readable__(mem_ptr[8]),
                    human_readable__(mem_ptr[9]),
                    human_readable__(mem_ptr[10]),
                    human_readable__(mem_ptr[11]),
                    human_readable__(mem_ptr[12]),
                    human_readable__(mem_ptr[13]),
                    human_readable__(mem_ptr[14]),
                    human_readable__(mem_ptr[15]));
            }
            else
            {
                printf(
                    "               | "
                        "%02X %02X %02X %02X %02X %02X %02X %02X | "
                            "%c%c%c%c%c%c%c%c\n",
                    mem_ptr[0],
                    mem_ptr[1],
                    mem_ptr[2],
                    mem_ptr[3],
                    mem_ptr[4],
                    mem_ptr[5],
                    mem_ptr[6],
                    mem_ptr[7],
                    human_readable__(mem_ptr[0]),
                    human_readable__(mem_ptr[1]),
                    human_readable__(mem_ptr[2]),
                    human_readable__(mem_ptr[3]),
                    human_readable__(mem_ptr[4]),
                    human_readable__(mem_ptr[5]),
                    human_readable__(mem_ptr[6]),
                    human_readable__(mem_ptr[7]));
            }

            printf(
                "               | ...\n"
                "               |\n"
                "               | USED (%lu %s%s%s)\n"
                "               |\n",
                HUMAN_READABLE_MEM_SIZE__(payload_size),
                HUMAN_READABLE_MEM_UNIT__(payload_size),
                is_mmapped ? " mmap() memory" : "",
                thread_arena ? " in thread arena" : "");
        }
        else
        {
            printf(
                "%14p +-----------------------------------\n"
                "   %s | %02X %02X %02X %02X %02X %02X %02X %02X\n"
                "               +-----------------------------------\n"
                "               | chunk_size = %lu %s | AMP = %X%X%X\n"
                "               +-----------------------------------\n"
                "               | next_free = %p\n"
                "               +-----------------------------------\n"
                "               | prev_free = %p\n"
                "               +-----------------------------------\n"
                "               |\n"
                "               | FREE (%lu %s)\n"
                "               |\n",
                p,
                is_top ? "(TOP CHUNK)" : "           ",
                (unsigned char) ((((chunk_t*) p)->overhead) >> 56),
                (unsigned char) ((((chunk_t*) p)->overhead) >> 48),
                (unsigned char) ((((chunk_t*) p)->overhead) >> 40),
                (unsigned char) ((((chunk_t*) p)->overhead) >> 32),
                (unsigned char) ((((chunk_t*) p)->overhead) >> 24),
                (unsigned char) ((((chunk_t*) p)->overhead) >> 16),
                (unsigned char) ((((chunk_t*) p)->overhead) >> 8),
                (unsigned char) (((chunk_t*) p)->overhead),
                HUMAN_READABLE_MEM_SIZE__(chunk_size),
                HUMAN_READABLE_MEM_UNIT__(chunk_size),
                thread_arena ? 0x1 : 0x0,
                is_mmapped ? 0x1 : 0x0,
                prev_in_use ? 0x1 : 0x0,
                (size_t*) ((chunk_t*) p)->next_free,
                (size_t*) ((chunk_t*) p)->prev_free,
                HUMAN_READABLE_MEM_SIZE__(payload_size),
                HUMAN_READABLE_MEM_UNIT__(payload_size));
            if(is_top)
            {
                size_t* p_end = (size_t*) (((char*) p) + chunk_size);
                if(p_end == sbrk(0)) //should be always the case
                {
                    printf(
                        "%14p +---------- TOP = sbrk(0) ----------\n",
                        p_end);
                }
                else
                {
                    printf(
                        "%14p +-------------- TOP ----------------\n",
                        p_end);
                }
            }
        }
    }

    //-------------------------------------------------------------------------
    // ARENAS AND HEAPS (from glibc's arena.c):
    //-------------------------------------------------------------------------
    // An ARENA is defined by the structure malloc_state_t (aka mstate) and a
    // HEAP is described by the structure heap_info_t at its bottom.
    //-------------------------------------------------------------------------
    // The struct malloc_state_t is used for all arenas. The first existing
    // arena is the main arena, which is the root of the main contiguous heap:
    //
    //      static struct malloc_state main_arena =
    //      {
    //          .mutex = _LIBC_LOCK_INITIALIZER,
    //          .next = &main_arena,
    //          .attached_threads = 1
    //      };
    //
    // All additionally dynamically allocated arenas must be linked to the main
    // arena by the next field!
    //
    // Mmapped chunks are standalone, not linked via arenas and allocated when
    // the size of the chunk exceeds DEFAULT_MMAP_THRESHOLD_MAX.
    // In that case, a chunk is not included in the main arena or any memory
    // region belonging to another arena, but the operating system is asked
    // for an exclusive memory region just for that chunk (via the mmap API
    // call), in which the chunk is placed. As the mmap API call returns
    // memory space in terms of pages, the minimum size of a MMAPPED chunk is
    // one page (which is at least 4096 bytes or a multiple of that) and is
    // evenly divisible by one page size. When an MMAPPED chunk is freed, the
    // whole memory space it is allocating is removed from the process space
    // and returned to the operating system.
    //
    // The main heap is a contiguous region of memory containing all chunks
    // of the main arena. While it is contiguous, it can however get split
    // up in multiple contiguous memory regions. Its describing malloc_state_t
    // struct is stored in the bss section of the mapped glibc library. The
    // main heap always ends at sbrk(0), which is the first invalid address
    // on top the heap.
    //
    // Different glibc implementations use diffrent malloc_state_t structure
    // to describe an arena. The following fields are the same for all
    // implementations:
    //
    //  struct malloc_state_t //aka mstate (represents a heap arena)
    //  {
    //      ... //at least 40 bytes
    //
    //      struct chunk_t* fastbinsY[NFASTBINS];
    //
    //      struct chunk_t* top; //top chunk
    //      struct chunk_t* last_remainder;
    //
    //      struct chunk_t* bins[2 * NBINS - 2];
    //
    //      unsigned int binmap[BINMAPSIZE]; //BINMAPSIZE
    //
    //      struct malloc_state_t* next; //= main_arena if pointing to itself
    //
    //      ... //more
    //  };
    //
    // At each heap of an allocated arena (not of the main arena) is a
    // heap_info_t structure stored:
    //
    //  struct heap_info_t
    //  {
    //      struct malloc_state_t* ar_ptr; //heap arena
    //      struct heap_info_t* prev; //previous heap
    //      size_t size; //heap size
    //
    //      ...//more
    //  };
    //
    // BE CAREFUL:
    //
    // Any heap arena can consist of several heaps (heap segments). Every heap
    // segment starts with a heap_info_t structure, but not the main arena.
    // The main arena is an exception and has just a malloc_state_t structure,
    // which is pre-allocated by glibc. The only reliable fact about all of
    // this is, that the heap_info_t structure is located exactly at the start
    // (bottom) of the heap and that the size field tells the total size of
    // the heap, as long as the heap segment is above sbrk(0), meaning as long
    // as we don't speak about the main heap. The heaps top chunk is located
    // exactly at the end of the each heap, meaning the next address is the
    // first one not inside the same heap segment. To find the bottom of an
    // allocated heap (= a heap above sbrk(0)), one needs to mask any chunk
    // address by ~(HEAP_MAX_SIZE - 1), as shown below (this mask potentially
    // sets the last 5 nibbles of the address to 0):
    //
    //   //Get the next chunk:
    //   size_t chunk_size = (size_t) (((chunk_t*) p)->size & ~FLAGS_MASK);
    //   size_t* next = (size_t*) (((char*) chunk_ptr) + chunk_size);
    //
    //   //If chunk is in a heap above sbrk(0):
    //   if(chunk_ptr > sbrk(0))
    //   {
    //       //Get the heap info:
    //       heap_info_t* heapinf = (heap_info_t*)
    //                                    ((size_t) p & ~(HEAP_MAX_SIZE - 1));
    //       //Determine the heap end:
    //       size_t* heap_end = (size_t*) (((char*) heapinf) + heapinf->size);
    //
    //       if(next > heap_end)
    //           ... //---> ERROR
    //
    //       if(next == heap_end)
    //           ... //---> TOP CHUNK
    //   }
    //
    // So in general the ARENA/HEAP layout is like that:
    //
    //     MAIN HEAP        ARENA 1 HEAP 1  top 1        ARENA 1 HEAP 2
    //  +-------------+<-------   +-------------+<----     +-------------+<----
    //  |  TOP CHUNK  | sbrk(0)   |             | top1     |  TOP CHUNK  | top2
    //  | MAIN ARENA  |           |             |          |   ARENA 1   |
    //  +-------------+<-+        +-------------+   +----->+-------------+
    //  |             |  |        |             |   |      |             |
    //  |             |  |        |             |   |      |             |
    //  +-------------+  |        +-------------+   |      +-------------+
    //  |             |  |        |             |   |      |             |
    //  |             |  |        |             |   |      |             |
    //  +-------------+  |        +-------------+   |      +-------------+
    //  |             |  |        |             |   |      |             |
    //  |             |  |        /             /   |      /             /
    //  +-------------+  |        /             /   |      /             /
    //  |             |  |        |             |   |      |             |
    //  |             |  |        |             |   |      +-------------+
    //  /             /  |        +-------------+   |      |             |
    //  /             /  |        |   BOTTOM    |   |      |             |
    //  |             |  |        |   CHUNK     |   |      +-------------+
    //  +-------------+  |        +=============+   |      |             |
    //  |             |  |        |   ARENA 1   |   |      |             |
    //  |             |  |        |             |   |      +-------------+
    //  |             |  |        |    top -----|---+      |   BOTTOM    |
    //  |             |  |    +---|--- next     |          |   CHUNK     |
    //  +-------------+  |    |   +=============+<--+---+  +=============+
    //  |             |  |    |   |  HEAP INFO  |   |   |  |  HEAP INFO  |
    //  |   BOTTOM    |  |    |   |     size    |   |   |  |    size     |
    //  |   CHUNK     |  |    |   | prev = NULL |   |   |  |    prev ----|----+
    //  |             |  |    |   |   ar_ptr ---|---+   |  |   ar_ptr ---|--+ |
    //  +-------------+  |    |   +=============+<---+  |  +=============+  | |
    //                   |    |                      |  +-------------------+ |
    //  +=============+  |    |                      +------------------------+
    //  |             |  |    |
    //  | MAIN ARENA  |  |    |    The first HEAP INFO
    //  |             |  |    |    shows up with a prev
    //  |    top -----|--+    |    field that is NULL
    //  |    next ----|----+  |
    //  |             |    /  |
    //  +=============+<--+---+
    //
    // As you can see: the top chunk of a heap segment cannot be determined
    // by the top pointer of the arena, but just by the top of the heap
    // segment, which is given by the heap info pointer. To get the heap info
    // you need to mask any chunk pointer by ~(HEAP_MAX_SIZE - 1)
    //
    //     heap_info_t* heapinf = (heap_info_t*)
    //                                  ((size_t) p & ~(HEAP_MAX_SIZE - 1));
    //
    // This means for example setting the lower 5 nibbles to zero (& ~0xFFFFF).
    // Then the end of the heap (first invalid address like top1, top2, in a
    // (size_t*) grid) is calculated:
    //
    //      size_t* heap_end = (size_t*) (((char*) heapinf) + heapinf->size);
    //
    // A trick to find the main arena, which is not published by glibc, is to
    // allocate memory in a thread (---> dedicated thread arena), then to find
    // the top chunk of the corresponding contiguous heap. Then to get the heap
    // info by masking the top chunk address with ~(HEAP_MAX_SIZE - 1). Then
    // to get the beginning of the malloc_state_t struct (ARENA 1) by the heap
    // info's ar_ptr. Then to parse the memory on top the ar_ptr and below the
    // first allocated chunk for a (size_t*) field that matches the address of
    // the found top chunk. Then to add a known offset
    //
    //    IDX_OFFS_TOP_NEXT = (2 + 2 * NBINS - 2 + BINMAPSIZE/2)
    //
    // to the top field to get the 'next' field, pointing to the next arena.
    // Now to follow the 'next' fields to find the next arenas, until one
    // 'next' field points to it own arena ---> this is the main arena.
    //-------------------------------------------------------------------------

    #define NUM_ADDR_FIELDS (NFASTBINS + 2 * NBINS - 2 + BINMAPSIZE/2 + 100)
    #define IDX_OFFS_TOP_NEXT (2 + 2 * NBINS - 2 + BINMAPSIZE/2)
    struct gen_ar_t //generic arena (implementation independent)
    {
        size_t* addr[NUM_ADDR_FIELDS];
    };

    struct heap_bott_t //just the first 3 fields are implementation independent
    {
        struct gen_ar_t* ar_ptr; //heap arena
        struct heap_bott_t* prev; //previous heap
        size_t size; //total heap size from bottom to top
    };

    //Get start of an allocated heap segment:
    inline heap_bott_t* get_start_of_allocated_heap_segment(size_t* p)
    {
        return (heap_bott_t*) ((size_t) p & ~(HEAP_MAX_SIZE - 1));
    }

    //Dump heap info:
    inline void dump_heap_info(size_t* p)
    {
        if(!p)
            return;
        heap_bott_t* hb = get_start_of_allocated_heap_segment(p);
        if(!hb)
            return;
        size_t* heap_end = (size_t*) (((char*) hb) + hb->size);
        printf(
            "%14p +========== HEAP INFO ==============\n"
            "               | ar_ptr = %p\n"
            "               +-----------------------------------\n"
            "               | prev = %p\n"
            "               +-----------------------------------\n"
            "               | size = %lu -> top = %p\n"
            "               +-----------------------------------\n"
            "               | ...\n"
            "               +===================================\n",
            (size_t*) hb,
            (size_t*) (hb->ar_ptr),
            (size_t*) (hb->prev),
            hb->size,
            heap_end);
    }

    //Get the memory pointer from the chunk pointer:
    inline void* get_mem_ptr(size_t* p)
    {
        if(!p)
            return (void*) 0;
        return (void*) (((char*) (p)) + (2 * sizeof(size_t)));
    }

    //Get the chunk pointer from the memory pointer:
    inline size_t* get_chunk(void* mem_ptr)
    {
        if(!mem_ptr)
            return (size_t*) 0;
        return (size_t*) (((char*) (mem_ptr)) - (2 * sizeof(size_t)));
    }

    //Get the full size of a chunk:
    inline size_t get_chunk_size(size_t* p)
    {
        if(!p)
            return 0;
        return (size_t) (((chunk_t*) p)->size & ~FLAGS_MASK);
    }

    //Get the next chunk pointer:
    inline size_t* get_next_chunk(size_t* p)
    {
        if(!p)
            return (size_t*) 0;
        const size_t& chunk_size =
                            (size_t) (((chunk_t*) p)->size & ~FLAGS_MASK);
        return (size_t*) (((char*) p) + chunk_size);
    }

    //Check if the chunk is is_in_use:
    inline bool is_in_use(size_t* p)
    {
        if(!p)
            return false;

        if(((chunk_t*) p)->size & M__)
            return true; //mmapped memory is always in use

        if(is_top_chunk(p))
            return false; //top is always a free chunk

        const size_t& chunk_size =
                            (size_t) (((chunk_t*) p)->size & ~FLAGS_MASK);
        size_t* next = (size_t*) (((char*) p) + chunk_size);
        if(!next)
            return false;
        return (((chunk_t*) next)->size & P__) ? true : false;
    }

    //Check if the chunk is mmapped:
    inline bool is_mmapped(size_t* p)
    {
        if(!p)
            return false;
        return (((chunk_t*) p)->size & M__) ? true : false;
    }

    //Check if the chunk belongs to a thread's arena:
    inline bool is_in_thread_arena(size_t* p)
    {
        if(!p)
            return false;
        return (((chunk_t*) p)->size & A__) ? true : false;
    }

    //Get the payload size (or potential payload size) of a chunk:
    inline size_t get_payload_size(size_t* p)
    {
        if(!p)
            return 0;
        const size_t& chunk_size =
                            (size_t) (((chunk_t*) p)->size & ~FLAGS_MASK);
        if(chunk_size < 2 * sizeof(size_t))
            return 0;
        return (size_t) (chunk_size - 2 * sizeof(size_t));
    }

#endif

//*****************************************************************************
// Include control (end):
//*****************************************************************************

#endif

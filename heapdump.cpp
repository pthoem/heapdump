//*****************************************************************************
// File ..................: heapdump.cpp
// Description ...........: Heap analysis
// Author ................: Peter Thoemmes
//-----------------------------------------------------------------------------
// Copyright (c) 2000-2018 Peter Thoemmes, Weinbergstrasse 3a, D-54441 Ockfen
//*****************************************************************************

//*****************************************************************************
// Header files:
//*****************************************************************************

#include "heapdump.h"

//*****************************************************************************
// Implementation:
//*****************************************************************************

//-----------------------------------------------------------------------------
// Defines:
//-----------------------------------------------------------------------------

//Maximum number of heaps per arena (for arrays to be put on the stack):
#define MAX_NUM_HEAPS 1024

//-----------------------------------------------------------------------------
// Initialize this module:
//-----------------------------------------------------------------------------

static size_t* max_addr__ = (size_t*) 0;
static size_t* max_user_addr__ = (size_t*) 0;
static size_t* stack_top__ = (size_t*) 0;

void init_stack__()
{
    int i = 0;
    if(&i < (int*) STACK_TOP32_2GB)
    {
        max_addr__ = (size_t*) MAX_ADDR_4GB;
        max_user_addr__ = (size_t*) MAX_ADDR_2GB;
        stack_top__ = STACK_TOP32_2GB;
    }
    else if(&i < (int*) STACK_TOP32_3GB)
    {
        max_addr__ = (size_t*) MAX_ADDR_4GB;
        max_user_addr__ = (size_t*) MAX_ADDR_3GB;
        stack_top__ = STACK_TOP32_3GB;
    }
    else
    {
        max_addr__ = (size_t*) MAX_ADDR_256TB;
        max_user_addr__ = (size_t*) MAX_ADDR_128TB;
        stack_top__ = STACK_TOP64;
    }
}

static size_t* heap_bottom_chunk__ = (size_t*) 0;

#if !defined(_WIN32) && !defined(_WIN64)

    static gen_ar_t* main_arena_ptr__ = (gen_ar_t*) 0;
    static int top_idx__ = -1;
    static int next_idx__ = -1;

#endif

#if defined(_WIN32) || defined(_WIN64)

    void init_heapdump(unsigned char verbose)
    {
        if(heap_bottom_chunk__)
            return;

        init_stack__();

        void* mem_ptr = (void*) 0;
        if(verbose)
        {
            mem_ptr = (void*) malloc(4 * sizeof(size_t));
            char* p = (char*) mem_ptr;
            *p = 'H';
            *(++p) = 'i';
            *(++p) = ' ';
            *(++p) = 'P';
            *(++p) = 'e';
            *(++p) = 't';
            *(++p) = 'e';
            *(++p) = 'r';
            *(++p) = (char) 0x0A;
        }

        _HEAPINFO hi;
        hi._pentry = NULL; //initate _heapwalk()

        size_t* chunk_ptr = (size_t*) 0;
        int heap_status = 0;
        while((heap_status = _heapwalk(&hi)) == _HEAPOK)
        {
            chunk_ptr = (size_t*) ((char*) hi._pentry);
            if(!heap_bottom_chunk__ || (heap_bottom_chunk__ > chunk_ptr))
                heap_bottom_chunk__ = chunk_ptr;
            if(verbose)
                dump_chunk(chunk_ptr,hi._size);
        }

        if(verbose)
        {
            free(mem_ptr);
            mem_ptr = (void*) 0;

            printf(
                "\n"
                "Verbose heap bottom determination...\n"
                "\n"
                "A walk over the heap, using _heapwalk() was done to\n"
                "find the heap bottom chunk, which sits at %p\n"
                "\n",
                heap_bottom_chunk__);
        }
    }

#else

    //-------------------------------------------------------------------------
    // Find the main arena:
    //-------------------------------------------------------------------------
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

    void* ma_finder(void* verbose_ptr)
    {
        unsigned char verbose =
                        (unsigned char) *((unsigned char*) verbose_ptr);

        #define NUM_CHUNKS 5

        void* mem_ptr[NUM_CHUNKS - 1];
        memset(mem_ptr,'0',NUM_CHUNKS * sizeof(void*));

        size_t* chunk_ptr[NUM_CHUNKS + 1];
        memset(chunk_ptr,'0',NUM_CHUNKS * sizeof(size_t*));

        //Allocate memory in a small chunk (32 bytes on 64 bit OS):
        if(verbose)
        {
            printf("ma_finder() thread goes allocating memory...");
            fflush(stdout);
        }
        mem_ptr[0] = malloc(4 * sizeof(size_t));
        chunk_ptr[0] = get_chunk(mem_ptr[0]);
        strcpy((char*) mem_ptr[0],"!! Peter\n");
        mem_ptr[1] = malloc(4 * sizeof(size_t));
        strcpy((char*) mem_ptr[1],"!! Claudia\n");
        mem_ptr[2] = malloc(4 * sizeof(size_t));
        strcpy((char*) mem_ptr[2],"!! Anna-Lena\n");
        mem_ptr[3] = malloc(4 * sizeof(size_t));
        strcpy((char*) mem_ptr[3],"!! Markus\n");
        if(verbose)
            printf("done.\n");

        //Get the all the other chunk pointers:
        if(verbose)
        {
            printf("ma_finder() thread goes determining top chunk pointer...");
            fflush(stdout);
        }
        chunk_ptr[1] = get_chunk(mem_ptr[1]);
        chunk_ptr[2] = get_chunk(mem_ptr[2]);
        chunk_ptr[3] = get_chunk(mem_ptr[3]);
        chunk_ptr[4] = get_next_chunk(chunk_ptr[3]); //should be TOP CHUNK
        size_t* bottom_chunk_ptr = chunk_ptr[0];
        size_t* top_chunk_ptr = chunk_ptr[4];
        if(verbose)
            printf("done.\n");

        //Access the struct heap_bott_t at the heap segment start:
        heap_bott_t* heap_info_ptr =
                        get_start_of_allocated_heap_segment(bottom_chunk_ptr);
        if(verbose)
        {
            printf(
                "ma_finder() thread got heap info from heap segment start...\n"
                "heap_info_ptr ..............: %p (heap segment start)\n"
                "   heap_info_ptr->ar_ptr ...: %p (heap arena)\n"
                "   heap_info_ptr->prev .....: %p%s\n"
                "   heap_info_ptr->size .....: %lu\n",
                (size_t*) heap_info_ptr,
                (size_t*) heap_info_ptr->ar_ptr,
                (size_t*) heap_info_ptr->prev,
                !heap_info_ptr->prev ? " (nil -> 1st heap info in arena)" : "",
                heap_info_ptr->size);
        }

        //Try to find the link to our top chunk:
        if(verbose)
        {
            printf(
                "ma_finder() thread tries to find the "
                "arena top chunk entry...");
            fflush(stdout);
        }
        top_idx__ = -1;
        next_idx__ = -1;
        gen_ar_t* ar_ptr = (gen_ar_t*) heap_info_ptr->ar_ptr;
        uint32 i = 0;
        for(;i < NUM_ADDR_FIELDS;++i)
        {
            if(top_idx__ < 0)
            {
                if(ar_ptr->addr[i] == top_chunk_ptr)
                {
                    top_idx__ = i;
                    next_idx__ = top_idx__ + IDX_OFFS_TOP_NEXT;
                    if(verbose)
                    {
                        size_t* p = (size_t*) &ar_ptr->addr[top_idx__];
                        printf("done.\n");
                        printf(
                            "ma_finder() found the arena's top field "
                            "(entry = 0x%012lX)\n"
                            "at address %p (index %u)...\n"
                            "  %p: 0x%012lX\n"
                            "  %p: 0x%012lX\n"
                            "  %p: 0x%012lX\n"
                            "  %p: 0x%012lX\n"
                            "  %p: 0x%012lX\n"
                            "  %p: 0x%012lX <--- ar_ptr->top\n"
                            "  %p: 0x%012lX\n"
                            "  %p: 0x%012lX\n"
                            "  %p: 0x%012lX\n"
                            "  %p: 0x%012lX\n"
                            "  %p: 0x%012lX\n"
                            "  %p: 0x%012lX\n",
                            (size_t) *p,
                            p,
                            top_idx__,
                            p - 5,
                            (size_t) *(p - 5),
                            p - 4,
                            (size_t) *(p - 4),
                            p - 3,
                            (size_t) *(p - 3),
                            p - 2,
                            (size_t) *(p - 2),
                            p - 1,
                            (size_t) *(p - 1),
                            p,
                            (size_t) *p,
                            p + 1,
                            (size_t) *(p + 1),
                            p + 2,
                            (size_t) *(p + 2),
                            p + 3,
                            (size_t) *(p + 3),
                            p + 4,
                            (size_t) *(p + 4),
                            p + 5,
                            (size_t) *(p + 5),
                            p + 6,
                            (size_t) *(p + 6));

                        p = (size_t*) &ar_ptr->addr[next_idx__];
                        printf("done.\n");
                        printf(
                            "ma_finder() found the arena's next field "
                            "(entry = 0x%012lX)\n"
                            "at address %p (index %u)...\n"
                            "  %p: 0x%012lX\n"
                            "  %p: 0x%012lX\n"
                            "  %p: 0x%012lX\n"
                            "  %p: 0x%012lX\n"
                            "  %p: 0x%012lX\n"
                            "  %p: 0x%012lX <--- ar_ptr->next\n"
                            "  %p: 0x%012lX\n"
                            "  %p: 0x%012lX\n"
                            "  %p: 0x%012lX\n"
                            "  %p: 0x%012lX\n"
                            "  %p: 0x%012lX\n"
                            "  %p: 0x%012lX\n",
                            (size_t) *p,
                            p,
                            next_idx__,
                            p - 5,
                            (size_t) *(p - 5),
                            p - 4,
                            (size_t) *(p - 4),
                            p - 3,
                            (size_t) *(p - 3),
                            p - 2,
                            (size_t) *(p - 2),
                            p - 1,
                            (size_t) *(p - 1),
                            p,
                            (size_t) *p,
                            p + 1,
                            (size_t) *(p + 1),
                            p + 2,
                            (size_t) *(p + 2),
                            p + 3,
                            (size_t) *(p + 3),
                            p + 4,
                            (size_t) *(p + 4),
                            p + 5,
                            (size_t) *(p + 5),
                            p + 6,
                            (size_t) *(p + 6));
                    }
                    break;
                }
            }
        }
        //Try to find the main arena:
        if((top_idx__ < 0) || (next_idx__ < 0))
        {
            if(verbose)
                printf("FAILED!\n");
        }
        else
        {
            if(verbose)
            {
                printf(
                    "ma_finder() thread will step forward (using "
                    "the 'next' field) searching\n"
                    "for the main arena...\n");
                fflush(stdout);
            }
            gen_ar_t* start_ar_ptr = ar_ptr;
            for(;ar_ptr;)
            {
                if(ar_ptr->addr[top_idx__])
                {
                    if(ar_ptr->addr[top_idx__] <= sbrk(0))
                    {
                        main_arena_ptr__ = ar_ptr;
                        if(verbose)
                        {
                            size_t* p = (size_t*) main_arena_ptr__;
                            printf(
                                "ma_finder() found the main arena "
                                "at address %p...\n"
                                "  %p: 0x%012lX\n"
                                "  %p: 0x%012lX\n"
                                "  %p: 0x%012lX\n"
                                "  %p: 0x%012lX\n"
                                "  %p: 0x%012lX\n"
                                "  %p: 0x%012lX <--- &main_arena\n"
                                "  %p: 0x%012lX\n"
                                "  %p: 0x%012lX\n"
                                "  %p: 0x%012lX\n"
                                "  %p: 0x%012lX\n"
                                "  %p: 0x%012lX\n"
                                "  %p: 0x%012lX\n",
                                p,
                                p - 5,
                                (size_t) *(p - 5),
                                p - 4,
                                (size_t) *(p - 4),
                                p - 3,
                                (size_t) *(p - 3),
                                p - 2,
                                (size_t) *(p - 2),
                                p - 1,
                                (size_t) *(p - 1),
                                p,
                                (size_t) *p,
                                p + 1,
                                (size_t) *(p + 1),
                                p + 2,
                                (size_t) *(p + 2),
                                p + 3,
                                (size_t) *(p + 3),
                                p + 4,
                                (size_t) *(p + 4),
                                p + 5,
                                (size_t) *(p + 5),
                                p + 6,
                                (size_t) *(p + 6));
                            printf(
                                "ma_finder() thread found the main arena:\n"
                                "   &main_arena ....: %p\n"
                                "          top .....: %p (sbrk(0) = %p)\n"
                                "          next ....: %p\n",
                                main_arena_ptr__,
                                main_arena_ptr__->addr[top_idx__],
                                sbrk(0),
                                main_arena_ptr__->addr[next_idx__]);
                        }
                        break;
                    }
                    if(verbose)
                    {
                        printf(
                            "ma_finder() thread found a thread arena:\n"
                            "   ar_ptr .........: %p\n"
                            "          top .....: %p\n"
                            "          next ....: %p\n",
                            ar_ptr,
                            ar_ptr->addr[top_idx__],
                            ar_ptr->addr[next_idx__]);
                    }
                }
                if(!ar_ptr->addr[next_idx__])
                    break;
                ar_ptr = (gen_ar_t*) ar_ptr->addr[next_idx__];
                if(ar_ptr == start_ar_ptr)
                    break; //linked list looped back to start
            }
            if(!main_arena_ptr__)
            {
                if(verbose)
                    printf("FAILED!\n");
            }
        }

        //Free all heap memory:
        if(verbose)
        {
            printf("ma_finder() thread frees the memory again...");
            fflush(stdout);
        }
        for(i = 0;i < (NUM_CHUNKS - 1);++i)
        {
            free(mem_ptr[i]);
            mem_ptr[i] = (void*) 0;
        }
        if(verbose)
            printf("done.\n");

        return (void*) 0;
    }

    void init_heapdump(unsigned char verbose)
    {
        if(heap_bottom_chunk__)
            return;

        init_stack__();

        //Allocate a small chunk of memory to find the bottom chunk:
        if(!verbose)
        {
            //Allocate memory in a small chunk (32 bytes on 64 bit OS):
            void* mem_ptr = malloc(4 * sizeof(size_t));

            //Get the heap bottom chunk:
            heap_bottom_chunk__ = get_chunk(mem_ptr);

            //Free the heap memory:
            free(mem_ptr);
            mem_ptr = (void*) 0;
        }
        else
        {
            #define NUM_CHUNKS 5

            void* mem_ptr[NUM_CHUNKS - 1];
            memset(mem_ptr,'0',NUM_CHUNKS * sizeof(void*));

            size_t* chunk_ptr[NUM_CHUNKS + 1];
            memset(chunk_ptr,'0',NUM_CHUNKS * sizeof(size_t*));

            //Allocate memory in a small chunk (32 bytes on 64 bit OS):
            mem_ptr[0] = malloc(4 * sizeof(size_t));
            strcpy((char*) mem_ptr[0],"Hi Peter\n");
            mem_ptr[1] = malloc(4 * sizeof(size_t));
            strcpy((char*) mem_ptr[1],"Hi Claudia\n");
            mem_ptr[2] = malloc(4 * sizeof(size_t));
            strcpy((char*) mem_ptr[2],"Hi Anna-Lena\n");
            mem_ptr[3] = malloc(4 * sizeof(size_t));
            strcpy((char*) mem_ptr[3],"Hi Markus\n");

            //Get the heap bottom chunk:
            chunk_ptr[0] = get_chunk(mem_ptr[0]);
            heap_bottom_chunk__ = chunk_ptr[0];

            //Get the all the other chunk pointers:
            chunk_ptr[1] = get_chunk(mem_ptr[1]);
            chunk_ptr[2] = get_chunk(mem_ptr[2]);
            chunk_ptr[3] = get_chunk(mem_ptr[3]);
            chunk_ptr[4] = get_next_chunk(chunk_ptr[3]); //should be TOP CHUNK

            //Dump the chunks:
            printf(
                "Verbose heap bottom determination...\n"
                "\n"
                "A few malloc() allocations have been made at the start \n"
                "of the program to find the heap bottom chunk, which\n"
                "was then found at %p:\n"
                "\n",
                heap_bottom_chunk__);
            dump_chunk(chunk_ptr[0]);
            dump_chunk(chunk_ptr[1]);
            dump_chunk(chunk_ptr[2]);
            dump_chunk(chunk_ptr[3]);
            dump_chunk(chunk_ptr[4]);
            printf("\n");

            //Free all heap memory:
            uint32 i = 0;
            for(;i < (NUM_CHUNKS - 1);++i)
            {
                free(mem_ptr[i]);
                mem_ptr[i] = (void*) 0;
            }
        }

        //Start a thread to find the main arena:
        if(verbose)
        {
            printf(
                "+---------------------------------------"
                "---------------------------------------\n"
                "| ma_finder() thread will be started "
                "to find the main arena!\n"
                "+---------------------------------------"
                "---------------------------------------\n");
        }
        pthread_t pth_ma_finder = (pthread_t) 0;
        pthread_create(&pth_ma_finder,NULL,ma_finder,(void*) &verbose);
        pthread_join(pth_ma_finder,NULL);
        if(verbose)
        {
            if(main_arena_ptr__)
            {
                printf(
                    "+---------------------------------------"
                    "---------------------------------------\n"
                    "| ma_finder() thread terminated successful!\n"
                    "+---------------------------------------"
                    "---------------------------------------\n");
                printf(
                    "The main arena was found:\n"
                    "\n"
                    "   &main_arena ....: %p\n"
                    "          top .....: %p (sbrk(0) = %p)\n"
                    "          next ....: %p\n"
                    "\n",
                    main_arena_ptr__,
                    main_arena_ptr__->addr[top_idx__],
                    sbrk(0),
                    main_arena_ptr__->addr[next_idx__]);
                dump_chunk(main_arena_ptr__->addr[top_idx__]);
            }
            else
            {
                printf(
                    "+---------------------------------------"
                    "---------------------------------------\n"
                    "| ma_finder() thread terminated without "
                    "finding the main arena!\n"
                    "+---------------------------------------"
                    "---------------------------------------\n");
            }
            printf("\n");
        }
    }

#endif

//-----------------------------------------------------------------------------
// Get the memory dimensions:
//-----------------------------------------------------------------------------

bool get_memory_dimenions(
                        size_t** max_addr,
                        size_t** max_user_addr,
                        size_t** stack_top,
                        size_t** heap_bottom_chunk)
{
    *max_addr = max_addr__;
    *max_user_addr = max_user_addr__;
    *stack_top = stack_top__;
    *heap_bottom_chunk = heap_bottom_chunk__;
    if(!max_addr__ || !max_user_addr__ || !stack_top__ || !heap_bottom_chunk__)
        return false;
    return true;
}

//-----------------------------------------------------------------------------
// Get the heap bottom chunk:
//-----------------------------------------------------------------------------
// Return: pointer to heap bottom chunk or NULL if module was not initialized
//-----------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)

    size_t* get_heap_bottom_chunk()
    {
        _HEAPINFO hi;
        hi._pentry = NULL; //initate _heapwalk()
        int heap_status = _heapwalk(&hi);
        if(heap_status == _HEAPOK)
        {
            heap_bottom_chunk__ = (size_t*) hi._pentry;
            return heap_bottom_chunk__;
        }

        return (size_t*) 0;;
    }

#else

    size_t* get_heap_bottom_chunk()
    {
        return heap_bottom_chunk__;
    }

#endif

//-----------------------------------------------------------------------------
// Get the current HEAP limit:
//-----------------------------------------------------------------------------
// Returns the full size of the heap
//-----------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)

    size_t get_current_contiguous_heap_limit(
                                size_t** heap_top_end,
                                size_t** heap_top_chunk)
    {
        *heap_top_end = (size_t*) 0;
        *heap_top_chunk = (size_t*) 0;

        _HEAPINFO hi;
        hi._pentry = NULL; //initate _heapwalk()

        size_t* chunk_ptr = (size_t*) 0;
        size_t heap_size = 0;
        int heap_status = 0;
        while((heap_status = _heapwalk(&hi)) == _HEAPOK)
        {
            chunk_ptr = (size_t*) ((char*) hi._pentry);
            if(*heap_top_chunk < chunk_ptr)
            {
                *heap_top_chunk = chunk_ptr;
                *heap_top_end = (size_t*)(((char*) heap_top_chunk) + hi._size);
            }
            heap_size += hi._size;
        }

        return heap_size;
    }

#else

    size_t get_current_contiguous_heap_limit(
                                size_t** heap_top_end,
                                size_t** heap_top_chunk)
    {
        *heap_top_end = (size_t*) sbrk(0);
        *heap_top_chunk = (size_t*) 0;

        size_t heap_size = 0;
        size_t chunk_size = 0;
        size_t* next_chunk_ptr = (size_t*) 0;
        size_t* chunk_ptr = heap_bottom_chunk__;
        for(;;)
        {
            chunk_size = get_chunk_size(chunk_ptr);
            heap_size += chunk_size;

            next_chunk_ptr = get_next_chunk(chunk_ptr);
            if(next_chunk_ptr == sbrk(0))
            {
                *heap_top_chunk = chunk_ptr;
                break; //top reached
            }
            if(next_chunk_ptr > sbrk(0))
            {
                heap_size = 0;
                break; //error
            }

            chunk_ptr = next_chunk_ptr;
        }

        return heap_size;
    }

#endif

//-----------------------------------------------------------------------------
// Get the chunk size of the allocated memory around a pointer:
//-----------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)

    size_t get_allocated_chunk_size(void* mem_ptr)
    {
        if(!mem_ptr)
            return 0;

        _HEAPINFO hi;
        hi._pentry = NULL; //initate _heapwalk()

        int heap_status = 0;
        while((heap_status = _heapwalk(&hi)) == _HEAPOK)
        {
            #ifdef _DEBUG //mem_ptr = chunk_ptr + DBG_HDR_SIZE
                size_t* mptr = (size_t*) (((char*) hi._pentry) + DBG_HDR_SIZE);
            #else //mem_ptr = chunk_ptr
                size_t* mptr = (size_t*) ((char*) hi._pentry);
            #endif
            if(mptr == mem_ptr)
                return hi._size;
        }
        return 0;
    }

#else

    size_t get_allocated_chunk_size(void* mem_ptr)
    {
        if(!mem_ptr)
            return 0;
        size_t* chunk_ptr = get_chunk(mem_ptr);
        return get_chunk_size(chunk_ptr);
    }

#endif

//-----------------------------------------------------------------------------
// Get the payload size behind the pointer:
//-----------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)

    size_t get_allocated_payload_size(void* mem_ptr)
    {
        if(!mem_ptr)
            return 0;
        return _msize(mem_ptr);
    }

#else

    size_t get_allocated_payload_size(void* mem_ptr)
    {
        if(!mem_ptr)
            return 0;
        size_t* chunk_ptr = get_chunk(mem_ptr);
        return get_payload_size(chunk_ptr);
    }

#endif

//-----------------------------------------------------------------------------
// Dump the total heap footprint:
//-----------------------------------------------------------------------------
// Attention: do not allocate heap inside (no STL) -> only use stack!!!
//-----------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)

    void dump_heap_footprint()
    {
        size_t* heap_top_end = (size_t*) 0;
        size_t* heap_top_chunk = (size_t*) 0;
        size_t* heap_bottom_chunk = (size_t*) 0;

        printf("\n");

        _HEAPINFO hi;
        hi._pentry = NULL; //initate _heapwalk()

        size_t used_total = 0;
        size_t free_total = 0;
        #ifdef _DEBUG
            size_t* mem_ptr = (size_t*) 0;
        #endif
        size_t* chunk_ptr = (size_t*) 0;
        size_t heap_size = 0;
        int heap_status = 0;
        while((heap_status = _heapwalk(&hi)) == _HEAPOK)
        {
            chunk_ptr = (size_t*) ((char*) hi._pentry);
            if(!heap_bottom_chunk || (heap_bottom_chunk > chunk_ptr))
                heap_bottom_chunk = chunk_ptr;
            if(heap_top_chunk < chunk_ptr)
            {
                heap_top_chunk = chunk_ptr;
                heap_top_end = (size_t*) (((char*) heap_top_chunk) + hi._size);
            }
            heap_size += hi._size;

            #ifdef _DEBUG
                mem_ptr = (size_t*) (((char*) chunk_ptr) + DBG_HDR_SIZE);
                printf(
                    "%8p  mem: %8p  %10u bytes %s\n",
                    chunk_ptr,
                    mem_ptr,
                    hi._size,
                    hi._useflag == _USEDENTRY ? "USED" : "FREE");
            #else
                printf(
                    "%8p   %10u bytes %s\n",
                    chunk_ptr,
                    hi._size,
                    hi._useflag == _USEDENTRY ? "USED" : "FREE");
            #endif
            if(hi._useflag == _USEDENTRY)
                used_total += hi._size;
            else
                free_total += hi._size;
        }

        switch(heap_status)
        {
            case _HEAPEMPTY:
                printf("Heap is empty\n");
                break;
            case _HEAPEND:
                printf(
                    "\n"
                    "         +--------------------------+ STACK TOP\n"
                    "         |          STACK           |\n"
                    "         +---||------||-------||----+\n"
                    "         |   \\/      \\/       \\/    |\n"
                    "         |        Free Space        |\n"
                    "         |   /\\      /\\       /\\    |\n"
                    "%8p +---||------||-------||----+ HEAP TOP\n"
                    "         |                          |\n"
                    "         |          HEAP            |\n"
                    "         | %10lu %s size       |\n"
                    "         |                          |\n"
                    "         | %10lu %s used       |\n"
                    "         | %10lu %s free       |\n"
                    "         |                          |\n"
                    "%8p +--------------------------+ HEAP BOTTOM\n"
                    "\n",
                    heap_top_end,
                    HUMAN_READABLE_MEM_SIZE__(heap_size),
                    HUMAN_READABLE_MEM_UNIT_2__(heap_size),
                    HUMAN_READABLE_MEM_SIZE__(used_total),
                    HUMAN_READABLE_MEM_UNIT_2__(used_total),
                    HUMAN_READABLE_MEM_SIZE__(free_total),
                    HUMAN_READABLE_MEM_UNIT_2__(free_total),
                    heap_bottom_chunk);
                break;
            case _HEAPBADPTR:
                printf("ERROR - bad pointer to heap\n");
                break;
            case _HEAPBADBEGIN:
                printf("ERROR - bad start of heap\n");
                break;
            case _HEAPBADNODE:
                printf("ERROR - bad node in heap\n");
                break;
        }
    }

#else

    void dump_heap_footprint()
    {
        if(!heap_bottom_chunk__)
        {
            printf("ERROR - heap_bottom_chunk__ was not initialized\n");
            return;
        }

        //Start with the main arena:
        gen_ar_t* ar_ptr = main_arena_ptr__;
        size_t* chunk_ptr = heap_bottom_chunk__;

        //Storage for heap bottoms of further arenas:
        size_t baddr_arr[MAX_NUM_HEAPS]; //bottom pointer addresses as size_t
        memset(baddr_arr,0,sizeof(size_t) * MAX_NUM_HEAPS);
        size_t* baddr_arr_ptr = (size_t*) &baddr_arr[0];
        size_t bott_idx = 0;
        size_t num_botts = 0;

        size_t used_total = 0;
        size_t free_total = 0;
        size_t heap_size = 0;
        bool in_use = false;
        void* mem_ptr = (void*) 0;
        size_t chunk_size = 0;
        size_t* next_chunk_ptr = (size_t*) 0;
        printf("--------- MAIN ARENA: ---------\n\n");
        for(;;)
        {
            chunk_size = get_chunk_size(chunk_ptr);
            if(!chunk_size)
            {
                printf("ERROR - bad chunk at %p\n",chunk_ptr);
                return;
            }
            heap_size += chunk_size;

            if(is_top_chunk(chunk_ptr))
            {
                free_total += chunk_size;
                printf(
                    "%14p  * !HEAP TOP CHUNK! * %10lu bytes FREE\n",
                    chunk_ptr,
                    chunk_size);
                if(ar_ptr)
                {
                    //Try to get the bottom of the next heap of the arena:
                    if(++bott_idx < num_botts)
                    {
                        chunk_ptr = (size_t*) baddr_arr[bott_idx];
                        if(chunk_ptr)
                        {
                            printf("\n");
                            continue; //dump next heap
                        }
                    }

                    //Delete all heap bottoms:
                    memset(baddr_arr,0,sizeof(size_t) * MAX_NUM_HEAPS);
                    bott_idx = 0;
                    num_botts = 0;

                    //Try to get another arena:
                    ar_ptr = (gen_ar_t*) get_next_arena((size_t*) ar_ptr);
                    if(ar_ptr)
                    {
                        //Get all heap bottoms of the arena:
                        num_botts = get_all_bottom_chunks_of_arena(
                                                       ar_ptr->addr[top_idx__],
                                                       &baddr_arr_ptr,
                                                       MAX_NUM_HEAPS);
                        if(num_botts)
                            chunk_ptr = (size_t*) baddr_arr[bott_idx];

                        //Start next arena in case:
                        if(chunk_ptr)
                        {
                            printf("\n");
                            printf("--------- NEXT ARENA: ---------\n\n");
                            continue;
                        }
                    }
                }
                break;
            }

            mem_ptr = get_mem_ptr(chunk_ptr);

            in_use = is_in_use(chunk_ptr);

            printf(
                "%14p  mem: %14p  %10lu bytes %s\n",
                chunk_ptr,
                mem_ptr,
                chunk_size,
                in_use ? "USED" : "FREE");

            if(in_use)
                used_total += chunk_size;
            else
                free_total += chunk_size;

            next_chunk_ptr = get_next_chunk(chunk_ptr);
            chunk_ptr = next_chunk_ptr;
        }

        printf(
            "\n"
            "                 +--------------------------+ STACK TOP\n"
            "                 |          STACK           |\n"
            "                 +---||------||-------||----+\n"
            "                 |   \\/      \\/       \\/    |\n"
            "                 |        Free Space        |\n"
            "                 |   /\\      /\\       /\\    |\n"
            "                 +---||------||-------||----+\n"
            "                 |                          |\n"
            "                 |          HEAP            |\n"
            "                 | %10lu %s size       |\n"
            "                 |                          |\n"
            "                 | %10lu %s used       |\n"
            "                 | %10lu %s free       |\n"
            "                 |                          |\n"
            "%16p +--------------------------+ HEAP BOTTOM\n"
            "\n",
            HUMAN_READABLE_MEM_SIZE__(heap_size),
            HUMAN_READABLE_MEM_UNIT_2__(heap_size),
            HUMAN_READABLE_MEM_SIZE__(used_total),
            HUMAN_READABLE_MEM_UNIT_2__(used_total),
            HUMAN_READABLE_MEM_SIZE__(free_total),
            HUMAN_READABLE_MEM_UNIT_2__(free_total),
            heap_bottom_chunk__);
    }

#endif

//-----------------------------------------------------------------------------
// Dump heap details for debugging:
//-----------------------------------------------------------------------------
// Attention: do not allocate heap inside (no STL) -> only use stack!!!
//-----------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)

    void dump_heap_details(
                    size_t* /* start_chunk */,
                    size_t* /* heap_top_end */) //first invalid address
    {
        _HEAPINFO hi;
        hi._pentry = NULL; //initate _heapwalk()

        size_t* chunk_ptr = (size_t*) 0;
        int heap_status = 0;
        while((heap_status = _heapwalk(&hi)) == _HEAPOK)
        {
            chunk_ptr = (size_t*) ((char*) hi._pentry);
            dump_chunk(chunk_ptr,hi._size);
        }
        printf("\n");
    }

#else

    void dump_heap_details(
                    size_t* start_chunk,
                    size_t* heap_top_end) //first invalid address
    {
        if(!start_chunk)
        {
            printf("ERROR - start_chunk address is missing\n");
            return;
        }
        if(!heap_top_end)
        {
            printf("ERROR - heap top address is missing\n");
            return;
        }
        if(start_chunk >= heap_top_end)
        {
            printf("ERROR - start chunk address is too big\n");
            return;
        }

        //Starting arena:
        gen_ar_t* ar_ptr = (gen_ar_t*) get_arena(start_chunk);
        heap_bott_t* hb = (heap_bott_t*) 0;
        if(ar_ptr == main_arena_ptr__)
        {
            printf("--------- MAIN ARENA at %p:\n\n",main_arena_ptr__);
            printf("          HEAP at %p:\n\n",heap_bottom_chunk__);
        }
        else
        {
            hb = get_start_of_allocated_heap_segment(start_chunk);
            printf("--------- ALLOCATED ARENA at %p:\n\n",ar_ptr);
            printf("          HEAP at %p:\n\n",(size_t*) hb);
            dump_heap_info(start_chunk);
        }

        //Storage for heap bottoms of further arenas:
        size_t baddr_arr[MAX_NUM_HEAPS]; //bottom pointer addresses as size_t
        memset(baddr_arr,0,sizeof(size_t) * MAX_NUM_HEAPS);
        size_t* baddr_arr_ptr = (size_t*) &baddr_arr[0];
        size_t bott_idx = 0;
        size_t num_botts = 0;

        size_t chunk_size = 0;
        size_t* next_chunk_ptr = (size_t*) 0;
        size_t* chunk_ptr = start_chunk;
        for(;;)
        {
            if(chunk_ptr >= heap_top_end)
                break;

            chunk_size = get_chunk_size(chunk_ptr);
            if(!chunk_size)
            {
                printf("ERROR - bad chunk at %p\n",chunk_ptr);
                return;
            }

            dump_chunk(chunk_ptr);

            if(is_top_chunk(chunk_ptr))
            {
                if(ar_ptr)
                {
                    //Try to get the bottom of the next heap of the arena:
                    if(++bott_idx < num_botts)
                    {
                        chunk_ptr = (size_t*) baddr_arr[bott_idx];
                        if(chunk_ptr)
                        {
                            hb = get_start_of_allocated_heap_segment(
                                                                chunk_ptr);
                            heap_top_end = get_heap_top_end(chunk_ptr);
                            printf("\n");
                            printf("          HEAP at %p:\n\n",(size_t*) hb);
                            dump_heap_info(chunk_ptr);
                            continue; //next heap
                        }
                    }

                    //Delete all heap bottoms:
                    memset(baddr_arr,0,sizeof(size_t) * MAX_NUM_HEAPS);
                    bott_idx = 0;
                    num_botts = 0;

                    //Try to get another arena:
                    ar_ptr = (gen_ar_t*) get_next_arena((size_t*) ar_ptr);
                    if(ar_ptr)
                    {
                        //Get all heap bottoms of the arena:
                        num_botts = get_all_bottom_chunks_of_arena(
                                                       ar_ptr->addr[top_idx__],
                                                       &baddr_arr_ptr,
                                                       MAX_NUM_HEAPS);
                        if(num_botts)
                            chunk_ptr = (size_t*) baddr_arr[bott_idx];

                        //Start next arena in case:
                        if(chunk_ptr)
                        {
                            hb = get_start_of_allocated_heap_segment(
                                                                chunk_ptr);
                            heap_top_end = get_heap_top_end(chunk_ptr);
                            printf("\n");
                            printf(
                                "--------- ALLOCATED ARENA at %p:\n\n",ar_ptr);
                            printf("          HEAP at %p:\n\n",(size_t*) hb);
                            dump_heap_info(chunk_ptr);
                            continue; //next arena
                        }
                    }
                }
                break;
            }

            next_chunk_ptr = get_next_chunk(chunk_ptr);
            chunk_ptr = next_chunk_ptr;
        }
        printf("\n");
    }

#endif

//-----------------------------------------------------------------------------
// HEX dump of the heap:
//-----------------------------------------------------------------------------
// Attention: do not allocate heap inside (no STL) -> only use stack!!!
//-----------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)

    void dump_heap_hex(
                    size_t* /* start_chunk */,
                    size_t* /* heap_top_end */, //first invalid address
                    uint32 max_kb)
    {
        _HEAPINFO hi;
        hi._pentry = NULL; //initate _heapwalk()

        size_t max_bytes = max_kb * 1024; //KB ---> bytes
        size_t output_cnt = 0;

        char* ptr = (char*) 0;
        size_t* this_chunk_top = (size_t*) 0;
        int heap_status = 0;
        while((heap_status = _heapwalk(&hi)) == _HEAPOK)
        {
            ptr = (char*) hi._pentry;
            this_chunk_top = (size_t*) (ptr + hi._size);

            for(;((size_t*) ptr) < this_chunk_top;ptr += 8)
            {
                printf(
                    "%02X %02X %02X %02X   %02X %02X %02X %02X | "
                    "%c%c%c%c %c%c%c%c %12p ... %12p\n",
                    (unsigned char) ptr[0],
                    (unsigned char) ptr[1],
                    (unsigned char) ptr[2],
                    (unsigned char) ptr[3],
                    (unsigned char) ptr[4],
                    (unsigned char) ptr[5],
                    (unsigned char) ptr[6],
                    (unsigned char) ptr[7],
                    human_readable__(ptr[0]),
                    human_readable__(ptr[1]),
                    human_readable__(ptr[2]),
                    human_readable__(ptr[3]),
                    human_readable__(ptr[4]),
                    human_readable__(ptr[5]),
                    human_readable__(ptr[6]),
                    human_readable__(ptr[7]),
                    &ptr[0],
                    &ptr[7]);
            }

            if(max_bytes)
            {
                output_cnt += hi._size;
                if(output_cnt >= max_bytes)
                {
                    printf("\n");
                    printf(">>> INTERRUPTED after %u KB <<<\n",max_kb);
                    break;
                }
            }
        }
        if(output_cnt < max_bytes)
        {
            printf("\n");
            if(output_cnt < (100 * 1024))
            {
                printf(
                    "TOTAL: %5.3lf KB\n",
                    (double) (((double) output_cnt)/1024.0));
            }
            else
            {
                printf("TOTAL: %lu KB\n",output_cnt/1024);
            }
        }
        printf("\n");
    }

#else

    void dump_heap_hex(
                    size_t* start_chunk,
                    size_t* heap_top_end, //first invalid address
                    uint32 max_kb)
    {
        if(!start_chunk)
        {
            printf("ERROR - start_chunk address is missing\n");
            return;
        }
        if(!heap_top_end)
        {
            printf("ERROR - heap top address is missing\n");
            return;
        }
        if(start_chunk >= heap_top_end)
        {
            printf("ERROR - start chunk address is too big\n");
            return;
        }

        //Starting arena:
        gen_ar_t* ar_ptr = (gen_ar_t*) get_arena(start_chunk);
        heap_bott_t* hb = (heap_bott_t*) 0;
        if(ar_ptr == main_arena_ptr__)
        {
            printf(
                "MAIN ARENA at %p (MAIN HEAP at %p):\n\n",
                main_arena_ptr__,
                heap_bottom_chunk__);
        }
        else
        {
            hb = get_start_of_allocated_heap_segment(start_chunk);
            printf(
                "ALLOCATED ARENA at %p (HEAP at %p):\n\n",
                ar_ptr,
                (size_t*) hb);
        }

        size_t max_bytes = max_kb * 1024; //KB ---> bytes
        size_t output_cnt = 0;
        size_t zero_word_cnt = 0;

        size_t num_hidden_zero_blocks = 0;
        char* p = (char*) 0;
        char* ptr = (char*) start_chunk;
        for(;((size_t*) ptr) < heap_top_end;ptr += 8)
        {
            if(!ptr[0] && !ptr[1] && !ptr[2] && !ptr[3] &&
               !ptr[4] && !ptr[5] && !ptr[6] && !ptr[7])
            {
                ++zero_word_cnt;
            }
            else
            {
                if(zero_word_cnt >= 9)
                {
                    num_hidden_zero_blocks =
                                        (zero_word_cnt - 9) * sizeof(size_t);
                    if(num_hidden_zero_blocks)
                    {
                        printf(
                            "%lu more zero words "
                            "-> total zero block size: %lu bytes\n",
                            num_hidden_zero_blocks,
                            zero_word_cnt*sizeof(size_t));
                    }
                    else
                    {
                        printf(
                            "-> total zero block size: %lu bytes\n",
                            zero_word_cnt*sizeof(size_t));
                    }
                    p = ptr - 8;
                    printf(
                       "00 00 00 00   00 00 00 00 | .... .... %14p ... %14p\n",
                       &p[0],
                       &p[7]);
                }
                zero_word_cnt = 0;
            }
            if(zero_word_cnt < 9)
            {
                printf(
                    "%02X %02X %02X %02X   %02X %02X %02X %02X | "
                    "%c%c%c%c %c%c%c%c %14p ... %14p\n",
                    (unsigned char) ptr[0],
                    (unsigned char) ptr[1],
                    (unsigned char) ptr[2],
                    (unsigned char) ptr[3],
                    (unsigned char) ptr[4],
                    (unsigned char) ptr[5],
                    (unsigned char) ptr[6],
                    (unsigned char) ptr[7],
                    human_readable__(ptr[0]),
                    human_readable__(ptr[1]),
                    human_readable__(ptr[2]),
                    human_readable__(ptr[3]),
                    human_readable__(ptr[4]),
                    human_readable__(ptr[5]),
                    human_readable__(ptr[6]),
                    human_readable__(ptr[7]),
                    &ptr[0],
                    &ptr[7]);
            }

            if(max_bytes)
            {
                output_cnt += 8;
                if(output_cnt >= max_bytes)
                {
                    printf("\n");
                    printf(">>> INTERRUPTED after %u KB <<<\n",max_kb);
                    break;
                }
            }
        }
        if(output_cnt < max_bytes)
        {
            printf("\n");
            if(output_cnt < (100 * 1024))
            {
                printf(
                    "TOTAL: %5.3lf KB\n",
                    (double) (((double) output_cnt)/1024.0));
            }
            else
            {
                printf("TOTAL: %lu KB\n",output_cnt/1024);
            }
        }
        printf("\n");
    }

#endif

//-----------------------------------------------------------------------------
// Raw heap dump:
//-----------------------------------------------------------------------------
// Attention: do not allocate heap inside (no STL) -> only use stack!!!
//-----------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)

    void dump_heap_raw(
                    size_t* /* start_chunk */,
                    size_t* /* heap_top_end */, //first invalid address
                    uint32 max_kb)
    {
        _HEAPINFO hi;
        hi._pentry = NULL; //initate _heapwalk()

        size_t max_bytes = max_kb * 1024; //KB ---> bytes
        size_t output_cnt = 0;

        char* ptr = (char*) 0;
        size_t* this_chunk_top = (size_t*) 0;
        int heap_status = 0;
        while((heap_status = _heapwalk(&hi)) == _HEAPOK)
        {
            ptr = (char*) hi._pentry;
            this_chunk_top = (size_t*) (ptr + hi._size);

            for(;ptr < (char*) this_chunk_top;++ptr)
                printf("%c",*ptr);

            if(max_bytes)
            {
                output_cnt += hi._size;
                if(output_cnt >= max_bytes)
                    break;
            }
        }
        fflush(stdout);
    }

#else

    void dump_heap_raw(
                    size_t* start_chunk,
                    size_t* heap_top_end, //first invalid address
                    uint32 max_kb)
    {
        if(!start_chunk)
        {
            printf("ERROR - start_chunk address is missing\n");
            return;
        }
        if(!heap_top_end)
        {
            printf("ERROR - heap top address is missing\n");
            return;
        }
        if(start_chunk >= heap_top_end)
        {
            printf("ERROR - start chunk address is too big\n");
            return;
        }

        size_t max_bytes = max_kb * 1024; //KB ---> bytes
        size_t output_cnt = 0;

        if(max_bytes)
        {
            char* ptr = (char*) start_chunk;
            for(;ptr < (char*) heap_top_end;++ptr)
            {
                printf("%c",*ptr);
                if(++output_cnt >= max_bytes)
                    break;
            }
        }
        else
        {
            char* ptr = (char*) start_chunk;
            for(;ptr < (char*) heap_top_end;++ptr)
                printf("%c",*ptr);
        }
        fflush(stdout);
    }

#endif

//-----------------------------------------------------------------------------
// Get the arena pointer to a chunk_ptr:
//-----------------------------------------------------------------------------

#if !defined(_WIN32) && !defined(_WIN64)

    size_t* get_arena(size_t* chunk_ptr)
    {
        if(!main_arena_ptr__)
            return (size_t*) 0;

        if(((chunk_t*) chunk_ptr)->size & A__) //allocated arena
        {
            size_t* heap_start =
                     (size_t*) get_start_of_allocated_heap_segment(chunk_ptr);
            size_t* heap_end =
                     (size_t*) (((char*) heap_start) + HEAP_MAX_SIZE);

            gen_ar_t* ar_ptr = main_arena_ptr__;
            gen_ar_t* start_ar_ptr = ar_ptr;
            for(;ar_ptr;)
            {
                if((ar_ptr->addr[top_idx__] >= heap_start) &&
                   (ar_ptr->addr[top_idx__] < heap_end))
                {
                    return (size_t*) ar_ptr;
                }
                ar_ptr = (gen_ar_t*) ar_ptr->addr[next_idx__];
                if(ar_ptr == start_ar_ptr)
                    break; //linked list looped back to start
            }
            return (size_t*) 0;
        }

        return (size_t*) main_arena_ptr__;
    }

#endif

//-----------------------------------------------------------------------------
// Get the next arena or NULL:
//-----------------------------------------------------------------------------

#if !defined(_WIN32) && !defined(_WIN64)

    size_t* get_next_arena(size_t* ar_ptr)
    {
        if(!ar_ptr)
            return (size_t*) 0;

        gen_ar_t* p = (gen_ar_t*) ar_ptr;
        if(p->addr[next_idx__] == (size_t*) main_arena_ptr__)
            return (size_t*) 0;

        return p->addr[next_idx__];
    }

#endif

//-----------------------------------------------------------------------------
// Test whether a chunk is the top level chunk or not:
//-----------------------------------------------------------------------------

#if !defined(_WIN32) && !defined(_WIN64)

    bool is_top_chunk(size_t* chunk_ptr)
    {
        if(!chunk_ptr)
            return false;

        //If mmapped chunk:
        if(((chunk_t*) chunk_ptr)->size & M__)
            return true; //there is only one, so it is the top chunk

        //If chunk in allocated contiguous heap:
        size_t chunk_size =
                       (size_t) (((chunk_t*) chunk_ptr)->size & ~FLAGS_MASK);
        size_t* next = (size_t*) (((char*) chunk_ptr) + chunk_size);
        if(chunk_ptr > sbrk(0))
        {
            heap_bott_t* hb = get_start_of_allocated_heap_segment(chunk_ptr);
            size_t* heap_end = (size_t*) (((char*) hb) + hb->size);
            if(next == heap_end)
                return true;
            return false;
        }

        //If chunk in main contiguous heap:
        return (((char*) chunk_ptr) + chunk_size) == sbrk(0) ? true : false;
    }

#endif

//-----------------------------------------------------------------------------
// Find the heap top end of a contiguous heap:
//-----------------------------------------------------------------------------

#if !defined(_WIN32) && !defined(_WIN64)

    size_t* get_heap_top_end(size_t* chunk_ptr)
    {
        if(!chunk_ptr)
            return (size_t*) 0;

        //If this is an mmapped chunk:
        if(((chunk_t*) chunk_ptr)->size & M__)
            return (size_t*) 0;

        //If the chunk is in an allocated heap:
        if(chunk_ptr > sbrk(0))
        {
            heap_bott_t* hb = get_start_of_allocated_heap_segment(chunk_ptr);
            if(!hb)
                return (size_t*) 0;
            return (size_t*) (((char*) hb) + hb->size);
        }

        //If chunk is in the main contiguous heap:
        return (size_t*) sbrk(0);
    }

#endif

//-----------------------------------------------------------------------------
// Find the bottom chunk inside a contiguous heap:
//-----------------------------------------------------------------------------

#if !defined(_WIN32) && !defined(_WIN64)

    size_t* get_bottom_chunk(size_t* chunk_ptr)
    {
        if(!chunk_ptr)
            return (size_t*) 0;

        //If this is an mmapped chunk:
        if(((chunk_t*) chunk_ptr)->size & M__)
            return chunk_ptr; //there is only one, so it is the bottom chunk

        //If the chunk is in an allocated heap:
        size_t chunk_size =
                         (size_t) (((chunk_t*) chunk_ptr)->size & ~FLAGS_MASK);
        size_t* next = (size_t*) (((char*) chunk_ptr) + chunk_size);
        if(chunk_ptr > sbrk(0))
        {
            heap_bott_t* hb = get_start_of_allocated_heap_segment(chunk_ptr);
            if(!hb)
                return (size_t*) 0;
            size_t* heap_end = (size_t*) (((char*) hb) + hb->size);
            if(next > heap_end)
                return (size_t*) 0;

            //Step back to find the first chunk:
            size_t* last_valid_chunk_ptr = chunk_ptr;
            size_t* test = (size_t*) 0;
            size_t* p = chunk_ptr;
            for(p -= 2;p > (size_t*) hb;p -= 2)
            {
                test = get_next_chunk(p); //get next chunk by the 'size' field
                if(test == last_valid_chunk_ptr)
                {
                    last_valid_chunk_ptr = test;
                }
            }

            return last_valid_chunk_ptr;
        }

        //If chunk is in the main contiguous heap:
        return heap_bottom_chunk__;
    }

#endif

//-----------------------------------------------------------------------------
// Find all bottom chunks of an arena by the arena's top chunk:
//-----------------------------------------------------------------------------
// Call the function after putting a size_t array on the stack to store
// the found bottom pointer addresses:
//
//     size_t baddr_arr[MAX_NUM_HEAPS]; //bottom pointer addresses as size_t
//     memset(baddr_arr,0,sizeof(size_t) * MAX_NUM_HEAPS);
//     size_t* baddr_arr_ptr = (size_t*) &baddr_arr[0];
//     ...
//     size_t num_botts = get_all_bottom_chunks_of_arena(
//                                                  ar_ptr->addr[top_idx__],
//                                                  &baddr_arr_ptr,
//                                                   MAX_NUM_HEAPS);
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------

#if !defined(_WIN32) && !defined(_WIN64)

    size_t get_all_bottom_chunks_of_arena(
                                size_t* ar_top_chunk_ptr, //arena's top chunk
                                size_t** baddr_arr_ptr,
                                size_t max_num_botts)
    {
        size_t* pbaddr = *baddr_arr_ptr;
        memset(pbaddr,0,sizeof(size_t) * max_num_botts);
        if(!ar_top_chunk_ptr)
            return 0;

        if(ar_top_chunk_ptr < sbrk(0))
        {
            *pbaddr = (size_t) heap_bottom_chunk__;
            return 1;
        }

        size_t* last_valid_chunk_ptr = (size_t*) 0;
        size_t* test = (size_t*) 0;
        size_t* p = (size_t*) 0;

        size_t* chunk_ptr = ar_top_chunk_ptr;
        heap_bott_t* hb = get_start_of_allocated_heap_segment(chunk_ptr);
        if(!hb)
            return 0;
        size_t* heap_end = (size_t*) (((char*) hb) + hb->size);
        pbaddr += max_num_botts - 1; //move to last entry
        size_t i = 0;
        for(;i < max_num_botts;++i)
        {
            //Get the pointer to the next chunk:
            size_t chunk_size =
                         (size_t) (((chunk_t*) chunk_ptr)->size & ~FLAGS_MASK);
            size_t* next = (size_t*) (((char*) chunk_ptr) + chunk_size);

            //Check if the next chunk is in the allowed range:
            if(next > heap_end) //next is maximum at heap_end (if top chunk)
            {
                if(!i) //if first loop
                    return 0;
                break;
            }

            //Step back to find the first chunk in this heap:
            last_valid_chunk_ptr = chunk_ptr;
            test = (size_t*) 0;
            p = chunk_ptr;
            for(p -= 2;p > (size_t*) hb;p -= 2)
            {
                test = get_next_chunk(p); //get next chunk by the 'size' field
                if(test == last_valid_chunk_ptr)
                {
                    last_valid_chunk_ptr = test;
                }
            }

            //Store the found bottom pointer (reverse order):
            *pbaddr = (size_t) last_valid_chunk_ptr;
            --pbaddr;

            //Get the previous heap:
            hb = hb->prev;
            if(!hb)
                break;

            //Get the heap end:
            heap_end = (size_t*) (((char*) hb) + hb->size);

            //Find the top chunk of the new heap:
            test = (size_t*) 0;
            p = heap_end;
            for(p -= 2;p > (size_t*) hb;p -= 2)
            {
                test = get_next_chunk(p); //get next chunk by the 'size' field
                if(test == heap_end)
                {
                    chunk_ptr = test;
                    continue; //go ahead
                }
            }
            break;
        }

        //Get the number of bottoms found:
        size_t num_botts = max_num_botts; //unlikely
        if(i != max_num_botts) //---> after broken loop (break)
            num_botts = ++i;

        ///////////////////////////////////////////////////////////////////////
        // Shift the found bottoms forward to index 0:
        //---------------------------------------------------------------------
        // Example:
        //
        // We have max. 10 bottoms and 3 bottoms found:
        //
        //    baddr_arr[0] = 0
        //    baddr_arr[1] = 0
        //    baddr_arr[2] = 0
        //    baddr_arr[3] = 0
        //    baddr_arr[4] = 0
        //    baddr_arr[5] = 0
        //    baddr_arr[6] = 0
        //    baddr_arr[7] = bott_0 <--- idx = 7 = max_num_botts - num_botts
        //    baddr_arr[8] = bott_1
        //    baddr_arr[9] = bott_3 <--- idx = 9 = max_num_botts - 1
        //
        // So we need to shift by 7 = max_num_botts - num_botts:
        //
        //    baddr_arr[0] = baddr_arr[7] = bott_0
        //    baddr_arr[1] = baddr_arr[8] = bott_1
        //    baddr_arr[2] = baddr_arr[9] = bott_2
        //    baddr_arr[3] = 0
        //    baddr_arr[4] = 0
        //    baddr_arr[5] = 0
        //    baddr_arr[6] = 0
        //    baddr_arr[7] = 0
        //    baddr_arr[8] = 0
        //    baddr_arr[9] = 0
        //
        // That means:
        //
        //  for(i = 0;i < num_botts;++i)
        //      baddr_arr[i] = baddr_arr[max_num_botts - num_botts + i]
        //  for(i = num_botts;i < max_num_botts;++i)
        //      baddr_arr[i] = 0
        //---------------------------------------------------------------------

        pbaddr = *baddr_arr_ptr; //move to index 0
        if(num_botts < max_num_botts)
        {
            for(i = 0;i < num_botts;++i)
                *(pbaddr + i) = *(pbaddr + max_num_botts - num_botts + i);
            for(i = num_botts;i < max_num_botts;++i)
                *(pbaddr + i) = 0;
        }

        return num_botts;
    }

#endif

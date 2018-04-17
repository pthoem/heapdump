#include "settings.h"
#include "heapdump.h"

static const char APP_NAME[] = "heapdump";
static const char APP_VER_STR[5 + 1] = "1.8.4";
#define APP_COPYRIGHT "(c) 2018 Peter Thoemmes, D-54441 Ocken/Germany"

static unsigned char g_verbose = 0; //0
static double g_alloc_size_mb = 0.0; //0.0
#define NUM_MEM_PTRS 10 //10
static void* g_mem_ptr[NUM_MEM_PTRS];

void usage()
{
    printf(
      "USAGE:\n"
      "\n"
      "INTERACTIVE MODE:\n"
      "   %s [-v] [-alloc_mb <size/MB>]\n"
      "\n"
      "DUMP THE HEAP FOOTPRINT:\n"
      "   %s [-v] [-alloc_mb <size/MB>] -footprint\n"
      "\n"
      "DEBUG DUMP OF THE HEAP:\n"
      "   %s [-v] [-alloc_mb <size/MB>] -debug\n"
      "\n"
      "HEX DUMP OR RAW DATA OUTPUT OF THE HEAP:\n"
      "   %s [-v] [-alloc_mb <size/MB>] [-hex|-raw] [-max_kb <size/KB>]\n"
      "\n"
      "Parameters:\n"
      "\n"
      "   -?                   Print this screen\n"
      "   -v                   Verbose output\n"
      "   -alloc_mb <size/MB>  Allocate <size> MB using malloc()\n"
      "                        REMARK: <size> as integer *or* floating point\n"
      "   -max_kb <size/KB>    Limit the output to <size> KB\n"
      "                        REMARK: <size> as integer\n"
      "\n"
      "--- VERSION:\n"
      "%s %s\n"
      "%s\n"
      "---------------------------\n",
      APP_NAME,
      APP_NAME,
      APP_NAME,
      APP_NAME,
      APP_NAME,
      APP_VER_STR,
      APP_COPYRIGHT);
}

int main(int argc,char* argv[])
{
    static int i = 1;
    for(;i < argc;++i)
    {
        if(!strcmp(argv[i],"-v"))
            g_verbose = 1;
        else if(!strcmp(argv[i],"-vv"))
            g_verbose = 2;
        else if(!strcmp(argv[i],"-vvv"))
            g_verbose = 3;
    }
    for(i = 0;i < NUM_MEM_PTRS;++i)
    {
        free(g_mem_ptr[i]);
        g_mem_ptr[i] = (void*) 0;
    }
    if(g_verbose)
        printf("\n");
    init_heapdump(g_verbose);

    size_t* MAX_ADDR = (size_t*) 0;
    size_t* MAX_USER_ADDR = (size_t*) 0;
    size_t* STACK_TOP = (size_t*) 0;
    size_t* HEAP_BOTTOM_CHUNK = (size_t*) 0;
    get_memory_dimenions(
                    &MAX_ADDR,
                    &MAX_USER_ADDR,
                    &STACK_TOP,
                    &HEAP_BOTTOM_CHUNK);

    #if !defined(_WIN32) && !defined(_WIN64)
        static const char SB = 1; //1st static const initialized
        size_t* static_bottom = (size_t*) &SB;
        size_t static_size = (size_t) (HEAP_BOTTOM_CHUNK - static_bottom);
    #endif

    static const unsigned char MODE_INTERACTIVE = 0;
    static const unsigned char MODE_FOOTPRINT   = 1;
    static const unsigned char MODE_DEBUGDUMP   = 2;
    static const unsigned char MODE_HEXDUMP     = 3;
    static const unsigned char MODE_RAW         = 4;
    unsigned char mode = MODE_INTERACTIVE;

    uint32 max_kb = 0;

    bool show_usage = false;
    static const unsigned char FLAG_ALLOC_MB = 0x01;
    static const unsigned char FLAG_MAX_KB   = 0x02;
    unsigned char flag = 0x00;
    for(i = 1;i < argc;++i)
    {
        if(!flag)
        {
            if(!strcmp(argv[i],"-?"))
            {
                show_usage = true;
                break;
            }
            else if(!strcmp(argv[i],"-v"))
            {
                g_verbose = 1;
            }
            else if(!strcmp(argv[i],"-vv"))
            {
                g_verbose = 2;
            }
            else if(!strcmp(argv[i],"-vvvv"))
            {
                g_verbose = 3;
            }
            else if(!strcmp(argv[i],"-alloc_mb"))
            {
                flag = FLAG_ALLOC_MB;
            }
            else if(!strcmp(argv[i],"-footprint"))
            {
                mode = MODE_FOOTPRINT;
            }
            else if(!strcmp(argv[i],"-debug"))
            {
                mode = MODE_DEBUGDUMP;
            }
            else if(!strcmp(argv[i],"-hex"))
            {
                mode = MODE_HEXDUMP;
            }
            else if(!strcmp(argv[i],"-raw"))
            {
                mode = MODE_RAW;
            }
            else if(!strcmp(argv[i],"-max_kb"))
            {
                flag = FLAG_MAX_KB;
            }
            else
            {
                show_usage = true;
                break;
            }
        }
        else
        {
            if(flag == FLAG_ALLOC_MB) //-alloc_mb <size/MB>
            {
                char* p_wrong_char = NULL;
                double val = strtod(argv[i],&p_wrong_char);
                if(*p_wrong_char == 0x00)
                    g_alloc_size_mb = val;
            }
            else if(flag == FLAG_MAX_KB) //-max_kb <size/KB>
            {
                char* p_wrong_char = NULL;
                uint32 val = strtoul(argv[i],&p_wrong_char,10);
                if(*p_wrong_char == 0x00)
                {
                    max_kb = val;
                }
                else
                {
                    show_usage = true;
                    break;
                }
            }
            else
            {
                show_usage = true;
                break;
            }
            flag = 0x00;
        }
    }

    if(max_kb && ((mode != MODE_HEXDUMP) && (mode != MODE_RAW)))
        show_usage = true;

    if(show_usage)
    {
        usage();
        return 0;
    }

    if(g_alloc_size_mb)
    {
        size_t byte_size = (size_t) (g_alloc_size_mb * 1024.0 * 1024.0);
        size_t mem_block_size = byte_size/NUM_MEM_PTRS;
        if(mem_block_size)
        {
            if(g_verbose)
            {
                printf(
                    "ALLOCATING %8.6lf MB in MAIN ARENA (%u chunks)...",
                    g_alloc_size_mb,
                    NUM_MEM_PTRS);
                fflush(stdout);
            }
            uint32 i = 0;
            for(;i < NUM_MEM_PTRS;++i)
                g_mem_ptr[i] = (size_t*) malloc(mem_block_size);
            if(g_verbose)
            {
                printf("done.\n");
                printf("\n");
            }
        }
    }

    size_t* heap_top_end = (size_t*) 0; //first invalid address
    size_t* heap_top_chunk =  (size_t*) 0; //heap top chunk
    size_t heap_size =
            get_current_contiguous_heap_limit(&heap_top_end,&heap_top_chunk);

    if(mode != MODE_INTERACTIVE)
    {
        if(mode == MODE_FOOTPRINT)
        {
            if(g_verbose)
                printf("Dumping the HEAP footprint...\n");
            printf("\n");
            dump_heap_footprint();
        }
        else if(mode == MODE_DEBUGDUMP)
        {
            if(g_verbose)
            {
                printf("DEBUG dump of the HEAP...\n");
            }
            printf("\n");
            dump_heap_details(HEAP_BOTTOM_CHUNK,heap_top_end);
        }
        else if(mode == MODE_HEXDUMP)
        {
            if(g_verbose)
            {
                if(max_kb)
                    printf("HEX dump of the HEAP (max: %u KB)...\n",max_kb);
                else
                    printf("HEX dump of the HEAP...\n");
            }
            printf("\n");
            dump_heap_hex(HEAP_BOTTOM_CHUNK,heap_top_end,max_kb);
        }
        else if(mode == MODE_RAW)
        {
            if(g_verbose)
            {
                if(max_kb)
                    printf("RAW dump of the HEAP (max: %u KB)...\n",max_kb);
                else
                    printf("RAW dump of the HEAP...\n");
            }
            printf("\n");
            dump_heap_raw(HEAP_BOTTOM_CHUNK,heap_top_end,max_kb);
        }
        if(g_alloc_size_mb)
        {
            for(i = 0;i < NUM_MEM_PTRS;++i)
            {
                free(g_mem_ptr[i]);
                g_mem_ptr[i] = (void*) 0;
            }
        }
        return 0;
    }

    if(g_verbose)
    {
        printf("Please press ENTER to show the HEAP layout...");
        getchar();
    }
    printf("\n");

    printf(
        "%16p +--------------------------+ ADDRESS SPACE TOP (%s)\n"
        "                 |                          |\n"
        "                 |                          |\n"
        "                 |      MAPPED KERNEL       |\n"
        "                 |                          |\n"
        "                 |                          |\n"
        "%16p +--------------------------+ USER SPACE TOP (%s)\n"
        "                 |        Guard Page        |\n"
        "%16p +--------------------------+ STACK TOP\n"
        "                 |          STACK           |\n"
        "                 +---||------||-------||----+\n"
        "                 |   \\/      \\/       \\/    |\n"
        "                 |        Free Space        |\n"
        "                 |   /\\      /\\       /\\    |\n"
        "%16p +---||------||-------||----+ MAIN HEAP TOP\n"
        "                 |                          |\n"
        "                 |        MAIN HEAP         |\n"
        "                 |   %10lu %s          |\n"
        "                 |                          |\n"
        "%16p +--------------------------+ MAIN HEAP BOTTOM\n"
    #if !defined(_WIN32) && !defined(_WIN64)
        "                 |                          |\n"
        "                 |      STATIC MEMORY       |\n"
        "                 |   %10lu %s          |\n"
        "                 |                          |\n"
        "%16p +--------------------------+ STATIC DATA BOTTOM\n"
    #else
        "                 |                          |\n"
        "                 |      STATIC MEMORY       |\n"
        "                 |                          |\n"
        "                 |                          |\n"
        "                 +--------------------------+ STATIC DATA BOTTOM\n"
    #endif
        "                 |                          |\n"
        "                 |          CODE            |\n"
        "                 |         (TEXT)           |\n"
        "                 |                          |\n"
        "                 +--------------------------+ CODE BOTTOM\n"
        "                 |                          |\n"
        "             0x0 +--------------------------+ ADDRESS SPACE BOTTOM\n"
        "\n",
        MAX_ADDR,
        STACK_TOP == STACK_TOP64 ? "256 TB" : "4 GB",
        MAX_USER_ADDR,
        STACK_TOP == STACK_TOP64 ? "128 TB" :
                                STACK_TOP == STACK_TOP32_3GB ? "3 GB" : "2 GB",
        STACK_TOP,
        heap_top_end,
        HUMAN_READABLE_MEM_SIZE__(heap_size),
        HUMAN_READABLE_MEM_UNIT_2__(heap_size),
        #if !defined(_WIN32) && !defined(_WIN64)
            HEAP_BOTTOM_CHUNK,
            HUMAN_READABLE_MEM_SIZE__(static_size),
            HUMAN_READABLE_MEM_UNIT_2__(static_size),
            static_bottom);
        #else
            HEAP_BOTTOM_CHUNK);
        #endif

    printf("Please press ENTER to show the HEAP footprint...");
    getchar();
    printf("\n");
    dump_heap_footprint();

    printf("Please press ENTER to see a DEBUG DUMP of the HEAP...\n");
    getchar();
    dump_heap_details(HEAP_BOTTOM_CHUNK,heap_top_end);

    printf("Please press ENTER to see a HEX DUMP of the HEAP...\n");
    getchar();
    dump_heap_hex(HEAP_BOTTOM_CHUNK,heap_top_end);

    printf("Please press ENTER to see the RAW HEAP data...\n");
    getchar();
    dump_heap_raw(HEAP_BOTTOM_CHUNK,heap_top_end);
    printf("\n");
    printf("\n");

    #ifdef _MSC_VER //MSVC++
        printf("Please press ENTER to quit...");
        getchar();
        printf("\n");
    #endif

    if(g_alloc_size_mb)
    {
        for(i = 0;i < NUM_MEM_PTRS;++i)
        {
            free(g_mem_ptr[i]);
            g_mem_ptr[i] = (void*) 0;
        }
    }
    return 0;
}

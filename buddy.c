/**
 * Buddy Allocator
 *
 * For the list library usage, see http://www.mcs.anl.gov/~kazutomo/list/
 */

/**************************************************************************
 * Conditional Compilation Options
 **************************************************************************/
#define USE_DEBUG 0

/**************************************************************************
 * Included Files
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "buddy.h"
#include "list.h"

/**************************************************************************
 * Public Definitions
 **************************************************************************/
#define MIN_ORDER 12
#define MAX_ORDER 20

#define MEMORY_AREA (1 << MAX_ORDER)
#define PAGE_SIZE (1<<MIN_ORDER)

#define PAGE_NUM (MEMORY_AREA/PAGE_SIZE)

/* page index to address */
#define PAGE_TO_ADDR(page_idx) (void *)((page_idx*PAGE_SIZE) + g_memory)

/* address to page index */
#define ADDR_TO_PAGE(addr) ((unsigned long)((void *)addr - (void *)g_memory) / PAGE_SIZE)

/* find buddy address */
#define BUDDY_ADDR(addr, o) (void *)((((unsigned long)addr - (unsigned long)g_memory) ^ (1<<o)) \
									 + (unsigned long)g_memory)

#if USE_DEBUG == 1
#  define PDEBUG(fmt, ...) \
	fprintf(stderr, "%s(), %s:%d: " fmt,			\
		__func__, __FILE__, __LINE__, ##__VA_ARGS__)
#  define IFDEBUG(x) x
#else
#  define PDEBUG(fmt, ...)
#  define IFDEBUG(x)
#endif

/**************************************************************************
 * Public Types
 **************************************************************************/
typedef struct {
	struct list_head list;
	/* TODO: DECLARE NECESSARY MEMBER VARIABLES */
    
    int block_size;
    int page_index;
    char *block_address;
} page_t;

/**************************************************************************
 * Global Variables
 **************************************************************************/
/* free lists*/
struct list_head free_area[MAX_ORDER+1];

/* memory area */
char g_memory[1<<MAX_ORDER];

/* page structures */
page_t g_pages[(1<<MAX_ORDER)/PAGE_SIZE];

/**************************************************************************
 * Public Function Prototypes
 **************************************************************************/

/**************************************************************************
 * Local Functions
 **************************************************************************/

/**
 * Initialize the buddy system
 */
void buddy_init()
{
	int i;
	int n_pages = (1<<MAX_ORDER) / PAGE_SIZE;
	for (i = 0; i < n_pages; i++) {
		/* TODO: INITIALIZE PAGE STRUCTURES */
        INIT_LIST_HEAD(&g_pages[i].list);
        
        g_pages[i].block_size = -1;
        g_pages[i].page_index = i;
        g_pages[i].block_address = PAGE_TO_ADDR(i);
	}

	/* initialize freelist */
	for (i = MIN_ORDER; i <= MAX_ORDER; i++) {
		INIT_LIST_HEAD(&free_area[i]);
	}

	/* add the entire memory as a freeblock */
	list_add(&g_pages[0].list, &free_area[MAX_ORDER]);
}

/**
 * Ceiling function that will find the exponent needed to calculate the size of
 * of smallest block needed for a memory allocation.
 *
 */

int order_exp(int size)
{
    int order_num = MIN_ORDER;
    
    while((1 << order_num) < size && order_num <= MAX_ORDER)
    {
        order_num++;
    }
    
    return order_num;
}

void split(int order, int index)
{
    //Get our page structure from list

    page_t *cur_page = &g_pages[ADDR_TO_PAGE(BUDDY_ADDR(PAGE_TO_ADDR(index), (order - 1)))];
    
    //Add page to proper list
    
    list_add(&(cur_page->list), &free_area[order-1]);
    
    //Recursive call to keep splitting if necessary
    
    split(order, index);
    
}
/**
 * Allocate a memory block.
 *
 * On a memory request, the allocator returns the head of a free-list of the
 * matching size (i.e., smallest block that satisfies the request). If the
 * free-list of the matching block size is empty, then a larger block size will
 * be selected. The selected (large) block is then splitted into two smaller
 * blocks. Among the two blocks, left block will be used for allocation or be
 * further splitted while the right block will be added to the appropriate
 * free-list.
 *
 * @param size size in bytes
 * @return memory block address
 */
void *buddy_alloc(int size)
{
    printStats();
	/* TODO: IMPLEMENT THIS FUNCTION */
    if(size <= 0 || size > (MEMORY_AREA))
    {
        return NULL;
    }
    
    //Gets the correct order based on the size of the request
    int order = order_exp(size);
    
    page_t *entry = NULL;
    
    //Iterate through the free lists to find a block
    for(int i = order; i < MAX_ORDER; i++)
    {
        //If we have found a list with a free block of the proper size, return the entry.
        if(!list_empty(&free_area[i]) && i == order)
        {
            //We found a block of proper size, so we can return it.
            entry = list_entry(free_area[i].next, page_t, list);
            list_del(free_area[i].next);
            return(entry);
        }
        else if(!list_empty(&free_area[i]) && i > order)
        {
            //We found the next best solution - the next free block of a larger size
            entry = list_entry(free_area[i].next, page_t, list);
            list_del(free_area[i].next);
            break;
        }
     
    }
    
    //Get the index for our recursive function
    
    int cur_index = entry->page_index;
    
    list_del_init(&(entry->list));
    
    entry->block_size = order;
    
    //Call the recursive function to split even further
    
    split(order, cur_index);
    
    return (PAGE_TO_ADDR(cur_index));
}

/**
 * Free an allocated memory block.
 *
 * Whenever a block is freed, the allocator checks its buddy. If the buddy is
 * free as well, then the two buddies are combined to form a bigger block. This
 * process continues until one of the buddies is not free.
 *
 * @param addr memory block address to be freed
 */
void buddy_free(void *addr)
{
	/* TODO: IMPLEMENT THIS FUNCTION */
    
    int free_index = ADDR_TO_PAGE(addr);
    page_t *free_page = &g_pages[free_index];
    int free_order = free_page->block_size;
    page_t *buddy_block = NULL;
    struct list_head *temp_list;
    int found = 0;
    
    for(int i = free_order; i <= MAX_ORDER; i++)
    {
        if(i == MAX_ORDER)
        {
            free_page->block_size = MAX_ORDER;
            list_add(&free_page->list, &free_area[i]);
        }
        
        buddy_block = &g_pages[ADDR_TO_PAGE(BUDDY_ADDR(PAGE_TO_ADDR(free_page->page_index), i))];
        list_for_each(temp_list, &free_area[i])
        {
            if(buddy_block == list_entry(temp_list, page_t, list))
            {
                found = 1;
            }
        }
        
        if(found == 0)
        {
            break;
        }
        else
        {
            list_del_init(&buddy_block->list);
            if(buddy_block->page_index < free_page->page_index)
            {
                free_page = buddy_block;
            }
        }
        free_order = i;
    }
    
    free_page -> block_size = free_order;
    list_add(&free_page->list, &free_area[free_order]);
    
}

/**
 * Print the buddy system status---order oriented
 *
 * print free pages in each order.
 */
void buddy_dump()
{
	int o;
	for (o = MIN_ORDER; o <= MAX_ORDER; o++) {
		struct list_head *pos;
		int cnt = 0;
		list_for_each(pos, &free_area[o]) {
			cnt++;
		}
		printf("%d:%dK ", cnt, (1<<o)/1024);
	}
	printf("\n");
}

void printStats()
{
    printf("MIN ORDER: %d\n", MIN_ORDER);
    printf("MAX ORDER: %d\n", MAX_ORDER);
    printf("PAGE SIZE: %d\n", PAGE_SIZE);
    printf("MEMORY AREA: %d\n", MEMORY_AREA);
    printf("PAGE NUM: %d\n", PAGE_NUM);
    
}

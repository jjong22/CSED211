/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// Definition of Macros
#define FREE 0
#define ALLOCATED 1

#define WORDSIZE 4
#define DWORDSIZE 8
#define PAGESIZE (1 << 12)

#define GET(ptr) (*(unsigned int *)(ptr))
#define PUT(ptr, val) (*(unsigned int *)(ptr) = (val))   

#define GET_SIZE(ptr) (GET(ptr) & ~0x7) 
#define GET_IS_ALLOCATED(ptr) (GET(ptr) & 0x1) // least significant bit   

#define HEADER_PTR(block_ptr) ((char *)(block_ptr) - WORDSIZE)  // get header pointer
#define FOOTER_PTR(block_ptr) ((char *)(block_ptr) + GET_SIZE(HEADER_PTR(block_ptr)) - DWORDSIZE) // get footer pointer

#define NEXT_BLOCK_PTR(block_ptr) ((char *)(block_ptr) + GET_SIZE(((char *)(block_ptr) - WORDSIZE))) // get next block pointer
#define PREV_BLOCK_PTR(block_ptr) ((char *)(block_ptr) - GET_SIZE(((char *)(block_ptr) - DWORDSIZE)))  // get previous block pointer

#define PREV_PTR(ptr) ((void*)(ptr) + WORDSIZE)
#define NEXT_PTR(ptr) ((void*)(ptr))

// global variable
static void* heap_start;
static void* free_start;
static int largest_free_size;

// allocation functions
static void* extend_heap(size_t number_of_words); // extend heap by number_of_words
static void* coalesce(void* block_ptr); // coalesce block with adjacent free blocks
static void insert_free_block(void* block_ptr); // insert block into free list
static void delete_free_block(void* block_ptr); // delete block from free list
static void* first_fit(size_t size); // find first fit block in free list 
static void allocate(void* block_ptr, size_t size); // allocate block with size

static void* extend_heap(size_t number_of_words)
{
    void* block_ptr;
    size_t size;

    if (number_of_words % 2 == 1) // Align size to even number
        number_of_words += 1;
        
    size = number_of_words * WORDSIZE;
    block_ptr = mem_sbrk(size); // extend heap

    if ((long) block_ptr == -1) // Failed to allocate space
        return NULL;

    PUT(NEXT_PTR(block_ptr), NULL); 
    PUT(PREV_PTR(block_ptr), NULL); 
    PUT(HEADER_PTR(block_ptr), size | FREE); 
    PUT(FOOTER_PTR(block_ptr), size | FREE);
    PUT(HEADER_PTR(NEXT_BLOCK_PTR(block_ptr)), 0 | ALLOCATED);

    return coalesce(block_ptr);
}
/**
* Extend heap by number_of_words. When we cannot find a free block that fits size, we extend heap by number_of_words.
*
* @param number_of_words it indicate the size to be extended..
* @return coalesce(block_ptr) The pointer of the coalesced block.
* 
* @note If the number_of_words is odd, we align it to even number. 
*       We extend heap by number_of_words and return the pointer of the coalesced block.
*       If we cannot allocate space, we return NULL.
*       Make sure new block is free, so we can use it.
*/

static void* coalesce(void* block_ptr)
{
    size_t size = GET_SIZE(HEADER_PTR(block_ptr)); // Size of current block
    size_t prev_size = GET_SIZE(FOOTER_PTR(PREV_BLOCK_PTR(block_ptr))); // Size of previous block
    size_t next_size = GET_SIZE(HEADER_PTR(NEXT_BLOCK_PTR(block_ptr))); // Size of next block
    void* prev_block_ptr = PREV_BLOCK_PTR(block_ptr); // Pointer of previous block
    void* next_block_ptr = NEXT_BLOCK_PTR(block_ptr); // Pointer of next block
    size_t is_prev_allocated = GET_IS_ALLOCATED(FOOTER_PTR(prev_block_ptr)); // Locate footer of prev block and extract allocation bit
    size_t is_next_allocated = GET_IS_ALLOCATED(HEADER_PTR(next_block_ptr)); // Locate header of next block and extract allocation bit
    
    // Case 1: both previous and next blocks are allocated -> nothing to do
    if (is_prev_allocated && is_next_allocated) {
        insert_free_block(block_ptr); // Insert coalesced block
        
        return block_ptr;
    }

    // Case 2: previous block is allocated, next block is free -> concatenate with next block
    else if (is_prev_allocated && !is_next_allocated) {
        delete_free_block(next_block_ptr);
        size += next_size; // Update size
        
        PUT(HEADER_PTR(block_ptr), size | FREE);
        PUT(FOOTER_PTR(next_block_ptr), size | FREE);
        
        insert_free_block(block_ptr); // Insert coalesced block
        
        return block_ptr;
    }

    // Case 3: previous block is free, next block is allocated -> concatenate with previous block
    else if (!is_prev_allocated && is_next_allocated) {
        delete_free_block(prev_block_ptr);
        size += prev_size; // Update size

        PUT(HEADER_PTR(prev_block_ptr), size | FREE); 
        PUT(FOOTER_PTR(block_ptr), size | FREE);
        
        insert_free_block(prev_block_ptr); // Insert coalesced block
        
        return prev_block_ptr;
    }

    // Case 4: both previous and next blocks are free -> concatenate with both blocks
    else if (!is_prev_allocated && !is_next_allocated) {
        delete_free_block(prev_block_ptr);
        size += prev_size; // Update size
        
        delete_free_block(next_block_ptr);
        size += next_size; // Update size

        PUT(HEADER_PTR(prev_block_ptr), size | FREE); 
        PUT(FOOTER_PTR(next_block_ptr), size | FREE);

        insert_free_block(prev_block_ptr); // Insert coalesced block

        return prev_block_ptr;
    }

    return NULL;
}
/**
* Coalesce the free block with adjacent free blocks.
*
* @param block_ptr The pointer of the block to be coalesced.
*
* @return block_ptr The pointer of the block after coalescing.
* @return prev_block_ptr The pointer of the previous block after coalescing.
* @return NULL If coalescing is not needed, return NULL.
* 
* @note Case 1: both previous and next blocks are allocated -> nothing to do
*       ex) [allocated] -> [block_ptr] -> [allocated]
*       Case 2: previous block is allocated, next block is free -> concatenate with next block
*       ex) [allocated] -> [block_ptr] -> [free]
*           [allocated] -> [     block_ptr     ]
*       Case 3: previous block is free, next block is allocated -> concatenate with previous block
*       ex) [free] -> [block_ptr] -> [allocated]
*           [  prev_block_ptr  ] -> [allocated]
*       Case 4: both previous and next blocks are free -> concatenate with both blocks
*       ex) [free] -> [block_ptr] -> [free]
*           [        prev_block_ptr       ]
*/

static void insert_free_block(void* block_ptr)
{
    void* current_block_ptr = GET(free_start); // Start from the first block
    void* previous_block_ptr = NULL; // Previous block pointer
    size_t block_size = GET_SIZE(HEADER_PTR(block_ptr)); // Size of the block to insert

    // for debugging
    // printf("Inserting block %d\n", block_ptr);

    while (current_block_ptr != NULL && block_size > GET_SIZE(HEADER_PTR(current_block_ptr))) {
        previous_block_ptr = current_block_ptr; 
        current_block_ptr = GET(NEXT_PTR(current_block_ptr)); 
    }

    if (block_size > largest_free_size)
        largest_free_size = block_size;


    if (previous_block_ptr != NULL) { // not beginning of the list
        PUT(NEXT_PTR(previous_block_ptr), block_ptr);
        PUT(PREV_PTR(block_ptr), previous_block_ptr);
    }
    else { // Insert at the beginning of the free list
        PUT(free_start, block_ptr); 
        PUT(PREV_PTR(block_ptr), NULL); 
    }

    if (current_block_ptr != NULL) { // not end of the list
        PUT(PREV_PTR(current_block_ptr), block_ptr);
    }
    PUT(NEXT_PTR(block_ptr), current_block_ptr);

    return;
}
/**
* insert block into sorted free list.
*
* @param block_ptr The pointer of the block to be inserted in to free list.
* @return void
* 
* @note Traverse the free list to find the correct position.
*       Update largest free size.
*       Insert the block in the correct position.
*       Update the pointers of the previous and next blocks.
*/


static void delete_free_block(void* block_ptr) // delete block from free list
{
    void* prev_block_ptr = GET(PREV_PTR(block_ptr)); // previous block pointer
    void* next_block_ptr = GET(NEXT_PTR(block_ptr)); // next block pointer

    if (prev_block_ptr != NULL && next_block_ptr != NULL) {
        PUT(PREV_PTR(next_block_ptr), prev_block_ptr);
        PUT(NEXT_PTR(prev_block_ptr), next_block_ptr);
    }
    else if (prev_block_ptr != NULL && next_block_ptr == NULL) {
        PUT(NEXT_PTR(prev_block_ptr), next_block_ptr);
    }
    else if (prev_block_ptr == NULL && next_block_ptr != NULL) {
        PUT(PREV_PTR(next_block_ptr), NULL);
        PUT(free_start, next_block_ptr);
    }
    else if (prev_block_ptr == NULL && next_block_ptr == NULL) {
        PUT(free_start, NULL);
    }

    PUT(NEXT_PTR(block_ptr), NULL);
    PUT(PREV_PTR(block_ptr), NULL);

    return;
}
/**
* delete block from free list.
*
* @param block_ptr The pointer of the block to be deleted from free list.
* @return void
* 
* @note Update the pointers of the previous and next blocks.
*
*       If the block is the first block, update the free_start. (prev_block_ptr == NULL, next_block_ptr != NULL)
*       If the block is the last block, update the next block's prev pointer. (prev_block_ptr != NULL, next_block_ptr == NULL)
*       If the block is the only block, update the free_start. (prev_block_ptr == NULL, next_block_ptr == NULL)
*       If the block is in the middle, update the pointers of the previous and next blocks. (prev_block_ptr != NULL, next_block_ptr != NULL)
*/

static void* first_fit(size_t size)
{
    void* block_ptr = GET(free_start);

    if (largest_free_size < size) // No free block fits size
        return NULL;

    // Start from first free block, end if free block is NULL, current block is next block
    for(block_ptr = GET(free_start); block_ptr != NULL; block_ptr = GET(NEXT_PTR(block_ptr))){ 
        if(size > GET_SIZE(HEADER_PTR(block_ptr))){ // Current block does not fit size
            continue; // Pass
        }

        return block_ptr; // Current block fits size
    }

    return NULL; // No fitting free block found
}
/**
* find first fit block in free list.
*
* @param size The size of the block to be allocated.
* 
* @return block_ptr The pointer of the first fit block in free list.
* @return NULL If no block fits size, return NULL.
* 
* @note Traverse the free list to find the first fit block.
*       If the block fits size, return the pointer of the block.
*       If no block fits size, return NULL.
*
*       We have global variable largest_free_size, so we can check if there is a free block that fits size.
*       free list is sorted by size, so we can find the first fit block. (almost best fit)
*/

static void allocate(void* block_ptr, size_t size)
{
    size_t block_size = GET_SIZE(HEADER_PTR(block_ptr)); // size of current block
    size_t rest_size = block_size - size; // Size of rest space
    void* rest_block_ptr; // Pointer of rest block

    delete_free_block(block_ptr); 

    if (rest_size > 2 * WORDSIZE) { // split block
        PUT(HEADER_PTR(block_ptr), size | ALLOCATED); 
        PUT(FOOTER_PTR(block_ptr), size | ALLOCATED);

        rest_block_ptr = NEXT_BLOCK_PTR(block_ptr); 
        PUT(NEXT_PTR(rest_block_ptr), NULL);
        PUT(PREV_PTR(rest_block_ptr), NULL);
        PUT(HEADER_PTR(rest_block_ptr), rest_size | FREE); 
        PUT(FOOTER_PTR(rest_block_ptr), rest_size | FREE);
        
        coalesce(rest_block_ptr);
    }

    else { // do not split block
        PUT(HEADER_PTR(block_ptr), block_size | ALLOCATED); 
        PUT(FOOTER_PTR(block_ptr), block_size | ALLOCATED); 
    }

    return ;
}
/**
* allocate block with size
*
* @param block_ptr The pointer of the block to be allocated.
* @param size The size of the block to be allocated.
*
* @return void
* 
* @note Delete the block from free list.
*       If the rest size is larger than 2 * WORDSIZE, split the block.
*       If the rest size is smaller than 2 * WORDSIZE, do not split the block.
*       Update the header and footer of the block.
*       Coalesce the rest block.
*/

int mm_init(void)
{
    heap_start = mem_sbrk(6 * WORDSIZE);

    if (heap_start == (void *)-1) // error
        return -1; 

    PUT(heap_start, 0); // padding for allignment
    PUT(heap_start + (1 * WORDSIZE), NULL); // NEXT pointer for free list
    PUT(heap_start + (2 * WORDSIZE), NULL); // PREV pointer for free list
    PUT(heap_start + (3 * WORDSIZE), 2 * WORDSIZE | ALLOCATED); // Prologue header
    PUT(heap_start + (4 * WORDSIZE), 2 * WORDSIZE | ALLOCATED); // Prologue footer
    PUT(heap_start + (5 * WORDSIZE), 0 * WORDSIZE | ALLOCATED); // Epilogue header

    free_start = heap_start + (2 * WORDSIZE); // free list
    heap_start += 4 * WORDSIZE; // root of heap
    largest_free_size = 0;

    return 0;
}
/**
* initialize the malloc package.
*
* @param void
*
* @return 0 if initialization is successful.
* @return -1 if initialization is failed.
* 
* @note Initialize the heap by extending heap by 6 * WORDSIZE.
*       Add padding for alignment.
*       Initialize the free list.
*       Initialize the prologue and epilogue blocks.
*       Initialize the global variables.
*/

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    void* block_ptr;
    size_t block_size;
    size_t extension_size;
    int init_success;

    if (size == 0)
        return NULL;

    if (heap_start == NULL) { // initialize heap when not initialized
        init_success = mm_init();
        if (init_success == -1) // error
            return NULL;
    }

    // 8-byte aligning
    block_size = ALIGN(size) + 2 * WORDSIZE; // Add header, footer space

    // search for free block
    block_ptr = first_fit(block_size);

    if (block_ptr == NULL) { // no free block found
        extension_size = block_size > PAGESIZE ? block_size : PAGESIZE;
        block_ptr = extend_heap(extension_size / WORDSIZE);
        if (block_ptr == NULL) // error
            return NULL;
    }

    allocate(block_ptr, block_size);

    return block_ptr;
}
/**
*   Allocate a block by incrementing the brk pointer.
*   Always allocate a block whose size is a multiple of the alignment.
*
* @param size The size of the block to be allocated.
*
* @return block_ptr The pointer of the allocated block.
* @return NULL If the block is not allocated, return NULL.
* 
* @note If the size is 0, return NULL.
*       If the heap is not initialized, initialize the heap.
*       Align the size to 8-byte, add header and footer space.
*       Search for the free block that fits size.
*       If no free block fits size, extend heap by PAGESIZE.
*       Allocate the block, and return the pointer of the block.
*/

void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HEADER_PTR(ptr));

    // Initialize free block
    PUT(NEXT_PTR(ptr), NULL) ; // init Next as NULL
    PUT(PREV_PTR(ptr), NULL); // init Prev as NULL
    PUT(HEADER_PTR(ptr), size | FREE); // Header of current block
    PUT(FOOTER_PTR(ptr), size | FREE); // Footer of current block

    coalesce(ptr);

    return;
}
/**
*   Freeing a block does nothing.
*
* @param ptr The pointer of the block to be freed.
*
* @return void
* 
* @note Initialize the free block.
*       Update the header and footer of the block.
*       Coalesce the block if needed.
*/

void *mm_realloc(void *ptr, size_t size)
{
    void* next_block_ptr = NEXT_BLOCK_PTR(ptr); // Pointer of next block
    void* newptr;
    void* rest_block_ptr;
    
    size_t is_next_allocated = GET_IS_ALLOCATED(HEADER_PTR(NEXT_BLOCK_PTR(ptr))); // Locate header of next block and extract allocation bit
    size_t old_size;
    size_t next_size;
    size_t allocate_size;

    if (ptr == NULL) // Allocate if ptr is NULL
        return mm_malloc(size);

    if (size == 0) { // free block if size is 0
        mm_free(ptr);
        return NULL;
    }

    size = ALIGN(size) + 2 * WORDSIZE; 
    old_size = GET_SIZE(HEADER_PTR(ptr));
    next_size = GET_SIZE(HEADER_PTR(NEXT_BLOCK_PTR(ptr)));

    if (size > old_size) { // Realloc to larger size
        if (next_size >= size - old_size && !is_next_allocated) { // Next block is free and has enough space
            delete_free_block(next_block_ptr);
            
            PUT(HEADER_PTR(ptr), old_size + next_size | ALLOCATED);
            PUT(FOOTER_PTR(ptr), old_size + next_size | ALLOCATED);
            
            return ptr;
        }

        else if (next_size < size - old_size && !is_next_allocated) { // Next block is free but does not have enough space
            
            // Try to extend heap
            allocate_size = size - (old_size + next_size) > PAGESIZE ? size - (old_size + next_size) : PAGESIZE;
            if (extend_heap(allocate_size / PAGESIZE) == NULL) {
                return NULL; // Failed to extend heap
            }
            size += allocate_size; 
            delete_free_block(next_block_ptr);
            PUT(HEADER_PTR(ptr), size | ALLOCATED); 
            PUT(FOOTER_PTR(ptr), size | ALLOCATED);

            return ptr;
        }

        else{ // Next block is allocated or does not exist
            // Allocate to new block
            newptr = mm_malloc(size);
            allocate(newptr, size);
            memcpy(newptr, ptr, size);
            mm_free(ptr); 

            return newptr;
        }
    }

    return ptr;
}
/**
*   Reallocate the block with new size.
*
* @param ptr The pointer of the block to be reallocated.
* @param size The size of the block to be reallocated.
*
* @return ptr The pointer of the reallocated block.
* 
* @note If ptr is NULL, allocate the block with size.
*       If size is 0, free the block.
*       Align the size to 8-byte, add header and footer space.
*       If the new size is larger than the old size, check if the next block is free and has enough space.
*       If the next block is free and has enough space -> merge the blocks.
*       If the next block is free but does not have enough space -> extend heap and merge the blocks.
*       If the next block is allocated or does not exist -> allocate new block, move the payload, and free the old block.
*       Return the pointer of the reallocated block.
*/
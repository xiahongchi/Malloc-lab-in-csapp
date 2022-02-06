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

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)

#define MAX(x,y) ((x)>(y)? (x):(y))

#define PACK(size,alloc) ((size)|(alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p,val) (*(unsigned int *)(p)=(val))

#define GET_SIZE(p) (GET(p) & (~0x7))
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char*)(bp)-WSIZE)
#define FTRP(bp) ((char*)(bp)+GET_SIZE(HDRP(bp))-DSIZE)

#define NEXT_BLKP(bp) ((char*)(bp)+GET_SIZE((char*)(bp)-WSIZE))
#define PREV_BLKP(bp) ((char*)(bp)-GET_SIZE((char*)(bp)-DSIZE))

static void* heap_listp;
static void* extend_heap(size_t words);
static void* coalesce(void* ptr);
static void* find_fit(size_t asize);
static void place(void* ptr,size_t asize);
static void* first_match(size_t asize);
static void* next_match(size_t asize);
static void* best_match(size_t asize);
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if((heap_listp=mem_sbrk(4*WSIZE))==(void*)-1){
        return -1;
    }
    PUT(heap_listp,0);
    PUT(heap_listp+(1*WSIZE),PACK(DSIZE,1));
    PUT(heap_listp+(2*WSIZE),PACK(DSIZE,1));
    PUT(heap_listp+(3*WSIZE),PACK(0,1));
    heap_listp+=(2*WSIZE);

    if(extend_heap(CHUNKSIZE/WSIZE)==NULL){
        return -1;
    }
    return 0;
}
static void* extend_heap(size_t words){
    char* bp;
    size_t size;
    size=(words%2)? (words+1)*WSIZE:words*WSIZE;
    if((long)(bp=mem_sbrk(size))==-1){
        return NULL;
    }
    PUT(HDRP(bp),PACK(size,0));
    PUT(FTRP(bp),PACK(size,0));
    PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1));

    return coalesce(bp);
}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extend_size;
    char* ptr;
    if(size==0) return NULL;
    asize=ALIGN(size+DSIZE);
    if((ptr=find_fit(asize))!=NULL){
        place(ptr,asize);
        return ptr;
    }   
    extend_size=MAX(asize,CHUNKSIZE);
    if((ptr=extend_heap(extend_size/WSIZE))==NULL){
        return NULL;
    }
    place(ptr,asize);
    return ptr;
}
static void* find_fit(size_t asize){
    return best_match(asize);
}
static void* first_match(size_t asize){
    void *ptr=heap_listp;
    ptr=NEXT_BLKP(ptr);
    while(GET_SIZE(HDRP(ptr))!=0){
        if(GET_ALLOC(HDRP(ptr))==0&&GET_SIZE(HDRP(ptr))>=asize){
            return ptr;
        }
        ptr=NEXT_BLKP(ptr);
    }
    return NULL;
}
static void* next_match(size_t asize){
}
static void* best_match(size_t asize){
    void*ptr=heap_listp;
    void*minp=NULL;
    size_t min=(~0);
    ptr=NEXT_BLKP(ptr);
    size_t cursize;
    while((cursize=GET_SIZE(HDRP(ptr)))!=0){
        if(GET_ALLOC(HDRP(ptr))==0&&cursize>=asize){
            if(cursize<min){
                min=cursize;
                minp=ptr;
            }
        }
        ptr=NEXT_BLKP(ptr);
    }
    return minp;
}
static void place(void* ptr,size_t asize){
    size_t csize=GET_SIZE(HDRP(ptr));
    if(csize==asize){
        PUT(HDRP(ptr),PACK(asize,1));
        PUT(FTRP(ptr),PACK(asize,1));
    }
    else{
        PUT(HDRP(ptr),PACK(asize,1));
        PUT(FTRP(ptr),PACK(asize,1));
        ptr=NEXT_BLKP(ptr);
        PUT(HDRP(ptr),PACK(csize-asize,0));
        PUT(FTRP(ptr),PACK(csize-asize,0));
    }
    return;
}
/*
 * mm_free
 */
void mm_free(void *ptr)
{
    size_t size=GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr),PACK(size,0));
    PUT(FTRP(ptr),PACK(size,0));
    coalesce(ptr);
}
static void* coalesce(void* ptr){
    size_t prev_alloc=GET_ALLOC(FTRP(PREV_BLKP(ptr)));
    size_t next_alloc=GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t size=GET_SIZE(HDRP(ptr));
    if(prev_alloc&&next_alloc){
        return ptr;
    }
    else if(!prev_alloc&&next_alloc){
        size_t prev_size=GET_SIZE(FTRP(PREV_BLKP(ptr)));
        PUT(HDRP(PREV_BLKP(ptr)),PACK((prev_size+size),0));
        PUT(FTRP(ptr),PACK((prev_size+size),0));
        ptr=PREV_BLKP(ptr);
        return ptr;
    }
    else if(prev_alloc&&!next_alloc){
        size_t next_size=GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(ptr),PACK((size+next_size),0));
        PUT(FTRP(ptr),PACK((size+next_size),0));
        return ptr;
    }
    else{
        size_t prev_size=GET_SIZE(FTRP(PREV_BLKP(ptr)));
        size_t next_size=GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(PREV_BLKP(ptr)),PACK((prev_size+size+next_size),0));
        PUT(FTRP(NEXT_BLKP(ptr)),PACK((prev_size+size+next_size),0));
        ptr=PREV_BLKP(ptr);
        return ptr;
    }
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if(ptr==NULL){
        return mm_malloc(size);
    }
    if(size==0){
        mm_free(ptr);
        return NULL;
    }
    size_t csize=GET_SIZE(HDRP(ptr));
    size_t asize=ALIGN(size+DSIZE);
    size_t prev_alloc=GET_ALLOC(FTRP(PREV_BLKP(ptr)));
    size_t next_alloc=GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t prev_size=GET_SIZE(FTRP(PREV_BLKP(ptr)));
    size_t next_size=GET_SIZE(HDRP(NEXT_BLKP(ptr)));
    if(next_alloc==1&&prev_alloc==1){
        if(csize>asize){
            PUT(HDRP(ptr),PACK(asize,1));
            PUT(FTRP(ptr),PACK(asize,1));
            void*next=NEXT_BLKP(ptr);
            PUT(HDRP(next),PACK(csize-asize,0));
            PUT(FTRP(next),PACK(csize-asize,0));
            return ptr;
        }
        if(csize==asize){
            return ptr;
        }
    }
    if(next_alloc==0&&prev_alloc==1){
        if(next_size+csize>asize){
            PUT(HDRP(ptr),PACK(asize,1));
            PUT(FTRP(ptr),PACK(asize,1));
            void*next=NEXT_BLKP(ptr);
            PUT(HDRP(next),PACK(next_size+csize-asize,0));
            PUT(FTRP(next),PACK(next_size+csize-asize,0));
            return ptr;
        }
        else if(next_size+csize==asize){
            PUT(HDRP(ptr),PACK(asize,1));
            PUT(FTRP(ptr),PACK(asize,1));
            return ptr;
        }
    }
    if(next_alloc==1&&prev_alloc==0&&prev_size+csize>=asize){
        void* prev=PREV_BLKP(ptr);
        PUT(HDRP(prev),PACK(asize,1));
        unsigned i;
        for(i=0;i<csize-DSIZE;i++){
            *((char*)prev+i)=*((char*)ptr+i);
        }
        PUT(FTRP(prev),PACK(asize,1));
        if(prev_size+csize>asize){
            ptr=NEXT_BLKP(prev);
            PUT(HDRP(ptr),PACK(prev_size+csize-asize,0));
            PUT(FTRP(ptr),PACK(prev_size+csize-asize,0));
        }
        return prev;
    }
    if(next_alloc==0&&prev_alloc==0&&prev_size+csize+next_size>=asize){
        void* prev=PREV_BLKP(ptr);
        PUT(HDRP(prev),PACK(asize,1));
        unsigned i;
        for(i=0;i<csize-DSIZE;i++){
            *((char*)prev+i)=*((char*)ptr+i);
        }
        PUT(FTRP(prev),PACK(asize,1));
        if(prev_size+csize+next_size>asize){
            ptr=NEXT_BLKP(prev);
            PUT(HDRP(ptr),PACK(prev_size+csize+next_size-asize,0));
            PUT(FTRP(ptr),PACK(prev_size+csize+next_size-asize,0));
        }
        return prev;
    }
    void* newptr=mm_malloc(size);
    if(!newptr) return NULL;
    memcpy(newptr,ptr,csize-DSIZE);
    mm_free(ptr);
    return newptr;
}
















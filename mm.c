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
#define CHUNKSIZE (1<<8)

#define MAX(x,y) ((x)>(y)? (x):(y))

#define PACK(size,alloc) ((size)|(alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p,val) (*(unsigned int *)(p)=(val))
#define DPUT(p,val) (*((unsigned *)(p))=(unsigned )(val))

#define GET_SIZE(p) (GET(p) & (~0x7))
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char*)(bp)-WSIZE)
#define FTRP(bp) ((char*)(bp)+GET_SIZE(HDRP(bp))-DSIZE)

#define NEXT_BLKP(bp) ((char*)(bp)+GET_SIZE((char*)(bp)-WSIZE))
#define PREV_BLKP(bp) ((char*)(bp)-GET_SIZE((char*)(bp)-DSIZE))

static void* heap_listp;
static void* free_listp;
static void* extend_heap(size_t words);
static void* coalesce(void* ptr);
static void* find_fit(size_t asize);
static void place(void* ptr,size_t asize);
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
    free_listp=NULL;
    if(extend_heap((1<<6)/WSIZE)==NULL) return -1;
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

    size_t prev_alloc=GET_ALLOC(FTRP(PREV_BLKP(bp)));
    if(prev_alloc==0){
        return coalesce(bp);
    }
    else{
        if(free_listp==NULL){
            free_listp=bp;
            DPUT(free_listp,0);
            DPUT(free_listp+WSIZE,0);
        }
        else{
            DPUT(bp,0);
            DPUT(bp+WSIZE,free_listp);
            DPUT(free_listp,bp);
            free_listp=bp;
        }
        return bp;
    }
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
    /*
    if(GET_ALLOC(mem_heap_hi()+1-DSIZE)==0){
        if((ptr=extend_heap(asize/WSIZE))==NULL){
            return NULL;
        }
        place(ptr,asize);
        return ptr;
    }
    */
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
static void* next_match(size_t asize){
    void *ptr=free_listp;
    while(ptr!=NULL){
        if(GET_ALLOC(HDRP(ptr))==0&&GET_SIZE(HDRP(ptr))>=asize){
            return ptr;
        }
        ptr=(void*)(*((unsigned *)(ptr+WSIZE)));
    }
    return NULL;
}
static void* best_match(size_t asize){
    void*ptr=free_listp;
    void*minp=NULL;
    size_t min=(~0);
    while(ptr!=NULL){
        size_t cursize=GET_SIZE(HDRP(ptr));
        if(GET_ALLOC(HDRP(ptr))==0&&cursize>=asize){
            if(cursize==asize) return ptr;
            else if(cursize<min){
                min=cursize;
                minp=ptr;
            }
        }
        ptr=(void*)(*((unsigned *)(ptr+WSIZE)));
    }
    return minp;
}
static void place(void* ptr,size_t asize){
    size_t csize=GET_SIZE(HDRP(ptr));
    if(csize-asize>=2*DSIZE){
        void*lprev=(void*)(*((unsigned *)ptr));
        void*lnext=(void*)(*((unsigned *)(ptr+WSIZE)));
        PUT(HDRP(ptr),PACK(asize,1));
        PUT(FTRP(ptr),PACK(asize,1));
        
        ptr=NEXT_BLKP(ptr);
        PUT(HDRP(ptr),PACK((csize-asize),0));
        PUT(FTRP(ptr),PACK((csize-asize),0));
        if(lprev!=0&&lnext!=0){
            DPUT((lprev+WSIZE),lnext);
            DPUT(lnext,lprev);
            DPUT(ptr,0);
            DPUT((ptr+WSIZE),free_listp);
            DPUT(free_listp,ptr);
            free_listp=ptr;
        }
        else if(lprev==0&&lnext!=0){
            DPUT(lnext,ptr);
            DPUT(ptr,0);
            DPUT((ptr+WSIZE),lnext);
            free_listp=ptr;
        }
        else if(lprev!=0&&lnext==0){
            DPUT((lprev+WSIZE),lnext);
            DPUT(ptr,0);
            DPUT((ptr+WSIZE),free_listp);
            DPUT(free_listp,ptr);
            free_listp=ptr;
        }
        else{
            DPUT(ptr,0);
            DPUT((ptr+WSIZE),0);
            free_listp=ptr;
        }
    }
    else{
        void*lprev=(void*)(*((unsigned *)ptr));
        void*lnext=(void*)(*((unsigned *)(ptr+WSIZE)));
        PUT(HDRP(ptr),PACK(csize,1));
        PUT(FTRP(ptr),PACK(csize,1));
        if(lprev!=0&&lnext!=0){
            DPUT((lprev+WSIZE),lnext);
            DPUT(lnext,lprev);
        }
        else if(lprev==0&&lnext!=0){
            DPUT(lnext,lprev);
            free_listp=lnext;
        }
        else if(lprev!=0&&lnext==0){
            DPUT((lprev+WSIZE),lnext);
        }
        else{
            free_listp=NULL;
        }
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
    if(free_listp!=NULL){
        //coalesce
        coalesce(ptr);
    }
    else{
        free_listp=ptr;
        DPUT(free_listp,0);
        DPUT((free_listp+WSIZE),0);
    }
    return;
}
static void* coalesce(void* ptr){
    size_t prev_alloc=GET_ALLOC(FTRP(PREV_BLKP(ptr)));
    size_t next_alloc=GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t size=GET_SIZE(HDRP(ptr));
    if(prev_alloc&&next_alloc){
        //free_list
        DPUT(ptr,0);
        DPUT((ptr+WSIZE),free_listp);
        DPUT(free_listp,ptr);
        free_listp=ptr;
        return ptr;
    }
    else if(!prev_alloc&&next_alloc){
        void*prev=PREV_BLKP(ptr);
        void*prev_lprev=(void*)(*((unsigned *)prev));
        void*prev_lnext=(void*)(*((unsigned *)(prev+WSIZE)));
        
        if(prev_lprev!=0&&prev_lnext!=0){
            //detach
            DPUT((prev_lprev+WSIZE),prev_lnext);
            DPUT(prev_lnext,prev_lprev);
            //insert
            DPUT(prev,0);
            DPUT((prev+WSIZE),free_listp);
            DPUT(free_listp,prev);
            free_listp=prev;
        }
        else if(prev_lprev!=0&&prev_lnext==0){
            DPUT((prev_lprev+WSIZE),prev_lnext);
            //insert
            DPUT(prev,0);
            DPUT((prev+WSIZE),free_listp);
            DPUT(free_listp,prev);
            free_listp=prev;
        }
        size_t prev_size=GET_SIZE(FTRP(PREV_BLKP(ptr)));
        PUT(HDRP(PREV_BLKP(ptr)),PACK((prev_size+size),0));
        PUT(FTRP(ptr),PACK((prev_size+size),0));
        ptr=PREV_BLKP(ptr);
        return ptr;
    }
    else if(prev_alloc&&!next_alloc){
        void*next=NEXT_BLKP(ptr);
        void*next_lprev=(void*)(*((unsigned *)next));
        void*next_lnext=(void*)(*((unsigned *)(next+WSIZE)));
        if(next_lprev!=0&&next_lnext!=0){
            //detach
            DPUT((next_lprev+WSIZE),next_lnext);
            DPUT(next_lnext,next_lprev);
            //insert
            DPUT(ptr,0);
            DPUT((ptr+WSIZE),free_listp);
            DPUT(free_listp,ptr);
            free_listp=ptr;
        }
        else if(next_lprev==0&&next_lnext!=0){
            //detach
            DPUT(next_lnext,ptr);
            //insert
            free_listp=ptr;
            DPUT(ptr,0);
            DPUT((ptr+WSIZE),next_lnext);
        }
        else if(next_lprev!=0&&next_lnext==0){
            //detach
            DPUT((next_lprev+WSIZE),next_lnext);
            //insert
            DPUT(ptr,0);
            DPUT((ptr+WSIZE),free_listp);
            DPUT(free_listp,ptr);
            free_listp=ptr;
        }
        else{
            free_listp=ptr;
            DPUT(ptr,0);
            DPUT((ptr+WSIZE),0);
        }
        
        size_t next_size=GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(ptr),PACK((size+next_size),0));
        PUT(FTRP(ptr),PACK((size+next_size),0));
        return ptr;
    }
    else{
        void*next=NEXT_BLKP(ptr);
        void*prev=PREV_BLKP(ptr);
        void*next_lprev=(void*)(*((unsigned *)next));
        void*next_lnext=(void*)(*((unsigned *)(next+WSIZE)));
        //detach next
        if(next_lprev!=0&&next_lnext!=0){
            DPUT((next_lprev+WSIZE),next_lnext);
            DPUT(next_lnext,next_lprev);
        }
        else if(next_lprev==0&&next_lnext!=0){
            DPUT(next_lnext,next_lprev);
            free_listp=next_lnext;
        }
        else if(next_lprev!=0&&next_lnext==0){
            DPUT((next_lprev+WSIZE),next_lnext);
        }
        else{
            printf("impossible!!!");
            pause();
        }
        void*prev_lprev=(void*)(*((unsigned *)prev));
        void*prev_lnext=(void*)(*((unsigned *)(prev+WSIZE)));
        
        //prev
        if(prev_lprev!=0&&prev_lnext!=0){
            //detach
            DPUT((prev_lprev+WSIZE),prev_lnext);
            DPUT(prev_lnext,prev_lprev);
            //insert
            DPUT(prev,0);
            DPUT((prev+WSIZE),free_listp);
            DPUT(free_listp,prev);
            free_listp=prev;
        }
        else if(prev_lprev!=0&&prev_lnext==0){
            DPUT((prev_lprev+WSIZE),prev_lnext);
            //insert
            DPUT(prev,0);
            DPUT((prev+WSIZE),free_listp);
            DPUT(free_listp,prev);
            free_listp=prev;
        }
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
    if(next_alloc==1&&prev_alloc==1&&csize>=asize){
        
        if(csize-asize>=2*DSIZE){
            PUT(HDRP(ptr),PACK(asize,1));
            PUT(FTRP(ptr),PACK(asize,1));
            void*next=NEXT_BLKP(ptr);
            PUT(HDRP(next),PACK(csize-asize,0));
            PUT(FTRP(next),PACK(csize-asize,0));
            
            if(free_listp==NULL){
                free_listp=next;
                DPUT(free_listp,0);
                DPUT(free_listp+WSIZE,0);
            }
            else{
                DPUT(next,0);
                DPUT(next+WSIZE,free_listp);
                DPUT(free_listp,next);
                free_listp=next;
            }
            
            return ptr;
        }
        
        else{
            return ptr;
        }
    }
    if(next_alloc==0&&prev_alloc==1&&next_size+csize>=asize){
        //detach next
        void*next=NEXT_BLKP(ptr);
        void*next_lprev=(void*)GET(next);
        void*next_lnext=(void*)GET(next+WSIZE);
        if(next_lprev!=0&&next_lnext!=0){
            DPUT((next_lprev+WSIZE),next_lnext);
            DPUT(next_lnext,next_lprev);
        }
        else if(next_lprev==0&&next_lnext!=0){
            DPUT(next_lnext,next_lprev);
            free_listp=next_lnext;
        }
        else if(next_lprev!=0&&next_lnext==0){
            DPUT((next_lprev+WSIZE),next_lnext);
        }
        else{
            free_listp=NULL;
        }
        /*----------------*/
        if(next_size+csize-asize>=2*DSIZE){
            PUT(HDRP(ptr),PACK(asize,1));
            PUT(FTRP(ptr),PACK(asize,1));
            void*next=NEXT_BLKP(ptr);
            PUT(HDRP(next),PACK(next_size+csize-asize,0));
            PUT(FTRP(next),PACK(next_size+csize-asize,0));
            if(free_listp==NULL){
                free_listp=next;
                DPUT(free_listp,0);
                DPUT((free_listp+WSIZE),0);
            }
            else{
                DPUT(next,0);
                DPUT((next+WSIZE),free_listp);
                DPUT(free_listp,next);
                free_listp=next;
            }
            return ptr;
        }
        else{
            PUT(HDRP(ptr),PACK(next_size+csize,1));
            PUT(FTRP(ptr),PACK(next_size+csize,1));
            return ptr;
        }
    }
    if(next_alloc==1&&prev_alloc==0&&prev_size+csize>=asize){
        void* prev=PREV_BLKP(ptr);
        /*-------------*/
        //detach prev
        void*prev_lprev=(void*)GET(prev);
        void*prev_lnext=(void*)GET(prev+WSIZE);
        if(prev_lprev!=0&&prev_lnext!=0){
            DPUT((prev_lprev+WSIZE),prev_lnext);
            DPUT(prev_lnext,prev_lprev);
        }
        else if(prev_lprev==0&&prev_lnext!=0){
            DPUT(prev_lnext,prev_lprev);
            free_listp=prev_lnext;
        }
        else if(prev_lprev!=0&&prev_lnext==0){
            DPUT((prev_lprev+WSIZE),prev_lnext);
        }
        else{
            free_listp=NULL;
        }
        /*--------------*/
        unsigned i;
        for(i=0;i<csize-DSIZE;i++){
            *((char*)prev+i)=*((char*)ptr+i);
        }
        if(prev_size+csize-asize>=2*DSIZE){
            PUT(HDRP(prev),PACK(asize,1));
            PUT(FTRP(prev),PACK(asize,1));
            ptr=NEXT_BLKP(prev);
            PUT(HDRP(ptr),PACK(prev_size+csize-asize,0));
            PUT(FTRP(ptr),PACK(prev_size+csize-asize,0));
            if(free_listp==NULL){
                free_listp=ptr;
                DPUT(free_listp,0);
                DPUT((free_listp+WSIZE),0);
            }
            else{
                DPUT(ptr,0);
                DPUT(ptr+WSIZE,free_listp);
                DPUT(free_listp,ptr);
                free_listp=ptr;
            }
            return prev;
        }
        else{
            PUT(HDRP(prev),PACK(prev_size+csize,1));
            PUT(FTRP(prev),PACK(prev_size+csize,1));
            return prev;
        }
    }
    if(next_alloc==0&&prev_alloc==0&&prev_size+csize+next_size>=asize){
        void* prev=PREV_BLKP(ptr);
        void* next=NEXT_BLKP(ptr);
        /*------------------*/
        //detach prev
        void*prev_lprev=(void*)GET(prev);
        void*prev_lnext=(void*)GET(prev+WSIZE);
        if(prev_lprev!=0&&prev_lnext!=0){
            DPUT((prev_lprev+WSIZE),prev_lnext);
            DPUT(prev_lnext,prev_lprev);
        }
        else if(prev_lprev==0&&prev_lnext!=0){
            DPUT(prev_lnext,prev_lprev);
            free_listp=prev_lnext;
        }
        else if(prev_lprev!=0&&prev_lnext==0){
            DPUT((prev_lprev+WSIZE),prev_lnext);
        }
        else{
            free_listp=NULL;
        }
        //detach next
        void*next_lprev=(void*)GET(next);
        void*next_lnext=(void*)GET(next+WSIZE);
        if(next_lprev!=0&&next_lnext!=0){
            DPUT((next_lprev+WSIZE),next_lnext);
            DPUT(next_lnext,next_lprev);
        }
        else if(next_lprev==0&&next_lnext!=0){
            DPUT(next_lnext,next_lprev);
            free_listp=next_lnext;
        }
        else if(next_lprev!=0&&next_lnext==0){
            DPUT((next_lprev+WSIZE),next_lnext);
        }
        else{
            free_listp=NULL;
        }
        /*------------------*/
        unsigned i;
        for(i=0;i<csize-DSIZE;i++){
            *((char*)prev+i)=*((char*)ptr+i);
        }
        if(prev_size+csize+next_size-asize>=2*DSIZE){
            PUT(HDRP(prev),PACK(asize,1));
            PUT(FTRP(prev),PACK(asize,1));
            ptr=NEXT_BLKP(prev);
            PUT(HDRP(ptr),PACK(prev_size+csize+next_size-asize,0));
            PUT(FTRP(ptr),PACK(prev_size+csize+next_size-asize,0));
            if(free_listp==NULL){
                free_listp=ptr;
                DPUT(free_listp,0);
                DPUT((free_listp+WSIZE),0);
            }
            else{
                DPUT(ptr,0);
                DPUT(ptr+WSIZE,free_listp);
                DPUT(free_listp,ptr);
                free_listp=ptr;
            }
            return prev;
        }
        else{
            PUT(HDRP(prev),PACK(prev_size+csize+next_size,1));
            PUT(FTRP(prev),PACK(prev_size+csize+next_size,1));
            return prev;
        }
    }
    void* newptr=mm_malloc(size);
    if(!newptr) return NULL;
    memcpy(newptr,ptr,csize-DSIZE);
    mm_free(ptr);
    return newptr;
}
















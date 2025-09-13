#include "mymalloc.h"
#include <sys/syscall.h>
#ifdef DEBUG  
    #include <stdio.h>
    #include <assert.h>

    #define DBG_PRINT_LOCK_STATE(lbl, blk) \
        printf("%-15s: block=%p lock=%d\n", lbl, blk, blk->lock.status)
        
    #define DBG_PRINT_LIST(lbl, list) \
        do { \
            printf("----- %s -----\n", lbl); \
            printf_total_list(list); \
        } while(0)

    #define DBG_PRINT_FREELIST(lbl, list) \
    do { \
        printf("----- %s -----\n", lbl); \
        printf_free_list(list); \
    } while(0)
        
    #define DBG_PRINT(fmt, ...) \
        printf(fmt, ##__VA_ARGS__)
#else
    #define DBG_PRINT_LOCK_STATE(lbl, blk)
    #define DBG_PRINT_LIST(lbl, list)
    #define DBG_PRINT_FREELIST(lbl, list)
    #define DBG_PRINT(fmt, ...)
#endif

typedef short pid_t;

#define PAGESIZE    0x1000
#define MAX_THREADS 64

// 每个 block 占据空间 block_meta + size
/*
++++++++++++++++++++++++++++++++++++++
+          +                         +
+   meta   +           size          +
+          +                         +
++++++++++++++++++++++++++++++++++++++
*/
typedef struct block {
    size_t capacity;           // 容量（不含元数据，头指针专属）
    size_t size;               // 可用内存大小（不含元数据）
    spinlock_t lock;           // 局部锁
    int is_free;               // 是否空闲（0/1）
    struct block *head;        // 指向链表头
    struct block *next;        // 指向下一个内存块
    struct block *prev;        // 指向上一个内存块 
    struct block *next_free;   // 指向下一个空闲块
    int tfd;                   // 所属线程号标识符
} block;

block* mem_blocks = NULL;
size_t mem_blocks_size = PAGESIZE;

struct ThreadEntry {
    pid_t tid;
    // block* mem;
    block* first_free;
};

static struct ThreadEntry thread_table[MAX_THREADS];
static int thread_count = 0;
spinlock_t global_lock;

static inline pid_t gettid(void) {
    pid_t tid;
    __asm__ volatile (
        "syscall"
        : "=a" (tid)
        : "0" (SYS_gettid)
        : "rcx", "r11", "memory"
    );
    return tid;
}

#ifdef DEBUG
static void printf_free_list(block *head) {
    printf("free_list: ");
    while(head) {
        printf("size %ld capacity %ld addr %p -> ", head->size, head->capacity, head);
        head = head->next_free;
    }
    printf("\n");
}

static void printf_total_list(block *head) {
    printf("total_list: ");
    while(head) {
        printf("size %ld capacity %ld addr %p -> ", head->size, head->capacity, head);
        head = head->next;
    }
    printf("\n");
}
#endif


size_t aligned_size(size_t size) {
    return (size + PAGESIZE - 1) / PAGESIZE * PAGESIZE;
}

block* allocate_page(int tfd, size_t size) {
    block* dummy = (block *)vmalloc(NULL, size);
    if(!dummy) return NULL;

    dummy->size = 0;
    dummy->capacity = size - sizeof(block);
    dummy->head = dummy;

    block* mem = dummy + 1;

    dummy->next = mem;
    dummy->prev = NULL;
    dummy->next_free = mem;
    dummy->is_free = 1;
    dummy->tfd = tfd;
    
    mem->size = size - 2 * sizeof(block);
    mem->capacity = 0;
    mem->head = dummy;
    mem->next = NULL;
    mem->prev = dummy;
    mem->next_free = NULL;
    mem->is_free = 1;
    mem->tfd = tfd;

    return dummy;
}


void *mymalloc(size_t size) {
    pid_t tid = gettid();
    int tfd = 0;
    
    for(tfd = 0; tfd < thread_count; tfd ++) {
        if(tid != thread_table[tfd].tid) continue;
        else break;
    }
    struct ThreadEntry* thread = &thread_table[tfd];

    if(tfd == thread_count) { 
        spin_lock(&global_lock);
        tfd = thread_count ++;
        spin_unlock(&global_lock);

        thread->tid = tid;
        thread->first_free = allocate_page(tfd, mem_blocks_size);
    }
    
    
    if (size == 0) return NULL;
    size = (size + 7) & ~7; // 8 字节对齐

    void *ptr = NULL;
    block *current = thread->first_free;
    // block *prev = NULL; 

    DBG_PRINT("===== Begin malloc(%p) =====\n", current);
    // DBG_PRINT_FREELIST("Begin malloc", current);


    while(!ptr) {
        // 当前空间已经不足以分配
        if(!current) {
            // 计算所需空间，已经对齐
            size_t needed_size = aligned_size(size + 3 * sizeof(block));

            // 创建新空间
            block* dummy = allocate_page(tfd, needed_size);
            if(!dummy) return NULL;
        
            block* old_dummy = thread->first_free;
            if(old_dummy) {
                spin_lock(&old_dummy->lock);
                thread->first_free = dummy;
                spin_unlock(&old_dummy->lock);
            } else {
                thread->first_free = dummy;
            }
            
            current = dummy;
        }

        spin_lock(&current->lock);
        // 尝试分配
        if(current->is_free && current->size >= size + sizeof(block)) {
            size_t remain = current->size - size;
            
            block *new_block = (block*)((void*)current + sizeof(block) + size);
            new_block->size = remain - sizeof(block);
            new_block->capacity = 0;  // 从母块拆分出来的块没有容量
            new_block->is_free = 1;
            if(current->next) {
                current->next->prev = new_block;
            }
            new_block->head = current->head;
            new_block->next = current->next;
            new_block->prev = current;
            new_block->tfd = tfd;

            new_block->next_free = current->next_free;
            

            current->size = size;
            current->next = new_block;
            current->next_free = NULL;

            ptr = (void *)(current + 1);
            current->is_free = 0;
            
        }
        spin_unlock(&current->lock);
        current = current->next_free;
    }
     
    // DBG_PRINT_LIST("After malloc", thread->first_free->head);
    DBG_PRINT("===== END malloc(%p) =====\n\n", ptr);
    return ptr;
}

void merge(block *curr_block, block *next_block) { 

    DBG_PRINT("merge current %ld and next_block %ld\n", curr_block->size, next_block->size);

    curr_block->size += next_block->size + sizeof(block);
    if(next_block->next) {
        next_block->next->prev = curr_block;
    }
    curr_block->next = next_block->next;
    curr_block->next_free = next_block->next_free;

    DBG_PRINT("merge complete current size %ld\n", curr_block->size);

}

void myfree(void *ptr) {
    block *current = (block *)ptr - 1;

    int tfd = current->tfd;
    struct ThreadEntry* thread = &thread_table[tfd];

    DBG_PRINT("===== BEGIN free(%p) =====\n", ptr);
    DBG_PRINT_LIST("Before free", current->head);

    block *curr_block = current;
    block *next_block = current->next;
    block *prev_block = current->prev;

    spin_lock(&curr_block->lock);
    DBG_PRINT_LOCK_STATE("Curr lock", current);

    // 合并后一块
    if(next_block && next_block->is_free) {
        spin_lock(&next_block->lock);
        merge(curr_block, next_block);
        DBG_PRINT_LIST("After forward merge", current->head);
        spin_unlock(&next_block->lock);
    }
    spin_unlock(&curr_block->lock);

    // 合并前一块
    if(prev_block && prev_block->is_free) {
        spin_lock(&prev_block->lock);
        spin_lock(&curr_block->lock);
        // merge prev block
        if(prev_block->capacity == 0) {
            merge(prev_block, curr_block);            
            DBG_PRINT_LIST("After backward merge", current->head);
        }
        spin_unlock(&prev_block->lock);
    } else {
        spin_lock(&curr_block->lock);
    }

    if(current->size + sizeof(block) == curr_block->prev->capacity) {
        block *dummy = curr_block->prev;

        if(thread->first_free && dummy == thread->first_free->head) {
            thread->first_free = NULL;
        }

        DBG_PRINT("Checking head: current size %lu, head cap %lu\n",
            current->size + sizeof(block), dummy->capacity);
        
        dummy->capacity = 0;
        dummy->next_free = NULL;
        dummy->next = NULL; 
        vmfree(dummy, dummy->capacity + sizeof(block));
        dummy = NULL;

    } else {
        spin_unlock(&curr_block->lock);
    }

    DBG_PRINT("===== END free(%p) =====\n\n", ptr);
}

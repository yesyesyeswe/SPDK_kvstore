#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

typedef struct {
    atomic_int status;
} spinlock_t;

#define LOCKED   1
#define UNLOCKED 0

static inline void spin_lock(spinlock_t *lock) {
    int expected;
    do {
        expected = UNLOCKED;
    } while (!atomic_compare_exchange_strong(&lock->status, &expected, LOCKED));
}

static inline void spin_unlock(spinlock_t *lock) {
    atomic_store_explicit(&lock->status, UNLOCKED, memory_order_release);
}

void *mymalloc(size_t size);
void myfree(void *ptr);

#include <sys/mman.h>

void *vmalloc(void *addr, size_t length) {
    // length must be aligned to page size (4096).
    void *result = mmap(addr, length, PROT_READ | PROT_WRITE, 
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (result == MAP_FAILED) {
        return NULL;
    }
    return result;
}

void vmfree(void *addr, size_t length) {
    munmap(addr, length);
}
/* VioletOS - Kernel heap memory manager */
#include "violet.h"

#define HEAP_START   0x00100000
#define HEAP_SIZE    (4 * 1024 * 1024)
#define BLOCK_MAGIC  0xDEADBEEF
#define MIN_BLOCK    32

struct mem_block {
    uint32_t magic;
    size_t   size;
    bool     free;
    struct mem_block* next;
};

static struct mem_block* heap_start = NULL;
static size_t total_memory = 0;
static size_t used_memory  = 0;

void memory_init(struct multiboot_info* mb_info) {
    if (mb_info && (mb_info->flags & 0x01)) {
        total_memory = (mb_info->mem_upper + mb_info->mem_lower) * 1024;
    } else {
        total_memory = 64 * 1024 * 1024;
    }

    heap_start = (struct mem_block*)HEAP_START;
    heap_start->magic = BLOCK_MAGIC;
    heap_start->size  = HEAP_SIZE - sizeof(struct mem_block);
    heap_start->free  = true;
    heap_start->next  = NULL;
}

static struct mem_block* find_free_block(size_t size) {
    struct mem_block* current = heap_start;
    while (current) {
        if (current->free && current->size >= size)
            return current;
        current = current->next;
    }
    return NULL;
}

static void split_block(struct mem_block* block, size_t size) {
    if (block->size <= size + MIN_BLOCK + sizeof(struct mem_block))
        return;

    struct mem_block* new_block = (struct mem_block*)((uint8_t*)block + sizeof(struct mem_block) + size);
    new_block->magic = BLOCK_MAGIC;
    new_block->size  = block->size - size - sizeof(struct mem_block);
    new_block->free  = true;
    new_block->next  = block->next;
    block->size = size;
    block->next = new_block;
}

void* kmalloc(size_t size) {
    if (size == 0) return NULL;

    struct mem_block* block = find_free_block(size);
    if (!block) return NULL;

    split_block(block, size);
    block->free = false;
    used_memory += block->size + sizeof(struct mem_block);
    return (void*)((uint8_t*)block + sizeof(struct mem_block));
}

void* kcalloc(size_t count, size_t size) {
    size_t total = count * size;
    void* ptr = kmalloc(total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void kfree(void* ptr) {
    if (!ptr) return;

    struct mem_block* block = (struct mem_block*)((uint8_t*)ptr - sizeof(struct mem_block));
    if (block->magic != BLOCK_MAGIC) return;

    block->free = true;
    used_memory -= block->size + sizeof(struct mem_block);

    /* Coalesce adjacent free blocks */
    struct mem_block* current = heap_start;
    while (current && current->next) {
        if (current->free && current->next->free) {
            current->size += current->next->size + sizeof(struct mem_block);
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

size_t get_total_memory(void) { return total_memory; }
size_t get_used_memory(void)  { return used_memory; }
size_t get_free_memory(void)  { return total_memory > used_memory ? total_memory - used_memory : 0; }

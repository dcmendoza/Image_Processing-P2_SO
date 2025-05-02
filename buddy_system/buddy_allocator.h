// buddy_allocator.h
#ifndef BUDDY_ALLOCATOR_H
#define BUDDY_ALLOCATOR_H

#include <cstddef>
#include <vector>
#include <unordered_map>

class BuddyAllocator {
public:
    explicit BuddyAllocator(size_t totalSize);
    ~BuddyAllocator();

    void* alloc(size_t size);
    void free(void* ptr);
    void* realloc(void* ptr, size_t newSize);
    
    // Cache de memoria para operaciones repetitivas
    void* getCache(size_t size);
    void releaseCache();

    // Métodos para diagnóstico
    size_t getTotalSize() const { return totalSize; }
    void printStatus() const; // Método para imprimir estado del allocator

private:
    static const size_t MIN_BLOCK_SIZE = 64;  // Incrementado para mejor rendimiento
    static const int MAX_LEVELS = 20;

    struct Block {
        Block* next;
        size_t size;  // Tamaño real del bloque
    };

    void* memoryBase;
    size_t totalSize;
    Block* freeLists[MAX_LEVELS];
    std::unordered_map<void*, int> allocatedBlocks;  // Mapeo de punteros a niveles
    void* cacheMemory;
    size_t cacheSize;

    int getLevel(size_t size) const;
    size_t getBlockSize(int level) const;
    size_t offset(void* ptr) const;
    void* buddyOf(void* ptr, int level) const;
    bool split(int level); // Modificado para retornar éxito/fallo
    void coalesce(void* ptr, int level);
    bool isAligned(void* ptr, size_t alignment) const;
};

#endif // BUDDY_ALLOCATOR_H
#include "buddy_allocator.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <cstdint>

const size_t BuddyAllocator::MIN_BLOCK_SIZE;

BuddyAllocator::BuddyAllocator(size_t size) {
    // Redondear al siguiente poder de 2
    totalSize = 1ULL << static_cast<int>(std::ceil(std::log2(size)));
    
    // Alinear memoria a 64 bytes para mejor rendimiento SIMD
    posix_memalign(&memoryBase, 64, totalSize);
    if (!memoryBase) {
        std::cerr << "Error: no se pudo reservar memoria inicial alineada\n";
        std::exit(1);
    }

    std::memset(freeLists, 0, sizeof(freeLists));
    
    // Inicializar cache
    cacheMemory = nullptr;
    cacheSize = 0;

    int level = getLevel(totalSize);
    freeLists[level] = static_cast<Block*>(memoryBase);
    freeLists[level]->next = nullptr;
    freeLists[level]->size = totalSize;
}

BuddyAllocator::~BuddyAllocator() {
    if (cacheMemory) {
        std::free(cacheMemory);
    }
    std::free(memoryBase);
}

int BuddyAllocator::getLevel(size_t size) const {
    size = std::max(size, MIN_BLOCK_SIZE);
    size_t paddedSize = size + sizeof(Block);
    
    int level = 0;
    size_t block = MIN_BLOCK_SIZE;
    while (block < paddedSize && level < MAX_LEVELS) {
        block <<= 1;
        ++level;
    }
    return level;
}

size_t BuddyAllocator::getBlockSize(int level) const {
    return MIN_BLOCK_SIZE << level;
}

void* BuddyAllocator::alloc(size_t size) {
    // Usar caché para asignaciones temporales pequeñas
    if (size <= 4096 && cacheMemory && cacheSize >= size) {
        void* result = cacheMemory;
        cacheMemory = nullptr;
        return result;
    }
    
    int level = getLevel(size);
    size_t reqBlockSize = getBlockSize(level);

    int l = level;
    while (l < MAX_LEVELS && !freeLists[l]) ++l;

    if (l == MAX_LEVELS) {
        std::cerr << "Error: no hay bloques suficientes para " << size << " bytes\n";
        return nullptr;
    }

    while (l > level) {
        split(l);
        --l;
    }

    Block* block = freeLists[level];
    freeLists[level] = block->next;
    
    // Reservar espacio para almacenar la información del bloque
    Block* headerBlock = block;
    headerBlock->size = size;
    
    // Registrar el bloque para liberación posterior
    allocatedBlocks[block] = level;
    
    // Devolver el espacio después del encabezado
    return static_cast<void*>(static_cast<char*>(static_cast<void*>(block)) + sizeof(Block));
}

void BuddyAllocator::free(void* ptr) {
    if (!ptr) return;
    
    // Ajustar el puntero para obtener el encabezado del bloque
    void* blockPtr = static_cast<char*>(ptr) - sizeof(Block);
    
    auto it = allocatedBlocks.find(blockPtr);
    if (it == allocatedBlocks.end()) {
        std::cerr << "Error: intento de liberar un puntero no asignado\n";
        return;
    }
    
    int level = it->second;
    allocatedBlocks.erase(it);
    
    coalesce(blockPtr, level);
}

void* BuddyAllocator::realloc(void* ptr, size_t newSize) {
    if (!ptr) return alloc(newSize);
    if (newSize == 0) {
        free(ptr);
        return nullptr;
    }
    
    // Obtener el encabezado del bloque
    Block* block = reinterpret_cast<Block*>(static_cast<char*>(ptr) - sizeof(Block));
    
    // Si el nuevo tamaño es menor o igual al actual, simplemente devolver el mismo puntero
    if (newSize <= block->size) {
        return ptr;
    }
    
    // Asignar un nuevo bloque
    void* newPtr = alloc(newSize);
    if (!newPtr) return nullptr;
    
    // Copiar los datos antiguos al nuevo bloque
    std::memcpy(newPtr, ptr, block->size);
    
    // Liberar el bloque antiguo
    free(ptr);
    
    return newPtr;
}

void* BuddyAllocator::getCache(size_t size) {
    if (cacheMemory && cacheSize >= size)
        return cacheMemory;
        
    if (cacheMemory)
        std::free(cacheMemory);
        
    cacheSize = size;
    cacheMemory = std::aligned_alloc(64, size);
    return cacheMemory;
}

void BuddyAllocator::releaseCache() {
    if (cacheMemory) {
        std::free(cacheMemory);
        cacheMemory = nullptr;
        cacheSize = 0;
    }
}

void* BuddyAllocator::buddyOf(void* ptr, int level) const {
    size_t blockSize = getBlockSize(level);
    size_t offsetBytes = offset(ptr);
    size_t buddyOffset = offsetBytes ^ blockSize;
    return static_cast<char*>(memoryBase) + buddyOffset;
}

size_t BuddyAllocator::offset(void* ptr) const {
    return static_cast<char*>(ptr) - static_cast<char*>(memoryBase);
}

void BuddyAllocator::split(int level) {
    Block* block = freeLists[level];
    if (!block) return;

    freeLists[level] = block->next;

    size_t size = getBlockSize(level - 1);
    Block* first = block;
    Block* second = reinterpret_cast<Block*>(reinterpret_cast<char*>(block) + size);
    first->next = nullptr;
    second->next = nullptr;
    first->size = size - sizeof(Block);
    second->size = size - sizeof(Block);

    freeLists[level - 1] = first;
    first->next = second;
}

void BuddyAllocator::coalesce(void* ptr, int level) {
    void* buddy = buddyOf(ptr, level);

    Block** curr = &freeLists[level];
    while (*curr) {
        if (*curr == buddy) {
            *curr = (*curr)->next;
            void* base = std::min(ptr, buddy);
            coalesce(base, level + 1);
            return;
        }
        curr = &((*curr)->next);
    }

    Block* block = static_cast<Block*>(ptr);
    block->next = freeLists[level];
    freeLists[level] = block;
}

bool BuddyAllocator::isAligned(void* ptr, size_t alignment) const {
    return (reinterpret_cast<uintptr_t>(ptr) % alignment) == 0;
}
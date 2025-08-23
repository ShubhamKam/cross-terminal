#pragma once

#include <atomic>
#include <bitset>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace cross_terminal {
namespace memory {

// Memory alignment utilities
template<size_t Alignment>
constexpr size_t align_up(size_t size) noexcept {
    return (size + Alignment - 1) & ~(Alignment - 1);
}

template<typename T>
constexpr size_t cache_line_align() noexcept {
    return align_up<64>(sizeof(T));
}

// High-performance memory pool for frequent allocations
template<typename T, size_t PoolSize = 1024>
class MemoryPool {
private:
    alignas(64) struct alignas(T) StorageBlock {
        char data[sizeof(T)];
    };
    
    StorageBlock storage_[PoolSize];
    std::bitset<PoolSize> used_;
    std::atomic<size_t> next_free_{0};
    std::atomic<size_t> allocated_count_{0};
    
    mutable std::shared_mutex pool_mutex_;
    
    // Find next available slot using bit scanning
    size_t find_free_slot() noexcept {
        for (size_t i = next_free_.load(); i < PoolSize; ++i) {
            if (!used_[i]) {
                next_free_.store((i + 1) % PoolSize);
                return i;
            }
        }
        
        // Wrap around search
        for (size_t i = 0; i < next_free_.load(); ++i) {
            if (!used_[i]) {
                next_free_.store((i + 1) % PoolSize);
                return i;
            }
        }
        
        return PoolSize; // Pool exhausted
    }
    
public:
    static constexpr size_t pool_size = PoolSize;
    static constexpr size_t element_size = sizeof(T);
    
    MemoryPool() noexcept = default;
    ~MemoryPool() = default;
    
    // Non-copyable, non-movable
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    MemoryPool(MemoryPool&&) = delete;
    MemoryPool& operator=(MemoryPool&&) = delete;
    
    T* allocate() noexcept {
        std::unique_lock lock(pool_mutex_);
        
        size_t slot = find_free_slot();
        if (slot >= PoolSize) {
            return nullptr; // Pool exhausted
        }
        
        used_[slot] = true;
        allocated_count_.fetch_add(1, std::memory_order_relaxed);
        
        return reinterpret_cast<T*>(&storage_[slot]);
    }
    
    void deallocate(T* ptr) noexcept {
        if (!ptr) return;
        
        std::unique_lock lock(pool_mutex_);
        
        // Validate pointer is within pool bounds
        const auto storage_begin = reinterpret_cast<uintptr_t>(&storage_[0]);
        const auto storage_end = reinterpret_cast<uintptr_t>(&storage_[PoolSize]);
        const auto ptr_addr = reinterpret_cast<uintptr_t>(ptr);
        
        if (ptr_addr < storage_begin || ptr_addr >= storage_end) {
            return; // Invalid pointer
        }
        
        size_t slot = (ptr_addr - storage_begin) / sizeof(StorageBlock);
        if (slot < PoolSize && used_[slot]) {
            used_[slot] = false;
            allocated_count_.fetch_sub(1, std::memory_order_relaxed);
            
            // Update next_free hint
            if (slot < next_free_.load()) {
                next_free_.store(slot);
            }
        }
    }
    
    // Pool statistics
    size_t allocated() const noexcept {
        return allocated_count_.load(std::memory_order_relaxed);
    }
    
    size_t available() const noexcept {
        return PoolSize - allocated();
    }
    
    double utilization() const noexcept {
        return static_cast<double>(allocated()) / PoolSize;
    }
    
    bool is_full() const noexcept {
        return allocated() >= PoolSize;
    }
    
    bool is_empty() const noexcept {
        return allocated() == 0;
    }
};

// Stack allocator for temporary allocations
template<size_t StackSize = 8192>
class StackAllocator {
private:
    alignas(64) char stack_[StackSize];
    std::atomic<size_t> top_{0};
    
public:
    static constexpr size_t stack_size = StackSize;
    
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) noexcept {
        size = align_up<alignof(std::max_align_t)>(size);
        
        size_t current_top = top_.load(std::memory_order_relaxed);
        size_t aligned_top = align_up<alignof(std::max_align_t)>(current_top);
        size_t new_top = aligned_top + size;
        
        if (new_top > StackSize) {
            return nullptr; // Stack overflow
        }
        
        // Try to update top atomically
        while (!top_.compare_exchange_weak(current_top, new_top, 
                                         std::memory_order_release,
                                         std::memory_order_relaxed)) {
            aligned_top = align_up<alignof(std::max_align_t)>(current_top);
            new_top = aligned_top + size;
            
            if (new_top > StackSize) {
                return nullptr;
            }
        }
        
        return &stack_[aligned_top];
    }
    
    void deallocate_to(void* ptr) noexcept {
        if (!ptr) return;
        
        const auto stack_begin = reinterpret_cast<uintptr_t>(&stack_[0]);
        const auto ptr_addr = reinterpret_cast<uintptr_t>(ptr);
        
        if (ptr_addr >= stack_begin && ptr_addr < stack_begin + StackSize) {
            size_t new_top = ptr_addr - stack_begin;
            top_.store(new_top, std::memory_order_release);
        }
    }
    
    void reset() noexcept {
        top_.store(0, std::memory_order_release);
    }
    
    size_t used() const noexcept {
        return top_.load(std::memory_order_acquire);
    }
    
    size_t available() const noexcept {
        return StackSize - used();
    }
};

// Memory manager with multiple allocation strategies
class MemoryManager {
private:
    // Pool for different object types
    MemoryPool<char, 4096> small_object_pool_;     // < 64 bytes
    MemoryPool<char, 2048> medium_object_pool_;    // 64-512 bytes
    MemoryPool<char, 1024> large_object_pool_;     // 512-4096 bytes
    
    // Stack allocator for temporary objects
    StackAllocator<16384> stack_allocator_;
    
    // Statistics tracking
    std::atomic<size_t> total_allocated_{0};
    std::atomic<size_t> total_deallocated_{0};
    std::atomic<size_t> peak_usage_{0};
    
    mutable std::shared_mutex stats_mutex_;
    std::unordered_map<size_t, size_t> allocation_histogram_;
    
    static MemoryManager* instance_;
    static std::once_flag init_flag_;
    
    MemoryManager() = default;
    
    void update_statistics(size_t size, bool allocating) noexcept {
        if (allocating) {
            size_t current = total_allocated_.fetch_add(size, std::memory_order_relaxed);
            size_t current_peak = peak_usage_.load(std::memory_order_relaxed);
            
            while (current > current_peak && 
                   !peak_usage_.compare_exchange_weak(current_peak, current,
                                                     std::memory_order_release,
                                                     std::memory_order_relaxed)) {
                // Retry if peak was updated by another thread
            }
            
            // Update histogram (with lock for map access)
            {
                std::unique_lock lock(stats_mutex_);
                allocation_histogram_[size]++;
            }
        } else {
            total_deallocated_.fetch_add(size, std::memory_order_relaxed);
        }
    }
    
public:
    static MemoryManager& instance() {
        std::call_once(init_flag_, []() {
            instance_ = new MemoryManager();
        });
        return *instance_;
    }
    
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) noexcept {
        void* ptr = nullptr;
        
        // Choose allocation strategy based on size
        if (size <= 64) {
            ptr = small_object_pool_.allocate();
        } else if (size <= 512) {
            ptr = medium_object_pool_.allocate();
        } else if (size <= 4096) {
            ptr = large_object_pool_.allocate();
        } else {
            // Fall back to system allocator for very large allocations
            ptr = std::aligned_alloc(alignment, size);
        }
        
        if (ptr) {
            update_statistics(size, true);
        }
        
        return ptr;
    }
    
    void deallocate(void* ptr, size_t size) noexcept {
        if (!ptr) return;
        
        // Try pool deallocation first
        bool deallocated = false;
        
        if (size <= 64) {
            small_object_pool_.deallocate(static_cast<char*>(ptr));
            deallocated = true;
        } else if (size <= 512) {
            medium_object_pool_.deallocate(static_cast<char*>(ptr));
            deallocated = true;
        } else if (size <= 4096) {
            large_object_pool_.deallocate(static_cast<char*>(ptr));
            deallocated = true;
        } else {
            std::free(ptr);
            deallocated = true;
        }
        
        if (deallocated) {
            update_statistics(size, false);
        }
    }
    
    void* allocate_temp(size_t size, size_t alignment = alignof(std::max_align_t)) noexcept {
        return stack_allocator_.allocate(size, alignment);
    }
    
    void reset_temp() noexcept {
        stack_allocator_.reset();
    }
    
    // Memory statistics
    struct MemoryStats {
        size_t total_allocated;
        size_t total_deallocated;
        size_t current_usage;
        size_t peak_usage;
        double pool_utilization_small;
        double pool_utilization_medium;
        double pool_utilization_large;
        size_t stack_usage;
    };
    
    MemoryStats get_stats() const noexcept {
        return {
            .total_allocated = total_allocated_.load(std::memory_order_relaxed),
            .total_deallocated = total_deallocated_.load(std::memory_order_relaxed),
            .current_usage = total_allocated_.load(std::memory_order_relaxed) - 
                           total_deallocated_.load(std::memory_order_relaxed),
            .peak_usage = peak_usage_.load(std::memory_order_relaxed),
            .pool_utilization_small = small_object_pool_.utilization(),
            .pool_utilization_medium = medium_object_pool_.utilization(),
            .pool_utilization_large = large_object_pool_.utilization(),
            .stack_usage = stack_allocator_.used()
        };
    }
};

// Custom allocator for STL containers
template<typename T>
class PoolAllocator {
private:
    MemoryManager* manager_;
    
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    template<typename U>
    struct rebind {
        using other = PoolAllocator<U>;
    };
    
    PoolAllocator() noexcept : manager_(&MemoryManager::instance()) {}
    
    template<typename U>
    PoolAllocator(const PoolAllocator<U>& other) noexcept 
        : manager_(other.manager_) {}
    
    T* allocate(size_t n) {
        void* ptr = manager_->allocate(n * sizeof(T), alignof(T));
        if (!ptr) {
            throw std::bad_alloc();
        }
        return static_cast<T*>(ptr);
    }
    
    void deallocate(T* ptr, size_t n) noexcept {
        manager_->deallocate(ptr, n * sizeof(T));
    }
    
    template<typename U>
    bool operator==(const PoolAllocator<U>& other) const noexcept {
        return manager_ == other.manager_;
    }
    
    template<typename U>
    bool operator!=(const PoolAllocator<U>& other) const noexcept {
        return !(*this == other);
    }
};

// RAII wrapper for temporary allocations
class TempAllocatorGuard {
private:
    MemoryManager& manager_;
    
public:
    explicit TempAllocatorGuard(MemoryManager& manager = MemoryManager::instance()) 
        : manager_(manager) {}
    
    ~TempAllocatorGuard() {
        manager_.reset_temp();
    }
    
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
        return manager_.allocate_temp(size, alignment);
    }
    
    template<typename T, typename... Args>
    T* construct(Args&&... args) {
        void* ptr = allocate(sizeof(T), alignof(T));
        if (!ptr) return nullptr;
        
        return new(ptr) T(std::forward<Args>(args)...);
    }
};

// Memory-mapped file for large data sets
class MemoryMappedFile {
private:
    void* mapped_data_;
    size_t file_size_;
    int file_descriptor_;
    
public:
    explicit MemoryMappedFile(const std::string& filename);
    ~MemoryMappedFile();
    
    // Non-copyable, movable
    MemoryMappedFile(const MemoryMappedFile&) = delete;
    MemoryMappedFile& operator=(const MemoryMappedFile&) = delete;
    MemoryMappedFile(MemoryMappedFile&& other) noexcept;
    MemoryMappedFile& operator=(MemoryMappedFile&& other) noexcept;
    
    void* data() const noexcept { return mapped_data_; }
    size_t size() const noexcept { return file_size_; }
    bool is_valid() const noexcept { return mapped_data_ != nullptr; }
};

} // namespace memory
} // namespace cross_terminal
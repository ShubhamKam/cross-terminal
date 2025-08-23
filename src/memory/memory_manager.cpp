#include "memory_manager.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

namespace cross_terminal {
namespace memory {

// Static member definitions
MemoryManager* MemoryManager::instance_ = nullptr;
std::once_flag MemoryManager::init_flag_;

// Memory-mapped file implementation
MemoryMappedFile::MemoryMappedFile(const std::string& filename) 
    : mapped_data_(nullptr), file_size_(0), file_descriptor_(-1) {
    
    file_descriptor_ = open(filename.c_str(), O_RDONLY);
    if (file_descriptor_ == -1) {
        return; // Failed to open file
    }
    
    struct stat file_stats;
    if (fstat(file_descriptor_, &file_stats) == -1) {
        close(file_descriptor_);
        file_descriptor_ = -1;
        return;
    }
    
    file_size_ = static_cast<size_t>(file_stats.st_size);
    if (file_size_ == 0) {
        close(file_descriptor_);
        file_descriptor_ = -1;
        return;
    }
    
    mapped_data_ = mmap(nullptr, file_size_, PROT_READ, MAP_PRIVATE, 
                       file_descriptor_, 0);
    
    if (mapped_data_ == MAP_FAILED) {
        mapped_data_ = nullptr;
        close(file_descriptor_);
        file_descriptor_ = -1;
        return;
    }
    
    // Advise kernel about access pattern
    madvise(mapped_data_, file_size_, MADV_SEQUENTIAL);
}

MemoryMappedFile::~MemoryMappedFile() {
    if (mapped_data_) {
        munmap(mapped_data_, file_size_);
    }
    
    if (file_descriptor_ != -1) {
        close(file_descriptor_);
    }
}

MemoryMappedFile::MemoryMappedFile(MemoryMappedFile&& other) noexcept
    : mapped_data_(other.mapped_data_)
    , file_size_(other.file_size_)
    , file_descriptor_(other.file_descriptor_) {
    
    other.mapped_data_ = nullptr;
    other.file_size_ = 0;
    other.file_descriptor_ = -1;
}

MemoryMappedFile& MemoryMappedFile::operator=(MemoryMappedFile&& other) noexcept {
    if (this != &other) {
        // Clean up current state
        if (mapped_data_) {
            munmap(mapped_data_, file_size_);
        }
        if (file_descriptor_ != -1) {
            close(file_descriptor_);
        }
        
        // Move from other
        mapped_data_ = other.mapped_data_;
        file_size_ = other.file_size_;
        file_descriptor_ = other.file_descriptor_;
        
        other.mapped_data_ = nullptr;
        other.file_size_ = 0;
        other.file_descriptor_ = -1;
    }
    
    return *this;
}

} // namespace memory
} // namespace cross_terminal
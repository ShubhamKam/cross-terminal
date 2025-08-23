#import "macos_platform.h"
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <sys/sysctl.h>
#import <sys/mount.h>
#import <ifaddrs.h>
#import <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

macOSPlatform::macOSPlatform() {
    NSLog(@"macOSPlatform initialized");
}

macOSPlatform::~macOSPlatform() {
    NSLog(@"macOSPlatform destroyed");
}

SystemInfo macOSPlatform::getSystemInfo() {
    SystemInfo info;
    
    // Get macOS version
    NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
    info.osName = "macOS";
    info.osVersion = [NSString stringWithFormat:@"%ld.%ld.%ld", 
                     version.majorVersion, version.minorVersion, version.patchVersion].UTF8String;
    
    // Get architecture
    size_t size = 0;
    sysctlbyname("hw.targettype", NULL, &size, NULL, 0);
    if (size > 0) {
        char* arch = (char*)malloc(size);
        sysctlbyname("hw.targettype", arch, &size, NULL, 0);
        info.architecture = std::string(arch);
        free(arch);
    } else {
        // Fallback to CPU type
        #ifdef __x86_64__
            info.architecture = "x86_64";
        #elif __arm64__
            info.architecture = "arm64";
        #else
            info.architecture = "unknown";
        #endif
    }
    
    // Get CPU cores
    int cores = 0;
    size = sizeof(cores);
    sysctlbyname("hw.ncpu", &cores, &size, NULL, 0);
    info.cpuCores = cores;
    
    // Get memory information
    int64_t totalMemory = 0;
    size = sizeof(totalMemory);
    sysctlbyname("hw.memsize", &totalMemory, &size, NULL, 0);
    info.totalMemory = totalMemory;
    
    // Get available memory
    vm_size_t page_size;
    vm_statistics64_data_t vm_stat;
    mach_port_t mach_port = mach_host_self();
    mach_msg_type_number_t count = sizeof(vm_stat) / sizeof(natural_t);
    
    if (host_page_size(mach_port, &page_size) == KERN_SUCCESS &&
        host_statistics64(mach_port, HOST_VM_INFO, (host_info64_t)&vm_stat, &count) == KERN_SUCCESS) {
        info.availableMemory = (int64_t)(vm_stat.free_count + vm_stat.inactive_count) * (int64_t)page_size;
    } else {
        info.availableMemory = totalMemory / 2; // Fallback estimate
    }
    
    return info;
}

std::string macOSPlatform::getDeviceModel() {
    size_t size = 0;
    sysctlbyname("hw.model", NULL, &size, NULL, 0);
    
    char* model = (char*)malloc(size);
    sysctlbyname("hw.model", model, &size, NULL, 0);
    
    std::string result(model);
    free(model);
    return result;
}

bool macOSPlatform::fileExists(const std::string& path) {
    NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
    return [[NSFileManager defaultManager] fileExistsAtPath:nsPath];
}

bool macOSPlatform::createDirectory(const std::string& path) {
    NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
    NSError* error = nil;
    
    BOOL success = [[NSFileManager defaultManager] 
                   createDirectoryAtPath:nsPath 
                   withIntermediateDirectories:YES 
                   attributes:nil 
                   error:&error];
    
    if (!success) {
        NSLog(@"Failed to create directory: %@", error.localizedDescription);
    }
    
    return success;
}

std::vector<std::string> macOSPlatform::listDirectory(const std::string& path) {
    std::vector<std::string> files;
    
    NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
    NSError* error = nil;
    
    NSArray* contents = [[NSFileManager defaultManager] 
                        contentsOfDirectoryAtPath:nsPath 
                        error:&error];
    
    if (contents) {
        for (NSString* file in contents) {
            files.push_back([file UTF8String]);
        }
    } else {
        NSLog(@"Failed to list directory: %@", error.localizedDescription);
    }
    
    return files;
}

std::string macOSPlatform::getCurrentDirectory() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        return std::string(cwd);
    }
    return "/";
}

bool macOSPlatform::setCurrentDirectory(const std::string& path) {
    return chdir(path.c_str()) == 0;
}

int macOSPlatform::executeCommand(const std::string& command, std::string& output) {
    @autoreleasepool {
        NSTask* task = [[NSTask alloc] init];
        task.launchPath = @"/bin/sh";
        task.arguments = @[@"-c", [NSString stringWithUTF8String:command.c_str()]];
        
        NSPipe* pipe = [NSPipe pipe];
        task.standardOutput = pipe;
        task.standardError = pipe;
        
        [task launch];
        [task waitUntilExit];
        
        NSData* data = [[pipe fileHandleForReading] readDataToEndOfFile];
        NSString* result = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
        
        output = [result UTF8String];
        return [task terminationStatus];
    }
}

bool macOSPlatform::killProcess(int pid) {
    return kill(pid, SIGTERM) == 0;
}

std::vector<int> macOSPlatform::getRunningProcesses() {
    std::vector<int> processes;
    
    // Get list of all processes
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
    size_t size = 0;
    
    if (sysctl(mib, 4, NULL, &size, NULL, 0) == 0) {
        struct kinfo_proc* procs = (struct kinfo_proc*)malloc(size);
        if (sysctl(mib, 4, procs, &size, NULL, 0) == 0) {
            int proc_count = size / sizeof(struct kinfo_proc);
            for (int i = 0; i < proc_count; i++) {
                processes.push_back(procs[i].kp_proc.p_pid);
            }
        }
        free(procs);
    }
    
    return processes;
}

bool macOSPlatform::hasHardwareAccess() {
    // macOS requires specific entitlements for hardware access
    // For now, check if we can access system information
    int mib[2] = {CTL_HW, HW_NCPU};
    int cores;
    size_t size = sizeof(cores);
    return sysctl(mib, 2, &cores, &size, NULL, 0) == 0;
}

bool macOSPlatform::requestHardwarePermissions() {
    // In a real macOS app, this would request permissions through the system
    // For terminal app, we assume permissions are granted or not needed
    return hasHardwareAccess();
}

bool macOSPlatform::hasNetworkAccess() {
    struct ifaddrs* ifaddrs_ptr = nullptr;
    if (getifaddrs(&ifaddrs_ptr) == -1) {
        return false;
    }
    
    bool hasNetwork = false;
    for (struct ifaddrs* ifa = ifaddrs_ptr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in* addr_in = (struct sockaddr_in*)ifa->ifa_addr;
            if (addr_in->sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
                hasNetwork = true;
                break;
            }
        }
    }
    
    freeifaddrs(ifaddrs_ptr);
    return hasNetwork;
}

std::string macOSPlatform::getIPAddress() {
    struct ifaddrs* ifaddrs_ptr = nullptr;
    if (getifaddrs(&ifaddrs_ptr) == -1) {
        return "";
    }
    
    std::string ipAddress;
    for (struct ifaddrs* ifa = ifaddrs_ptr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in* addr_in = (struct sockaddr_in*)ifa->ifa_addr;
            if (addr_in->sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
                ipAddress = inet_ntoa(addr_in->sin_addr);
                break;
            }
        }
    }
    
    freeifaddrs(ifaddrs_ptr);
    return ipAddress;
}

std::vector<std::string> macOSPlatform::getNetworkInterfaces() {
    std::vector<std::string> interfaces;
    
    struct ifaddrs* ifaddrs_ptr = nullptr;
    if (getifaddrs(&ifaddrs_ptr) == -1) {
        return interfaces;
    }
    
    for (struct ifaddrs* ifa = ifaddrs_ptr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr) {
            std::string name(ifa->ifa_name);
            if (std::find(interfaces.begin(), interfaces.end(), name) == interfaces.end()) {
                interfaces.push_back(name);
            }
        }
    }
    
    freeifaddrs(ifaddrs_ptr);
    return interfaces;
}
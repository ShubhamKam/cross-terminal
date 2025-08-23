#include "android_platform.h"
#include <android/log.h>
#include <unistd.h>
#include <sys/system_properties.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>
#include <fstream>
#include <sstream>

#define LOG_TAG "CrossTerminal"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

AndroidPlatform::AndroidPlatform() {
    LOGD("AndroidPlatform initialized");
}

AndroidPlatform::~AndroidPlatform() {
    LOGD("AndroidPlatform destroyed");
}

SystemInfo AndroidPlatform::getSystemInfo() {
    SystemInfo info;
    
    // Get Android version
    char release[PROP_VALUE_MAX];
    char sdk_version[PROP_VALUE_MAX];
    __system_property_get("ro.build.version.release", release);
    __system_property_get("ro.build.version.sdk", sdk_version);
    
    info.osName = "Android";
    info.osVersion = std::string(release) + " (API " + std::string(sdk_version) + ")";
    
    // Get architecture
    char abi[PROP_VALUE_MAX];
    __system_property_get("ro.product.cpu.abi", abi);
    info.architecture = std::string(abi);
    
    // Get CPU cores
    info.cpuCores = sysconf(_SC_NPROCESSORS_CONF);
    
    // Get memory information
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        info.totalMemory = si.totalram * si.mem_unit;
        info.availableMemory = si.freeram * si.mem_unit;
    } else {
        info.totalMemory = 0;
        info.availableMemory = 0;
    }
    
    return info;
}

std::string AndroidPlatform::getDeviceModel() {
    char manufacturer[PROP_VALUE_MAX];
    char model[PROP_VALUE_MAX];
    
    __system_property_get("ro.product.manufacturer", manufacturer);
    __system_property_get("ro.product.model", model);
    
    return std::string(manufacturer) + " " + std::string(model);
}

bool AndroidPlatform::fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool AndroidPlatform::createDirectory(const std::string& path) {
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
}

std::vector<std::string> AndroidPlatform::listDirectory(const std::string& path) {
    std::vector<std::string> files;
    
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr) {
        LOGE("Failed to open directory: %s", path.c_str());
        return files;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name(entry->d_name);
        if (name != "." && name != "..") {
            files.push_back(name);
        }
    }
    
    closedir(dir);
    return files;
}

std::string AndroidPlatform::getCurrentDirectory() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        return std::string(cwd);
    }
    return "/";
}

bool AndroidPlatform::setCurrentDirectory(const std::string& path) {
    return chdir(path.c_str()) == 0;
}

int AndroidPlatform::executeCommand(const std::string& command, std::string& output) {
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        LOGE("Failed to execute command: %s", command.c_str());
        return -1;
    }
    
    char buffer[128];
    output.clear();
    
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    
    int result = pclose(pipe);
    return WEXITSTATUS(result);
}

bool AndroidPlatform::killProcess(int pid) {
    return kill(pid, SIGTERM) == 0;
}

std::vector<int> AndroidPlatform::getRunningProcesses() {
    std::vector<int> processes;
    
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) {
        return processes;
    }
    
    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        if (entry->d_type == DT_DIR) {
            char* endptr;
            long pid = strtol(entry->d_name, &endptr, 10);
            if (*endptr == '\0' && pid > 0) {
                processes.push_back(static_cast<int>(pid));
            }
        }
    }
    
    closedir(proc_dir);
    return processes;
}

bool AndroidPlatform::hasHardwareAccess() {
    // Check if we have root access or are running in privileged environment
    return (geteuid() == 0) || fileExists("/system/xbin/su") || fileExists("/system/bin/su");
}

bool AndroidPlatform::requestHardwarePermissions() {
    // In a real Android app, this would request permissions through JNI
    // For now, we'll check if we have necessary file access
    return hasHardwareAccess();
}

bool AndroidPlatform::hasNetworkAccess() {
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

std::string AndroidPlatform::getIPAddress() {
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

std::vector<std::string> AndroidPlatform::getNetworkInterfaces() {
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
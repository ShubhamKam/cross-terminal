#include <jni.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <android/log.h>

// Include cross-platform terminal core
#include "../../../../../src/core/terminal_engine.h"
#include "../../../../../src/hardware/android/android_hardware.h"
#include "../../../../../src/platform/android/android_platform.h"

#define LOG_TAG "CrossTerminal"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace CrossTerminal;

// Global state management
static std::unordered_map<jlong, std::unique_ptr<TerminalEngine>> g_engines;
static std::mutex g_engines_mutex;
static jlong g_next_handle = 1;

// Session management
struct TerminalSession {
    int sessionId;
    std::unique_ptr<TerminalSession> session;
    std::queue<std::string> outputBuffer;
    std::mutex outputMutex;
    std::condition_variable outputCondition;
    bool isActive = true;
};

static std::unordered_map<jlong, std::unordered_map<int, std::unique_ptr<TerminalSession>>> g_sessions;
static std::mutex g_sessions_mutex;

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_crossplatform_terminal_terminal_TerminalController_nativeInitialize(JNIEnv *env, jclass clazz) {
    try {
        std::lock_guard<std::mutex> lock(g_engines_mutex);
        
        // Create platform instance
        auto platform = AndroidPlatform::create();
        
        // Create hardware controller
        auto hardware = std::make_unique<AndroidHardware>(platform.get());
        
        // Create terminal engine
        auto engine = std::make_unique<TerminalEngine>(
            std::move(platform),
            std::move(hardware)
        );
        
        if (!engine->initialize()) {
            LOGE("Failed to initialize terminal engine");
            return 0;
        }
        
        jlong handle = g_next_handle++;
        g_engines[handle] = std::move(engine);
        
        LOGD("Terminal engine initialized with handle: %lld", handle);
        return handle;
        
    } catch (const std::exception& e) {
        LOGE("Exception in nativeInitialize: %s", e.what());
        return 0;
    }
}

JNIEXPORT void JNICALL
Java_com_crossplatform_terminal_terminal_TerminalController_nativeDestroy(JNIEnv *env, jclass clazz, jlong handle) {
    try {
        std::lock_guard<std::mutex> lock(g_engines_mutex);
        
        auto it = g_engines.find(handle);
        if (it != g_engines.end()) {
            it->second->cleanup();
            g_engines.erase(it);
            LOGD("Terminal engine destroyed: %lld", handle);
        }
        
        // Clean up sessions
        {
            std::lock_guard<std::mutex> sessions_lock(g_sessions_mutex);
            g_sessions.erase(handle);
        }
        
    } catch (const std::exception& e) {
        LOGE("Exception in nativeDestroy: %s", e.what());
    }
}

JNIEXPORT jint JNICALL
Java_com_crossplatform_terminal_terminal_TerminalController_nativeCreateSession(JNIEnv *env, jclass clazz, jlong handle) {
    try {
        std::lock_guard<std::mutex> engines_lock(g_engines_mutex);
        
        auto engine_it = g_engines.find(handle);
        if (engine_it == g_engines.end()) {
            LOGE("Invalid engine handle: %lld", handle);
            return -1;
        }
        
        // Create new session
        static int next_session_id = 1;
        int sessionId = next_session_id++;
        
        std::lock_guard<std::mutex> sessions_lock(g_sessions_mutex);
        auto session = std::make_unique<TerminalSession>();
        session->sessionId = sessionId;
        session->isActive = true;
        
        g_sessions[handle][sessionId] = std::move(session);
        
        LOGD("Created terminal session: %d for handle: %lld", sessionId, handle);
        return sessionId;
        
    } catch (const std::exception& e) {
        LOGE("Exception in nativeCreateSession: %s", e.what());
        return -1;
    }
}

JNIEXPORT jboolean JNICALL
Java_com_crossplatform_terminal_terminal_TerminalController_nativeExecuteCommand(JNIEnv *env, jclass clazz,
                                                                                jlong handle, jint sessionId,
                                                                                jstring command) {
    try {
        std::lock_guard<std::mutex> engines_lock(g_engines_mutex);
        
        auto engine_it = g_engines.find(handle);
        if (engine_it == g_engines.end()) {
            LOGE("Invalid engine handle: %lld", handle);
            return JNI_FALSE;
        }
        
        const char* cmd_chars = env->GetStringUTFChars(command, nullptr);
        std::string cmd_str(cmd_chars);
        env->ReleaseStringUTFChars(command, cmd_chars);
        
        // Execute command through terminal engine
        std::string output;
        bool success = engine_it->second->executeCommand(cmd_str, output);
        
        // Store output in session buffer
        {
            std::lock_guard<std::mutex> sessions_lock(g_sessions_mutex);
            auto& sessions = g_sessions[handle];
            auto session_it = sessions.find(sessionId);
            
            if (session_it != sessions.end()) {
                std::lock_guard<std::mutex> output_lock(session_it->second->outputMutex);
                session_it->second->outputBuffer.push(output);
                session_it->second->outputCondition.notify_one();
            }
        }
        
        LOGD("Executed command: %s, success: %d", cmd_str.c_str(), success);
        return success ? JNI_TRUE : JNI_FALSE;
        
    } catch (const std::exception& e) {
        LOGE("Exception in nativeExecuteCommand: %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jboolean JNICALL
Java_com_crossplatform_terminal_terminal_TerminalController_nativeSendInput(JNIEnv *env, jclass clazz,
                                                                           jlong handle, jint sessionId,
                                                                           jstring input) {
    try {
        std::lock_guard<std::mutex> engines_lock(g_engines_mutex);
        
        auto engine_it = g_engines.find(handle);
        if (engine_it == g_engines.end()) {
            LOGE("Invalid engine handle: %lld", handle);
            return JNI_FALSE;
        }
        
        const char* input_chars = env->GetStringUTFChars(input, nullptr);
        std::string input_str(input_chars);
        env->ReleaseStringUTFChars(input, input_chars);
        
        // Send input to terminal engine
        bool success = engine_it->second->sendInput(input_str);
        
        LOGD("Sent input: %s, success: %d", input_str.c_str(), success);
        return success ? JNI_TRUE : JNI_FALSE;
        
    } catch (const std::exception& e) {
        LOGE("Exception in nativeSendInput: %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jstring JNICALL
Java_com_crossplatform_terminal_terminal_TerminalController_nativeGetOutput(JNIEnv *env, jclass clazz,
                                                                           jlong handle, jint sessionId) {
    try {
        std::lock_guard<std::mutex> sessions_lock(g_sessions_mutex);
        
        auto handle_it = g_sessions.find(handle);
        if (handle_it == g_sessions.end()) {
            return env->NewStringUTF("");
        }
        
        auto& sessions = handle_it->second;
        auto session_it = sessions.find(sessionId);
        
        if (session_it == sessions.end()) {
            return env->NewStringUTF("");
        }
        
        std::lock_guard<std::mutex> output_lock(session_it->second->outputMutex);
        
        std::string combined_output;
        while (!session_it->second->outputBuffer.empty()) {
            combined_output += session_it->second->outputBuffer.front();
            session_it->second->outputBuffer.pop();
        }
        
        return env->NewStringUTF(combined_output.c_str());
        
    } catch (const std::exception& e) {
        LOGE("Exception in nativeGetOutput: %s", e.what());
        return env->NewStringUTF("");
    }
}

JNIEXPORT void JNICALL
Java_com_crossplatform_terminal_terminal_TerminalController_nativeSetTerminalSize(JNIEnv *env, jclass clazz,
                                                                                 jlong handle, jint sessionId,
                                                                                 jint cols, jint rows) {
    try {
        std::lock_guard<std::mutex> engines_lock(g_engines_mutex);
        
        auto engine_it = g_engines.find(handle);
        if (engine_it != g_engines.end()) {
            engine_it->second->setTerminalSize(cols, rows);
            LOGD("Set terminal size: %dx%d for handle: %lld", cols, rows, handle);
        }
        
    } catch (const std::exception& e) {
        LOGE("Exception in nativeSetTerminalSize: %s", e.what());
    }
}

JNIEXPORT jstring JNICALL
Java_com_crossplatform_terminal_terminal_TerminalController_nativeGetSystemInfo(JNIEnv *env, jclass clazz, jlong handle) {
    try {
        std::lock_guard<std::mutex> engines_lock(g_engines_mutex);
        
        auto engine_it = g_engines.find(handle);
        if (engine_it == g_engines.end()) {
            return env->NewStringUTF("Terminal not initialized");
        }
        
        std::string sysInfo = engine_it->second->getSystemInfo();
        return env->NewStringUTF(sysInfo.c_str());
        
    } catch (const std::exception& e) {
        LOGE("Exception in nativeGetSystemInfo: %s", e.what());
        return env->NewStringUTF("Error getting system info");
    }
}

JNIEXPORT jstring JNICALL
Java_com_crossplatform_terminal_terminal_TerminalController_nativeGetHardwareInfo(JNIEnv *env, jclass clazz, jlong handle) {
    try {
        std::lock_guard<std::mutex> engines_lock(g_engines_mutex);
        
        auto engine_it = g_engines.find(handle);
        if (engine_it == g_engines.end()) {
            return env->NewStringUTF("Hardware not available");
        }
        
        std::string hwInfo = engine_it->second->getHardwareInfo();
        return env->NewStringUTF(hwInfo.c_str());
        
    } catch (const std::exception& e) {
        LOGE("Exception in nativeGetHardwareInfo: %s", e.what());
        return env->NewStringUTF("Error getting hardware info");
    }
}

} // extern "C"
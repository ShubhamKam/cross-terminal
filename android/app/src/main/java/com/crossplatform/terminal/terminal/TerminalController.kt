package com.crossplatform.terminal.terminal

import android.content.Context
import android.view.KeyEvent
import kotlinx.coroutines.*
import java.util.concurrent.atomic.AtomicBoolean

/**
 * Main controller for terminal functionality
 * 
 * Manages the interface between Android UI and native terminal engine
 * Handles command execution, output processing, and session management
 */
class TerminalController(private val context: Context) {
    
    private val isInitialized = AtomicBoolean(false)
    private val controllerScope = CoroutineScope(Dispatchers.Main + SupervisorJob())
    
    // Native terminal engine handle
    private var nativeHandle: Long = 0L
    
    // Session management
    private var currentSessionId = 0
    private val sessions = mutableMapOf<Int, TerminalSession>()
    
    // Output handling
    private var outputCallback: ((String) -> Unit)? = null
    
    companion object {
        // Native method declarations
        @JvmStatic
        external fun nativeInitialize(): Long
        
        @JvmStatic
        external fun nativeDestroy(handle: Long)
        
        @JvmStatic
        external fun nativeCreateSession(handle: Long): Int
        
        @JvmStatic
        external fun nativeExecuteCommand(handle: Long, sessionId: Int, command: String): Boolean
        
        @JvmStatic
        external fun nativeSendInput(handle: Long, sessionId: Int, input: String): Boolean
        
        @JvmStatic
        external fun nativeGetOutput(handle: Long, sessionId: Int): String
        
        @JvmStatic
        external fun nativeSetTerminalSize(handle: Long, sessionId: Int, cols: Int, rows: Int)
        
        @JvmStatic
        external fun nativeGetSystemInfo(handle: Long): String
        
        @JvmStatic
        external fun nativeGetHardwareInfo(handle: Long): String
        
        // Native library loading disabled for initial build
        // init {
        //     System.loadLibrary("cross-terminal")
        // }
    }
    
    data class TerminalSession(
        val id: Int,
        var isActive: Boolean = true,
        var workingDirectory: String = "/",
        var environmentVars: MutableMap<String, String> = mutableMapOf()
    )
    
    /**
     * Initialize the terminal controller and native engine
     */
    suspend fun initialize(): Boolean = withContext(Dispatchers.IO) {
        if (isInitialized.get()) return@withContext true
        
        try {
            // Initialize native terminal engine
            nativeHandle = nativeInitialize()
            
            if (nativeHandle == 0L) {
                throw RuntimeException("Failed to initialize native terminal engine")
            }
            
            // Create default session
            val sessionId = nativeCreateSession(nativeHandle)
            if (sessionId < 0) {
                throw RuntimeException("Failed to create default terminal session")
            }
            
            currentSessionId = sessionId
            sessions[sessionId] = TerminalSession(sessionId)
            
            isInitialized.set(true)
            
            // Start output monitoring
            startOutputMonitoring()
            
            // Send welcome message
            writeToTerminal("Cross-Terminal v1.0.0-alpha\nAndroid Hardware-Enabled Terminal\n\n")
            
            true
        } catch (e: Exception) {
            e.printStackTrace()
            false
        }
    }
    
    /**
     * Create a new terminal session
     */
    fun createNewSession(): Int {
        if (!isInitialized.get()) return -1
        
        val sessionId = nativeCreateSession(nativeHandle)
        if (sessionId >= 0) {
            sessions[sessionId] = TerminalSession(sessionId)
        }
        
        return sessionId
    }
    
    /**
     * Switch to a different session
     */
    fun switchToSession(sessionId: Int): Boolean {
        return if (sessions.containsKey(sessionId)) {
            currentSessionId = sessionId
            true
        } else {
            false
        }
    }
    
    /**
     * Execute a command in the current session
     */
    fun executeCommand(command: String): Boolean {
        if (!isInitialized.get()) return false
        
        return nativeExecuteCommand(nativeHandle, currentSessionId, command)
    }
    
    /**
     * Send input to the current session
     */
    fun sendInput(input: String): Boolean {
        if (!isInitialized.get()) return false
        
        return nativeSendInput(nativeHandle, currentSessionId, input)
    }
    
    /**
     * Handle key input events from the terminal view
     */
    fun handleKeyInput(keyCode: Int, event: KeyEvent): Boolean {
        if (!isInitialized.get()) return false
        
        val input = when (keyCode) {
            KeyEvent.KEYCODE_ENTER -> "\n"
            KeyEvent.KEYCODE_TAB -> "\t"
            KeyEvent.KEYCODE_DEL -> "\b"
            KeyEvent.KEYCODE_ESCAPE -> "\u001B"
            KeyEvent.KEYCODE_DPAD_UP -> "\u001B[A"
            KeyEvent.KEYCODE_DPAD_DOWN -> "\u001B[B"
            KeyEvent.KEYCODE_DPAD_RIGHT -> "\u001B[C"
            KeyEvent.KEYCODE_DPAD_LEFT -> "\u001B[D"
            KeyEvent.KEYCODE_HOME -> "\u001B[H"
            KeyEvent.KEYCODE_MOVE_END -> "\u001B[F"
            KeyEvent.KEYCODE_PAGE_UP -> "\u001B[5~"
            KeyEvent.KEYCODE_PAGE_DOWN -> "\u001B[6~"
            else -> {
                // Handle character input
                val unicodeChar = event.unicodeChar
                if (unicodeChar != 0) {
                    unicodeChar.toChar().toString()
                } else {
                    null
                }
            }
        }
        
        return if (input != null) {
            sendInput(input)
        } else {
            false
        }
    }
    
    /**
     * Handle back button press
     */
    fun handleBackPressed(): Boolean {
        // Send Ctrl+C to interrupt current command
        return sendInput("\u0003")
    }
    
    /**
     * Write text directly to terminal output
     */
    fun writeToTerminal(text: String) {
        outputCallback?.invoke(text)
    }
    
    /**
     * Set terminal size
     */
    fun onTerminalSizeChanged(cols: Int, rows: Int) {
        if (isInitialized.get()) {
            nativeSetTerminalSize(nativeHandle, currentSessionId, cols, rows)
        }
    }
    
    /**
     * Set output callback for terminal view updates
     */
    fun setOutputCallback(callback: (String) -> Unit) {
        outputCallback = callback
    }
    
    /**
     * Get system information
     */
    fun getSystemInfo(): String {
        return if (isInitialized.get()) {
            nativeGetSystemInfo(nativeHandle)
        } else {
            "Terminal not initialized"
        }
    }
    
    /**
     * Get hardware information
     */
    fun getHardwareInfo(): String {
        return if (isInitialized.get()) {
            nativeGetHardwareInfo(nativeHandle)
        } else {
            "Hardware not available"
        }
    }
    
    /**
     * Increase font size
     */
    fun increaseFontSize() {
        // This will be handled by the TerminalView
        outputCallback?.invoke("\u001B[font-size-increase]")
    }
    
    /**
     * Decrease font size
     */
    fun decreaseFontSize() {
        // This will be handled by the TerminalView
        outputCallback?.invoke("\u001B[font-size-decrease]")
    }
    
    /**
     * Start monitoring output from native terminal
     */
    private fun startOutputMonitoring() {
        controllerScope.launch {
            while (isInitialized.get()) {
                try {
                    val output = withContext(Dispatchers.IO) {
                        nativeGetOutput(nativeHandle, currentSessionId)
                    }
                    
                    if (output.isNotEmpty()) {
                        outputCallback?.invoke(output)
                    }
                    
                    delay(50) // Check for output every 50ms
                } catch (e: Exception) {
                    e.printStackTrace()
                    delay(1000) // Wait longer on error
                }
            }
        }
    }
    
    /**
     * Activity lifecycle callbacks
     */
    fun onResume() {
        // Resume terminal operations if needed
    }
    
    fun onPause() {
        // Pause terminal operations to save battery
    }
    
    /**
     * Cleanup resources
     */
    fun cleanup() {
        if (isInitialized.getAndSet(false)) {
            controllerScope.cancel()
            
            // Close all sessions
            sessions.clear()
            
            // Destroy native handle
            if (nativeHandle != 0L) {
                nativeDestroy(nativeHandle)
                nativeHandle = 0L
            }
        }
    }
    
    /**
     * Execute built-in terminal commands
     */
    private fun executeBuiltinCommand(command: String): Boolean {
        val parts = command.trim().split(" ")
        val cmd = parts[0].lowercase()
        
        return when (cmd) {
            "clear" -> {
                outputCallback?.invoke("\u001B[2J\u001B[H")
                true
            }
            "exit" -> {
                // Handle terminal exit
                cleanup()
                true
            }
            "help" -> {
                val helpText = """
                |Cross-Terminal Commands:
                |  clear          - Clear the terminal screen
                |  exit           - Exit the terminal
                |  sysinfo        - Show system information
                |  hwinfo         - Show hardware information
                |  gpio <pin> <state> - Control GPIO pin (high/low)
                |  sensor <type>  - Read sensor data
                |  help           - Show this help message
                |
                """.trimMargin()
                outputCallback?.invoke(helpText)
                true
            }
            "sysinfo" -> {
                outputCallback?.invoke(getSystemInfo() + "\n")
                true
            }
            "hwinfo" -> {
                outputCallback?.invoke(getHardwareInfo() + "\n")
                true
            }
            else -> false
        }
    }
}
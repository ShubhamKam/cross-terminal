package com.crossplatform.terminal

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import android.view.KeyEvent
import android.view.View
import android.view.WindowManager
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.lifecycle.lifecycleScope
import com.crossplatform.terminal.databinding.ActivityMainBinding
import com.crossplatform.terminal.terminal.TerminalController
import com.crossplatform.terminal.terminal.TerminalView
import com.crossplatform.terminal.hardware.HardwareManager
import kotlinx.coroutines.launch

/**
 * Main activity for Cross-Terminal Android application
 * 
 * Features:
 * - Full-screen terminal interface
 * - Hardware control integration
 * - Performance monitoring
 * - Native C++ terminal engine via JNI
 */
class MainActivity : AppCompatActivity() {
    
    private lateinit var binding: ActivityMainBinding
    private lateinit var terminalController: TerminalController
    private lateinit var hardwareManager: HardwareManager
    private var isFullScreen = true
    
    companion object {
        private const val PERMISSION_REQUEST_CODE = 1001
        
        // Required permissions for full terminal functionality
        private val REQUIRED_PERMISSIONS = arrayOf(
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.INTERNET,
            Manifest.permission.ACCESS_NETWORK_STATE,
            Manifest.permission.ACCESS_WIFI_STATE,
            Manifest.permission.CHANGE_WIFI_STATE,
            Manifest.permission.BLUETOOTH,
            Manifest.permission.BLUETOOTH_ADMIN,
            Manifest.permission.ACCESS_COARSE_LOCATION,
            Manifest.permission.ACCESS_FINE_LOCATION,
            Manifest.permission.CAMERA,
            Manifest.permission.RECORD_AUDIO,
            Manifest.permission.MODIFY_AUDIO_SETTINGS,
            Manifest.permission.WAKE_LOCK
        )
        
        // Load native library
        init {
            System.loadLibrary("cross-terminal")
        }
    }
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // Enable full-screen mode
        enableFullScreen()
        
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        
        // Initialize components
        initializeTerminal()
        initializeHardwareManager()
        setupUI()
        
        // Request necessary permissions
        requestPermissions()
        
        // Initialize terminal engine
        lifecycleScope.launch {
            terminalController.initialize()
        }
    }
    
    private fun enableFullScreen() {
        if (isFullScreen) {
            window.setFlags(
                WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN
            )
            
            window.decorView.systemUiVisibility = (
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                or View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_FULLSCREEN
            )
        }
    }
    
    private fun initializeTerminal() {
        terminalController = TerminalController(this)
        
        // Configure terminal view
        binding.terminalView.apply {
            terminalController = this@MainActivity.terminalController
            setBackgroundColor(ContextCompat.getColor(context, R.color.terminal_background))
            
            // Enable hardware acceleration for better performance
            setLayerType(View.LAYER_TYPE_HARDWARE, null)
        }
    }
    
    private fun initializeHardwareManager() {
        hardwareManager = HardwareManager(this)
        
        // Set up hardware monitoring
        hardwareManager.setDataUpdateCallback { data ->
            runOnUiThread {
                updateHardwareDisplay(data)
            }
        }
    }
    
    private fun setupUI() {
        // Terminal view setup
        binding.terminalView.apply {
            isFocusable = true
            isFocusableInTouchMode = true
            requestFocus()
            
            setOnKeyListener { _, keyCode, event ->
                handleKeyEvent(keyCode, event)
            }
        }
        
        // Toolbar setup
        binding.toolbar.apply {
            setNavigationOnClickListener {
                toggleFullScreen()
            }
            
            inflateMenu(R.menu.terminal_menu)
            setOnMenuItemClickListener { menuItem ->
                when (menuItem.itemId) {
                    R.id.action_new_tab -> {
                        terminalController.createNewSession()
                        true
                    }
                    R.id.action_settings -> {
                        // TODO: Open settings activity
                        true
                    }
                    R.id.action_hardware -> {
                        toggleHardwarePanel()
                        true
                    }
                    else -> false
                }
            }
        }
        
        // Hardware panel setup
        setupHardwarePanel()
        
        // Status bar setup
        setupStatusBar()
    }
    
    private fun setupHardwarePanel() {
        binding.hardwarePanel.apply {
            // CPU monitoring
            binding.cpuUsageText.text = "CPU: 0%"
            
            // Memory monitoring
            binding.memoryUsageText.text = "RAM: 0%"
            
            // Temperature monitoring
            binding.temperatureText.text = "Temp: --°C"
            
            // GPIO controls (always available, requires root)
            binding.gpioControlsLayout.visibility = View.VISIBLE
            setupGPIOControls()
            
            // Sensor monitoring
            setupSensorMonitoring()
        }
    }
    
    private fun setupGPIOControls() {
        // Example GPIO controls for common pins
        binding.gpio18Switch.setOnCheckedChangeListener { _, isChecked ->
            hardwareManager.setGpioPin(18, isChecked)
        }
        
        binding.gpio19Switch.setOnCheckedChangeListener { _, isChecked ->
            hardwareManager.setGpioPin(19, isChecked)
        }
        
        // Read GPIO states
        binding.readGpioButton.setOnClickListener {
            val pin18State = hardwareManager.readGpioPin(18)
            val pin19State = hardwareManager.readGpioPin(19)
            
            val message = "GPIO States:\nPin 18: ${if (pin18State == true) "HIGH" else if (pin18State == false) "LOW" else "ERROR"}\nPin 19: ${if (pin19State == true) "HIGH" else if (pin19State == false) "LOW" else "ERROR"}"
            terminalController.writeToTerminal(message)
        }
    }
    
    private fun setupSensorMonitoring() {
        binding.startSensorsButton.setOnClickListener {
            hardwareManager.startMonitoring()
            terminalController.writeToTerminal("Hardware monitoring started\n")
        }
        
        binding.stopSensorsButton.setOnClickListener {
            hardwareManager.stopMonitoring()
            terminalController.writeToTerminal("Hardware monitoring stopped\n")
        }
    }
    
    private fun setupStatusBar() {
        binding.statusBar.apply {
            binding.connectionStatus.text = "Connected"
            binding.sessionCount.text = "1 session"
        }
    }
    
    private fun handleKeyEvent(keyCode: Int, event: KeyEvent): Boolean {
        if (event.action == KeyEvent.ACTION_DOWN) {
            return terminalController.handleKeyInput(keyCode, event)
        }
        return false
    }
    
    private fun toggleFullScreen() {
        isFullScreen = !isFullScreen
        if (isFullScreen) {
            binding.toolbar.visibility = View.GONE
            binding.statusBar.visibility = View.GONE
            enableFullScreen()
        } else {
            binding.toolbar.visibility = View.VISIBLE
            binding.statusBar.visibility = View.VISIBLE
            window.decorView.systemUiVisibility = View.SYSTEM_UI_FLAG_VISIBLE
            window.clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN)
        }
    }
    
    private fun toggleHardwarePanel() {
        binding.hardwarePanel.apply {
            visibility = if (visibility == View.VISIBLE) View.GONE else View.VISIBLE
        }
    }
    
    private fun updateHardwareDisplay(data: HardwareManager.HardwareData) {
        binding.cpuUsageText.text = "CPU: ${String.format("%.1f", data.cpuUsage)}%"
        binding.memoryUsageText.text = "RAM: ${String.format("%.1f", data.memoryUsage)}%"
        
        val tempText = if (data.ambientTemp.isNaN()) "--" else String.format("%.1f", data.ambientTemp)
        binding.temperatureText.text = "Temp: ${tempText}°C"
        
        // Update battery info
        binding.batteryLevel.text = "Battery: ${data.batteryLevel}%"
        binding.chargingStatus.text = if (data.isCharging) "Charging" else "Discharging"
        
        // Update sensor displays
        binding.accelerometerText.text = String.format(
            "Accel: X=%.2f Y=%.2f Z=%.2f",
            data.accelerometer[0], data.accelerometer[1], data.accelerometer[2]
        )
        
        binding.gyroscopeText.text = String.format(
            "Gyro: X=%.2f Y=%.2f Z=%.2f",
            data.gyroscope[0], data.gyroscope[1], data.gyroscope[2]
        )
        
        val ambientText = if (data.ambientTemp.isNaN()) "--" else String.format("%.1f", data.ambientTemp)
        binding.ambientTempText.text = "Ambient: ${ambientText}°C"
    }
    
    
    private fun requestPermissions() {
        val missingPermissions = REQUIRED_PERMISSIONS.filter {
            ContextCompat.checkSelfPermission(this, it) != PackageManager.PERMISSION_GRANTED
        }
        
        if (missingPermissions.isNotEmpty()) {
            ActivityCompat.requestPermissions(
                this,
                missingPermissions.toTypedArray(),
                PERMISSION_REQUEST_CODE
            )
        }
    }
    
    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        
        if (requestCode == PERMISSION_REQUEST_CODE) {
            val deniedPermissions = permissions.filterIndexed { index, _ ->
                grantResults[index] != PackageManager.PERMISSION_GRANTED
            }
            
            if (deniedPermissions.isNotEmpty()) {
                // Show warning about limited functionality
                terminalController.writeToTerminal(
                    "Warning: Some permissions denied. Hardware features may be limited.\n"
                )
            } else {
                // Start hardware monitoring
                hardwareManager.startMonitoring()
                terminalController.writeToTerminal(
                    "Cross-Terminal v1.0.0-alpha\nAll permissions granted. Hardware features enabled.\n\n"
                )
            }
        }
    }
    
    override fun onResume() {
        super.onResume()
        terminalController.onResume()
    }
    
    override fun onPause() {
        super.onPause()
        terminalController.onPause()
    }
    
    override fun onDestroy() {
        super.onDestroy()
        terminalController.cleanup()
        hardwareManager.cleanup()
    }
    
    override fun onBackPressed() {
        if (!terminalController.handleBackPressed()) {
            super.onBackPressed()
        }
    }
    
    // Handle volume keys for terminal control
    override fun onKeyDown(keyCode: Int, event: KeyEvent?): Boolean {
        when (keyCode) {
            KeyEvent.KEYCODE_VOLUME_UP -> {
                terminalController.increaseFontSize()
                return true
            }
            KeyEvent.KEYCODE_VOLUME_DOWN -> {
                terminalController.decreaseFontSize()
                return true
            }
        }
        return super.onKeyDown(keyCode, event)
    }
}
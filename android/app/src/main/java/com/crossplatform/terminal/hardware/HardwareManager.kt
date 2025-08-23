package com.crossplatform.terminal.hardware

import android.content.Context
import android.content.IntentFilter
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.os.BatteryManager
import kotlinx.coroutines.*
import java.io.File
import kotlin.math.roundToInt

/**
 * Hardware manager for Android sensor and system monitoring
 * 
 * Provides unified access to device sensors, system metrics, and GPIO
 * Features real-time monitoring and data collection capabilities
 */
class HardwareManager(private val context: Context) : SensorEventListener {
    
    private val sensorManager = context.getSystemService(Context.SENSOR_SERVICE) as SensorManager
    private val hardwareScope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    
    // Sensor data
    private var accelerometerData = FloatArray(3) { 0f }
    private var gyroscopeData = FloatArray(3) { 0f }
    private var ambientTemperature = Float.NaN
    
    // System monitoring
    private var cpuUsage = 0f
    private var memoryUsage = 0f
    private var batteryLevel = 0
    private var isCharging = false
    
    // Callbacks
    private var onDataUpdateCallback: ((HardwareData) -> Unit)? = null
    
    data class HardwareData(
        val accelerometer: FloatArray,
        val gyroscope: FloatArray,
        val ambientTemp: Float,
        val cpuUsage: Float,
        val memoryUsage: Float,
        val batteryLevel: Int,
        val isCharging: Boolean,
        val timestamp: Long = System.currentTimeMillis()
    )
    
    fun startMonitoring() {
        // Register sensor listeners
        sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER)?.let {
            sensorManager.registerListener(this, it, SensorManager.SENSOR_DELAY_NORMAL)
        }
        
        sensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE)?.let {
            sensorManager.registerListener(this, it, SensorManager.SENSOR_DELAY_NORMAL)
        }
        
        sensorManager.getDefaultSensor(Sensor.TYPE_AMBIENT_TEMPERATURE)?.let {
            sensorManager.registerListener(this, it, SensorManager.SENSOR_DELAY_NORMAL)
        }
        
        // Start system monitoring coroutine
        hardwareScope.launch {
            while (isActive) {
                updateSystemMetrics()
                updateBatteryInfo()
                notifyDataUpdate()
                delay(1000) // Update every second
            }
        }
    }
    
    fun stopMonitoring() {
        sensorManager.unregisterListener(this)
        hardwareScope.cancel()
    }
    
    fun setDataUpdateCallback(callback: (HardwareData) -> Unit) {
        onDataUpdateCallback = callback
    }
    
    override fun onSensorChanged(event: SensorEvent) {
        when (event.sensor.type) {
            Sensor.TYPE_ACCELEROMETER -> {
                accelerometerData = event.values.clone()
            }
            Sensor.TYPE_GYROSCOPE -> {
                gyroscopeData = event.values.clone()
            }
            Sensor.TYPE_AMBIENT_TEMPERATURE -> {
                ambientTemperature = event.values[0]
            }
        }
    }
    
    override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) {
        // Handle accuracy changes if needed
    }
    
    private suspend fun updateSystemMetrics() = withContext(Dispatchers.IO) {
        try {
            // CPU usage calculation
            val cpuInfo = File("/proc/stat").readText()
            cpuUsage = parseCpuUsage(cpuInfo)
            
            // Memory usage calculation
            val memInfo = File("/proc/meminfo").readText()
            memoryUsage = parseMemoryUsage(memInfo)
        } catch (e: Exception) {
            // Handle file read errors
            cpuUsage = 0f
            memoryUsage = 0f
        }
    }
    
    private fun parseCpuUsage(cpuInfo: String): Float {
        try {
            val lines = cpuInfo.split("\n")
            val cpuLine = lines.first { it.startsWith("cpu ") }
            val values = cpuLine.split("\\s+".toRegex()).drop(1).map { it.toLong() }
            
            val idle = values[3]
            val total = values.sum()
            
            return ((total - idle).toFloat() / total * 100).coerceIn(0f, 100f)
        } catch (e: Exception) {
            return 0f
        }
    }
    
    private fun parseMemoryUsage(memInfo: String): Float {
        try {
            val lines = memInfo.split("\n")
            val memTotal = lines.first { it.startsWith("MemTotal:") }
                .split("\\s+".toRegex())[1].toLong()
            val memAvailable = lines.first { it.startsWith("MemAvailable:") }
                .split("\\s+".toRegex())[1].toLong()
            
            val used = memTotal - memAvailable
            return (used.toFloat() / memTotal * 100).coerceIn(0f, 100f)
        } catch (e: Exception) {
            return 0f
        }
    }
    
    private fun updateBatteryInfo() {
        try {
            val batteryManager = context.getSystemService(Context.BATTERY_SERVICE) as BatteryManager
            batteryLevel = batteryManager.getIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY)
            
            val intentFilter = IntentFilter(android.content.Intent.ACTION_BATTERY_CHANGED)
            val batteryStatus = context.registerReceiver(null, intentFilter)
            
            val status = batteryStatus?.getIntExtra(BatteryManager.EXTRA_STATUS, -1) ?: -1
            isCharging = status == BatteryManager.BATTERY_STATUS_CHARGING ||
                        status == BatteryManager.BATTERY_STATUS_FULL
        } catch (e: Exception) {
            batteryLevel = 0
            isCharging = false
        }
    }
    
    private fun notifyDataUpdate() {
        val data = HardwareData(
            accelerometer = accelerometerData.clone(),
            gyroscope = gyroscopeData.clone(),
            ambientTemp = ambientTemperature,
            cpuUsage = cpuUsage,
            memoryUsage = memoryUsage,
            batteryLevel = batteryLevel,
            isCharging = isCharging
        )
        
        onDataUpdateCallback?.invoke(data)
    }
    
    // GPIO Control Methods (requires root access)
    fun setGpioPin(pin: Int, state: Boolean): Boolean {
        return try {
            val value = if (state) "1" else "0"
            val process = Runtime.getRuntime().exec(arrayOf("su", "-c", "echo $value > /sys/class/gpio/gpio$pin/value"))
            process.waitFor() == 0
        } catch (e: Exception) {
            false
        }
    }
    
    fun readGpioPin(pin: Int): Boolean? {
        return try {
            val process = Runtime.getRuntime().exec(arrayOf("su", "-c", "cat /sys/class/gpio/gpio$pin/value"))
            val result = process.inputStream.bufferedReader().readText().trim()
            process.waitFor()
            result == "1"
        } catch (e: Exception) {
            null
        }
    }
    
    fun exportGpioPin(pin: Int): Boolean {
        return try {
            val process = Runtime.getRuntime().exec(arrayOf("su", "-c", "echo $pin > /sys/class/gpio/export"))
            process.waitFor() == 0
        } catch (e: Exception) {
            false
        }
    }
    
    fun setGpioPinDirection(pin: Int, isOutput: Boolean): Boolean {
        return try {
            val direction = if (isOutput) "out" else "in"
            val process = Runtime.getRuntime().exec(arrayOf("su", "-c", "echo $direction > /sys/class/gpio/gpio$pin/direction"))
            process.waitFor() == 0
        } catch (e: Exception) {
            false
        }
    }
    
    // System Information
    fun getSystemInfo(): String {
        return buildString {
            appendLine("=== Cross-Terminal System Info ===")
            appendLine("Android Version: ${android.os.Build.VERSION.RELEASE}")
            appendLine("API Level: ${android.os.Build.VERSION.SDK_INT}")
            appendLine("Device: ${android.os.Build.DEVICE}")
            appendLine("Model: ${android.os.Build.MODEL}")
            appendLine("Manufacturer: ${android.os.Build.MANUFACTURER}")
            appendLine("Hardware: ${android.os.Build.HARDWARE}")
            appendLine("CPU Architecture: ${System.getProperty("os.arch")}")
            appendLine("Available Processors: ${Runtime.getRuntime().availableProcessors()}")
            appendLine("Total Memory: ${Runtime.getRuntime().totalMemory() / 1024 / 1024} MB")
            appendLine("Free Memory: ${Runtime.getRuntime().freeMemory() / 1024 / 1024} MB")
            appendLine("CPU Usage: ${cpuUsage.roundToInt()}%")
            appendLine("Memory Usage: ${memoryUsage.roundToInt()}%")
            appendLine("Battery Level: $batteryLevel%")
            appendLine("Charging: ${if (isCharging) "Yes" else "No"}")
            
            // Available sensors
            appendLine("\n=== Available Sensors ===")
            sensorManager.getSensorList(Sensor.TYPE_ALL).forEach { sensor ->
                appendLine("${sensor.name}: ${sensor.type}")
            }
        }
    }
    
    fun cleanup() {
        stopMonitoring()
    }
}
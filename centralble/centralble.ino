#include <ArduinoBLE.h>

// Change the advertised local name to match your sensor/ML peripheral.
// Modified to match the peripheral's local name ("PredictionPeripheral")
const char* peripheralName = "PredictionPeripheral";

// Define the service and characteristic UUIDs used by the peripheral.
#define SERVICE_UUID        "19b10000-e8f2-537e-4f6c-d104768a1214"
#define CHARACTERISTIC_UUID "19b10001-e8f2-537e-4f6c-d104768a1214"

BLEDevice peripheralDevice;
BLECharacteristic sensorCharacteristic;

void setup() {
  Serial.begin(9600);
  while (!Serial); // wait for Serial Monitor

  // Initialize BLE hardware
  if (!BLE.begin()) {
    Serial.println("BLE initialization failed!");
    while (1);
  }
  
  Serial.println("BLE Central - Sensor data forwarding to dashboard");
  
  // Start scanning for peripherals advertising the desired UUID.
  BLE.scanForUuid(SERVICE_UUID);
}

void loop() {
  // Look for a BLE peripheral.
  BLEDevice discoveredPeripheral = BLE.available();

  if (discoveredPeripheral) {
    // Check if the discovered peripheral has the expected local name.
    if (discoveredPeripheral.hasLocalName() && 
        (strcmp(discoveredPeripheral.localName().c_str(), peripheralName) == 0)) {
      
      Serial.print("Found peripheral: ");
      Serial.println(discoveredPeripheral.localName());
      
      // Stop scanning while we connect.
      BLE.stopScan();

      // Connect to the peripheral.
      if (discoveredPeripheral.connect()) {
        Serial.println("Connected to peripheral");
      } else {
        Serial.println("Failed to connect!");
        // Continue scanning if connection fails.
        BLE.scanForUuid(SERVICE_UUID);
        return;
      }
      
      // Discover peripheral attributes.
      if (discoveredPeripheral.discoverAttributes()) {
        Serial.println("Discovered peripheral attributes");
      } else {
        Serial.println("Attribute discovery failed!");
        discoveredPeripheral.disconnect();
        BLE.scanForUuid(SERVICE_UUID);
        return;
      }
      
      // Get the sensor data characteristic.
      BLEService sensorService = discoveredPeripheral.service(SERVICE_UUID);
      sensorCharacteristic = sensorService.characteristic(CHARACTERISTIC_UUID);
      
      if (!sensorCharacteristic) {
        Serial.println("Sensor characteristic not found!");
        discoveredPeripheral.disconnect();
        BLE.scanForUuid(SERVICE_UUID);
        return;
      }
      
      // Subscribe to notifications from the sensor characteristic.
      if (!sensorCharacteristic.subscribe()) {
        Serial.println("Sensor characteristic subscription failed!");
        discoveredPeripheral.disconnect();
        BLE.scanForUuid(SERVICE_UUID);
        return;
      }
      
      Serial.println("Subscribed to sensor data notifications. Forwarding data to dashboard...");

      // Continuously listen for data until the peripheral disconnects.
      while (discoveredPeripheral.connected()) {
        if (sensorCharacteristic.valueUpdated()) {
          // Get the length of the received data.
          int len = sensorCharacteristic.valueLength();
          // Create a temporary buffer with an extra byte for the terminating null.
          char buffer[len + 1];
          // Copy the data into the buffer.
          memcpy(buffer, sensorCharacteristic.value(), len);
          // Null-terminate the string.
          buffer[len] = '\0';
          // Create a String from the buffer.
          String sensorData = String(buffer);
          
          // Forward the sensor data to the dashboard via Serial.
          Serial.print("Received sensor data: ");
          Serial.println(sensorData);
        }
        // Short delay to prevent overwhelming the Serial output.
        delay(100);
      }
      
      Serial.println("Peripheral disconnected. Restarting scan.");
      // Restart scanning for the desired peripheral.
      BLE.scanForUuid(SERVICE_UUID);
    }
  }
}

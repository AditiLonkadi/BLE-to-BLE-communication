#include <ArduinoBLE.h>
#include <math.h>

// BLE service and characteristic UUIDs (using the same as in your LED example)
BLEService predictionService("19B10000-E8F2-537E-4F6C-D104768A1214");
// Create a BLE characteristic for the ML prediction data as a string (with read and notify properties).
// The 20 indicates the maximum length of the string.
BLEStringCharacteristic predictionCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify, 20);

void setup() {
  Serial.begin(9600);
  while (!Serial); // wait for Serial Monitor to open

  // Initialize BLE module
  if (!BLE.begin()) {
    Serial.println("Starting Bluetooth® Low Energy module failed!");
    while (1);
  }
  // Set local name and advertise the service that contains the prediction characteristic
  BLE.setLocalName("PredictionPeripheral");
  BLE.setAdvertisedService(predictionService);
  // Add the characteristic to the service and add the service
  predictionService.addCharacteristic(predictionCharacteristic);
  BLE.addService(predictionService);
  // Write an initial value to the characteristic
  predictionCharacteristic.writeValue("0");
  // Start advertising BLE
  BLE.advertise();
  Serial.println("BLE Prediction Peripheral");
}

void loop() {
  // --- Begin ML Prediction Block ---
  // Replace this block with your actual ML prediction calculation.
  // Here we simulate a prediction using a simple sine wave based on elapsed time.
  float mlPrediction = sin(millis() / 1000.0);
  // --- End ML Prediction Block ---

  // Convert the float prediction to a string (max 20 characters) using snprintf
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "%.2f", mlPrediction);

  // Always print the predicted value to the Serial Monitor
  Serial.print("Predicted value: ");
  Serial.println(buffer);

  // Check for a connected central device
  BLEDevice central = BLE.central();
  if (central && central.connected()) {
    // Optionally print connection info once
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    // Write the prediction to the BLE characteristic (this will notify the connected central)
    predictionCharacteristic.writeValue(buffer);
    Serial.print("Sent predicted data: ");
    Serial.println(buffer);
  }
  // Delay between predictions (adjust based on your application needs)
  delay(500);
}

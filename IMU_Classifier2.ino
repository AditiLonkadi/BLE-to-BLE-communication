/*
  BMI270 Gyroscope LSTM Anomaly Detector
  Compatible with Arduino IDE 1.8.18
*/

#include <Arduino_BMI270_BMM150.h>
#include <TensorFlowLite.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/version.h>

// Include the model data header
#include "model.h"

// Parameters
const int sequenceLength = 100;
const float anomalyThreshold = 0.7;
const int samplingPeriod = 10;

// Buffers
float gyroSequence[sequenceLength][3];
int sequenceIndex = 0;
bool bufferFilled = false;

// TFLite setup
tflite::MicroErrorReporter tflErrorReporter;
tflite::AllOpsResolver tflOpsResolver;
const tflite::Model* tflModel = NULL;
tflite::MicroInterpreter* tflInterpreter = NULL;

// Memory buffer
const int tensorArenaSize = 32 * 1024;
byte tensorArena[tensorArenaSize];

void addToSequence(float gX, float gY, float gZ);
float runInference();
void preprocessData();

void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (!IMU.begin()) {
    //Serial.println("Failed to initialize IMU!");
    while (1);
  }

  //Serial.print("Gyroscope sample rate = ");
  //Serial.print(IMU.gyroscopeSampleRate());
  //Serial.println(" Hz");
 
  // Initialize TensorFlow Lite
  //Serial.println("Initializing TensorFlow Lite...");
 
  tflModel = tflite::GetModel(model);
  if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
    //Serial.println("Model schema mismatch!");
    while (1);
  }

  tflInterpreter = new tflite::MicroInterpreter(
    tflModel,
    tflOpsResolver,
    tensorArena,
    tensorArenaSize,
    &tflErrorReporter
  );

  if (tflInterpreter->AllocateTensors() != kTfLiteOk) {
    //Serial.println("AllocateTensors() failed");
    while (1);
  }

  //Serial.println("Model loaded successfully!");
  //Serial.println("LSTM Gyroscope Anomaly Detector Started");
  //Serial.println("X-Gyro,Y-Gyro,Z-Gyro,Anomaly_Score");
 
  // Initialize buffer
  memset(gyroSequence, 0, sizeof(gyroSequence));
}

void loop() {
  float gX, gY, gZ;
 
  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(gX, gY, gZ);
    addToSequence(gX, gY, gZ);
   
    //Serial.print(gX);
    //Serial.print(",");
    //Serial.print(gY);
    //Serial.print(",");
    //Serial.print(gZ);
   
    if (bufferFilled && sequenceIndex % 10 == 0) {
      float anomalyScore = runInference();
      //Serial.print(",");
      //Serial.println(anomalyScore, 6);
     
      if (anomalyScore > anomalyThreshold) {
        //Serial.println("ANOMALY DETECTED!");
      }
    } else {
      //Serial.println(",NA");
    }
   
    delay(samplingPeriod);
  }
}

void addToSequence(float gX, float gY, float gZ) {
  gyroSequence[sequenceIndex][0] = gX;
  gyroSequence[sequenceIndex][1] = gY;
  gyroSequence[sequenceIndex][2] = gZ;
 
  sequenceIndex = (sequenceIndex + 1) % sequenceLength;
  if (sequenceIndex == 0) bufferFilled = true;
}

void preprocessData() {
  TfLiteTensor* input = tflInterpreter->input(0);
 
  int seq_dim = (input->dims->size == 3) ? 1 : 0;
  int feature_dim = (input->dims->size == 3) ? 2 : 1;
 
  int seq_length = input->dims->data[seq_dim];
  int feature_count = input->dims->data[feature_dim];
 
  int actual_seq_length = min(seq_length, sequenceLength);
  int actual_feature_count = min(feature_count, 3);
 
  // Find min/max for normalization
  float minVal[3] = {1000, 1000, 1000};
  float maxVal[3] = {-1000, -1000, -1000};
 
  for (int i = 0; i < actual_seq_length; i++) {
    for (int j = 0; j < actual_feature_count; j++) {
      minVal[j] = min(minVal[j], gyroSequence[i][j]);
      maxVal[j] = max(maxVal[j], gyroSequence[i][j]);
    }
  }
 
  // Normalize and copy to input tensor
  for (int i = 0; i < actual_seq_length; i++) {
    for (int j = 0; j < actual_feature_count; j++) {
      float normalized_value = 0;
      if (maxVal[j] != minVal[j]) {
        normalized_value = 2.0 * (gyroSequence[i][j] - minVal[j]) / (maxVal[j] - minVal[j]) - 1.0;
      }
     
      if (input->dims->size == 3) {
        input->data.f[i * feature_count + j] = normalized_value;
      } else {
        input->data.f[i * feature_count + j] = normalized_value;
      }
    }
  }
}

float runInference() {
  preprocessData();
 
  if (tflInterpreter->Invoke() != kTfLiteOk) {
    //Serial.println("Invoke failed!");
    return -1.0;
  }
 
  return tflInterpreter->output(0)->data.f[0];
}

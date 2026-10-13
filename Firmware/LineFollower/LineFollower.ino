/*
 ==============================================================================
  Project: Arduino Nano PID Line Follower Robot
  Author:  Neil Gibson Pinto
  Hardware: Custom Arduino Nano Line Follower PCB
            - Controller: Arduino Nano (ATmega328P)
            - Motor Driver: SparkFun TB6612FNG Module (ROB-14450)
            - Sensor Array: 8-Channel Analog IR Sensor Bar (A0 - A7)
            - Indicator: Status LED (D12)
            - Control: Start Button (D2), Calibration Button (D3)
 ==============================================================================
*/

// ------------ Sensor Configuration ------------
#define NUM_SENSORS 8
const int sensorPins[NUM_SENSORS] = {A0, A1, A2, A3, A4, A5, A6, A7};
int sensorValue[NUM_SENSORS];

// ------------ TB6612FNG Motor Driver Pins ------------
#define STBY 4    // Standby enable pin (Active HIGH)
#define PWMA 5    // Left Motor Speed (PWM)
#define AIN2 7    // Left Motor Direction 2
#define AIN1 8    // Left Motor Direction 1

#define PWMB 6    // Right Motor Speed (PWM)
#define BIN1 9    // Right Motor Direction 1
#define BIN2 10   // Right Motor Direction 2

// ------------ User Interface Pins ------------
#define BTN_START 2       // Start / Run Button (Active LOW)
#define BTN_CALIBRATE 3   // Sensor Calibration Button (Active LOW)
#define LED_STATUS 12     // Status LED Indicator

// ------------ PID Controller Parameters ------------
float Kp = 0.25;
float Ki = 0.00;
float Kd = 3.50;

float error = 0;
float previousError = 0;
float integral = 0;
float derivative = 0;
float correction = 0;

int baseSpeed = 170;
int maxSpeed = 255;

// Sensor position weights across the array
const int weights[8] = {-3500, -2500, -1500, -500, 500, 1500, 2500, 3500};

void setup()
{
  // Motor control pins
  pinMode(STBY, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  // Push button inputs with internal pull-up resistors
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_CALIBRATE, INPUT_PULLUP);

  // Status indicator LED
  pinMode(LED_STATUS, OUTPUT);
  digitalWrite(LED_STATUS, LOW);

  // Enable motor driver
  digitalWrite(STBY, HIGH);

  Serial.begin(115200);

  // Optional: Wait for START button press before running
  // while (digitalRead(BTN_START) == HIGH) { delay(10); }
  // digitalWrite(LED_STATUS, HIGH);
}

void loop()
{
  readSensors();

  error = calculateError();

  integral += error;
  derivative = error - previousError;

  correction = (Kp * error) + (Ki * integral) + (Kd * derivative);

  previousError = error;

  int leftSpeed = baseSpeed + correction;
  int rightSpeed = baseSpeed - correction;

  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  setMotor(leftSpeed, rightSpeed);
}

// ----------------------------------------------------------------------------
// Read Analog Values from all 8 Sensors
// ----------------------------------------------------------------------------
void readSensors()
{
  for (int i = 0; i < NUM_SENSORS; i++)
  {
    sensorValue[i] = analogRead(sensorPins[i]);
  }
}

// ----------------------------------------------------------------------------
// Calculate Line Position Error using Weighted Average
// ----------------------------------------------------------------------------
float calculateError()
{
  long weightedSum = 0;
  long total = 0;

  for (int i = 0; i < NUM_SENSORS; i++)
  {
    int value = 1023 - sensorValue[i];
    if (value < 0) value = 0;

    weightedSum += (long)value * weights[i];
    total += value;
  }

  // If line is lost, hold the last turn direction
  if (total == 0)
  {
    if (previousError > 0)
      return 3500;
    else
      return -3500;
  }

  return (float)weightedSum / total;
}

// ----------------------------------------------------------------------------
// Differential Drive Motor Output
// ----------------------------------------------------------------------------
void setMotor(int left, int right)
{
  // Left Motor Control
  if (left >= 0)
  {
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
  }
  else
  {
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    left = -left;
  }

  // Right Motor Control
  if (right >= 0)
  {
    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);
  }
  else
  {
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, HIGH);
    right = -right;
  }

  left = constrain(left, 0, maxSpeed);
  right = constrain(right, 0, maxSpeed);

  analogWrite(PWMA, left);
  analogWrite(PWMB, right);
}


#include <SoftwareSerial.h>
#include <math.h>

// JY901S TX -> Arduino D2
SoftwareSerial imu(2, 3);

uint8_t packet[11];
int packetIndex = 0;

float roll = 0;
float pitch = 0;
float yaw = 0;

// Starting orientation
float neutralPitch = 0;
float neutralYaw = 0;

bool calibrated = false;
int calibrationSamples = 0;

float pitchTotal = 0;
float yawSinTotal = 0;
float yawCosTotal = 0;

// Control settings
const float DEADZONE = 5.0;
const float FULL_TILT = 30.0;
const float FULL_TURN = 45.0;

// Directional coordinates
float moveX = 0;
float moveY = 0;

float clampValue(float value, float minV, float maxV) {
  if (value < minV) return minV;
  if (value > maxV) return maxV;
  return value;
}

// Handle angle wraparound at +/-180 degrees
float angleDifference(float current, float reference) {
  float diff = current - reference;

  while (diff > 180) diff -= 360;
  while (diff < -180) diff += 360;

  return diff;
}

void setup() {
  Serial.begin(9600);
  imu.begin(9600);

  Serial.println("Hold controller level and forward");
}

void loop() {
  while (imu.available()) {

    uint8_t incoming = imu.read();

    if (packetIndex == 0 && incoming != 0x55) {
      continue;
    }

    packet[packetIndex++] = incoming;

    if (packetIndex == 11) {
      packetIndex = 0;

      uint8_t checksum = 0;

      for (int i = 0; i < 10; i++) {
        checksum += packet[i];
      }

      if (checksum != packet[10]) {
        continue;
      }

      if (packet[1] == 0x53) {

        int16_t r = (int16_t)
          ((packet[3] << 8) | packet[2]);

        int16_t p = (int16_t)
          ((packet[5] << 8) | packet[4]);

        int16_t y = (int16_t)
          ((packet[7] << 8) | packet[6]);

        roll = r * 180.0 / 32768.0;
        pitch = p * 180.0 / 32768.0;
        yaw = y * 180.0 / 32768.0;

        // Calibrate neutral orientation
        if (!calibrated) {
          pitchTotal += pitch;

          float yawRad = yaw * PI / 180.0;
          yawSinTotal += sin(yawRad);
          yawCosTotal += cos(yawRad);

          calibrationSamples++;

          if (calibrationSamples >= 30) {
            neutralPitch = pitchTotal / 30.0;

            neutralYaw =
              atan2(yawSinTotal, yawCosTotal)
              * 180.0 / PI;

            calibrated = true;
            Serial.println("CALIBRATION COMPLETE");
          }

          continue;
        }

        // Relative orientation
        float turnAngle =
          angleDifference(yaw, neutralYaw);

        float upAngle = pitch - neutralPitch;

        // Convert angles into control coordinates
        if (fabs(turnAngle) <= DEADZONE) {
          moveX = 0;
        } else {
          moveX = clampValue(
            turnAngle / FULL_TURN, -1.0, 1.0
          );
        }

        if (upAngle <= DEADZONE) {
          moveY = 0;
        } else {
          moveY = clampValue(
            upAngle / FULL_TILT, 0.0, 1.0
          );
        }

        // Output for Unity
        Serial.print("X:");
        Serial.print(moveX, 2);

        Serial.print("\tY:");
        Serial.print(moveY, 2);

        Serial.print("\tYaw:");
        Serial.print(turnAngle, 1);

        Serial.print("\tPitch:");
        Serial.println(upAngle, 1);
      }
    }
  }
}


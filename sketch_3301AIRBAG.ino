const int enA = 9;
const int in1 = 4;
const int in2 = 5;
const int STBY = 10;
const int pressurePin = A0;

// ---------- SETTINGS and VARs ----------
const int INITIAL_TARGET = 190;
const int MAX_PRESSURE = 220;

// Relative to resting pressure
const int SQUEEZE_THRESHOLD = 4;
const int RELEASE_THRESHOLD = 2;

// Pump settings
const int PUMP_PWM = 255;
const unsigned long PUMP_PULSE = 190;
const unsigned long MAX_INITIAL_TIME = 2500;

// Timing
const unsigned long DEBOUNCE = 50;
const unsigned long SETTLE_TIME = 600;


float filteredPressure = 0;
float baseline = 0;

int squeezeCount = 0;

bool squeezing = false;
bool pumpRunning = false;
bool finished = false;

unsigned long squeezeStart = 0;
unsigned long releaseStart = 0;
unsigned long pumpStart = 0;
unsigned long settleStart = 0;

enum State {
  INITIAL_INFLATION,
  SETTLING,
  READY,
  PUMPING,
  FINISHED
};

State state = INITIAL_INFLATION;
State afterSettling = READY;

// ---------- PUMP FUNCTIONS ----------

void pumpOn() {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(STBY, HIGH);
  analogWrite(enA, PUMP_PWM);
  pumpRunning = true;
}

void pumpOff() {
  analogWrite(enA, 0);
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(STBY, LOW);
  pumpRunning = false;
}

// ---------- SETUP ----------

void setup() {
  Serial.begin(9600);

  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(STBY, OUTPUT);
  pinMode(pressurePin, INPUT);

  pumpOff();

  // Initial sensor reading
  long total = 0;

  for (int i = 0; i < 100; i++) {
    total += analogRead(pressurePin);
    delay(10);
  }

  filteredPressure = total / 100.0;
  baseline = filteredPressure;

  Serial.print("Initial reading: ");
  Serial.println(baseline);

  // Begin initial inflation
  pumpStart = millis();
  pumpOn();

  Serial.println("INITIAL INFLATION");
}



void loop() {
  unsigned long now = millis();

  int raw = analogRead(pressurePin);

  filteredPressure =
      0.2 * raw + 0.8 * filteredPressure;

  // Stop pumping at configured ceiling
  if (filteredPressure >= MAX_PRESSURE) {
    pumpOff();
    state = FINISHED;
    Serial.println("PRESSURE LIMIT REACHED");
  }

  switch (state) {

    // 1. Initial inflation
    case INITIAL_INFLATION:

      if (filteredPressure >= INITIAL_TARGET ||
          now - pumpStart >= MAX_INITIAL_TIME) {

        pumpOff();

        state = SETTLING;
        afterSettling = READY;
        settleStart = now;

        Serial.println("INITIAL PUMP STOPPED");
      }
      break;

    // 2. Allow pressure to stabilize
    case SETTLING:

      if (now - settleStart >= SETTLE_TIME) {

        baseline = filteredPressure;
        squeezing = false;
        squeezeStart = 0;
        releaseStart = 0;

        state = afterSettling;

        Serial.print("New baseline: ");
        Serial.println(baseline);

        if (state == READY) {
          Serial.println("READY TO SQUEEZE");
        }
      }
      break;

    // 3. Detect squeeze and release
    case READY: {

      float change = filteredPressure - baseline;

      if (!squeezing) {

        if (change >= SQUEEZE_THRESHOLD) {

          if (squeezeStart == 0) {
            squeezeStart = now;
          }

          if (now - squeezeStart >= DEBOUNCE) {
            squeezing = true;
            releaseStart = 0;

            Serial.println("SQUEEZING");
          }

        } else {
          squeezeStart = 0;
        }

      } else {

        if (change <= RELEASE_THRESHOLD) {

          if (releaseStart == 0) {
            releaseStart = now;
          }

          if (now - releaseStart >= DEBOUNCE) {

            squeezeCount++;

            Serial.print("SQUEEZE COUNT: ");
            Serial.println(squeezeCount);

            squeezing = false;
            squeezeStart = 0;
            releaseStart = 0;

            // Add a small amount of air
            if (filteredPressure < MAX_PRESSURE) {
              pumpOn();
              pumpStart = now;
              state = PUMPING;

              Serial.println("PUMP ON");
            }
          }

        } else {
          releaseStart = 0;
        }
      }
      break;
    }

    // 4. Brief pump pulse
    case PUMPING:

      if (now - pumpStart >= PUMP_PULSE) {

        pumpOff();

        state = SETTLING;
        afterSettling = READY;
        settleStart = now;

        Serial.println("PUMP OFF");
      }
      break;

    // 5. Maximum pressure reached
    case FINISHED:
      pumpOff();
      break;
  }

  // Debug output
  static unsigned long lastPrint = 0;

  if (now - lastPrint >= 150) {
    lastPrint = now;

    
    Serial.print("Raw: ");
    Serial.print(raw);

    //Serial.print(" | Baseline: ");
    //Serial.print(baseline, 1);

    //Serial.print(" | State: ");
    //Serial.print(state);

    Serial.print("Count: ");
    Serial.println(squeezeCount);

    // serial plotter
    Serial.print("\tPressure:");
    Serial.print(filteredPressure, 1);

    Serial.print("\tBaseline:");
    Serial.print(baseline, 1);

    Serial.print("\tThreshold:");
    Serial.print(baseline + SQUEEZE_THRESHOLD, 1);

    Serial.print("\tPump:");
    Serial.print(pumpRunning ? 120 : 85);

  }


  delay(10);
}
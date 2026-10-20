const int enA = 9;
const int in1 = 4;
const int in2 = 5;
const int STBY = 10;
const int pressurePin = A0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin (9600);
  pinMode (STBY, OUTPUT) ;
  pinMode (enA, OUTPUT) ; 
  pinMode(in1, OUTPUT) ; 
  pinMode (in2, OUTPUT) ;
  // initialize motor
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);

}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  analogWrite(enA, 255);
  digitalWrite(STBY, HIGH);

}

const int OUT1_PIN = 2;
const int OUT2_PIN = 3;

void setup() 
{
  pinMode(OUT1_PIN, OUTPUT);
  pinMode(OUT2_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("Liquid sensor ready");
}

void loop() 
{
  static bool state = false;

  int pinState = state ? HIGH : LOW;

  digitalWrite(OUT1_PIN, pinState);
  digitalWrite(OUT2_PIN, !pinState);
  delay(100);
  
  //int data = state ? analogRead(A0) : analogRead(A1);
  int data1 = analogRead(A0);
  int data2 = analogRead(A1);
  state = !state;

  char serialbuffer[100];
  sprintf(serialbuffer, "data1=%d data2=%d final=%d", data1, data2, abs(data1-data2) );
  Serial.println(serialbuffer);

  delay(1000);
}

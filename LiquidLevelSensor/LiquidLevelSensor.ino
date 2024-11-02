const int OUT1_PIN = 2;
const int OUT2_PIN = 3;
const int INDICATOR_PIN = 4;

const int THRESHOLD = 500;

void setup() 
{
  pinMode(OUT1_PIN, OUTPUT);
  pinMode(OUT2_PIN, OUTPUT);
  pinMode(INDICATOR_PIN, OUTPUT);
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.begin(115200);
  Serial.println("Liquid sensor ready");
}

void loop() 
{
  static bool state = false;

  // Toggle output to avoid migration
  int pinState = state ? HIGH : LOW;

  digitalWrite(OUT1_PIN, pinState);
  digitalWrite(OUT2_PIN, !pinState);
  delay(100);
  
  // Measure signal
  int data1 = analogRead(A0);
  int data2 = analogRead(A1);
  state = !state;

  int value = abs(data1-data2);
  
  // Set indicator if syrup present
  digitalWrite(INDICATOR_PIN, value < THRESHOLD);
  digitalWrite(LED_BUILTIN, value < THRESHOLD);
  
  
  
  char serialbuffer[100];
  sprintf(serialbuffer, "data1=%d data2=%d final=%d", data1, data2, value );
  Serial.println(serialbuffer);

  delay(1000);
}

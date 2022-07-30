// Project sponsor: Mutea Sabir
// Email: mutea_sabir@hotmail.com
// Creator: Cedar Lake Instruments LLC
// Date: July, 2022
//
// Description:
// Measure the speed of 6 pool balls with one optointerrupter at each ball return chute
// Depending on speed, blink LEDs in a certain pattern and trigger audio on a DF Player Pro MP3 player

#include <pt.h>

// I/O Connections:
// Arduino pin   - I/O description 
// Pin2: pulse input
// Pin5: RELAY1
// SCL(18) LCD SCL
// SDA(19)  LCD SDA

#define LED 13

#define ACTIVE 0
#define INACTIVE 1
// ON and OFF time for RELAY
#define PULSE_ON_TIME 500
#define PULSE_OFF_TIME 100

#define BUF_SIZE 40
static struct pt _pt1;
byte _pulseCount = 0;
byte _threadPulseCount = 0;

// ISR globals
volatile byte _triggeredInput = 0;


//***************************************** USER SETUP *****************************

//**********************************************************************************

void setup() 
{
    // Initialize Serial port
    Serial.begin(115200);

    attachInterrupt(digitalPinToInterrupt(START_SWITCH), isrCount, FALLING);
    sei();
	PT_INIT(&_pt1);
}

void loop() 
{  
	// ISR handling
    if (_triggeredInput)
    {
		if (_triggeredInput)
		{
			cli();
			_triggeredInput = 0;
			sei();
			_pulseCount++;
		}
    }
}

// ISR for the count interrupt
void isrCount()
{
    // Signal interrupt occurred
    _triggeredInput = 1;
}

// Pulse relay on a separate thread
static int relayPulse(struct pt *pt)
{
	static unsigned long timestamp = 0;
	PT_BEGIN(pt);
	while (_threadPulseCount > 0)
	{
		digitalWrite(LED, HIGH);
		timestamp = millis();
		PT_WAIT_UNTIL(pt, millis() - timestamp > PULSE_ON_TIME);
		digitalWrite(LED, LOW);
		timestamp = millis();
		PT_WAIT_UNTIL(pt, millis() - timestamp > PULSE_OFF_TIME);
		_threadPulseCount--;
	}
	PT_END(pt);
}

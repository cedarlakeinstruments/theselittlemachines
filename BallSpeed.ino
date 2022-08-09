////////////////////////////////////////////////////////////////////////////////////////////////////////
// Project sponsor: Mutea Sabir
// Email: mutea_sabir@hotmail.com
// Creator: Cedar Lake Instruments LLC
// Date: July, 2022
//
// Description:
// Measure the speed of 6 pool balls with one optointerrupter at each ball return chute
// Depending on speed, blink LEDs in a certain pattern and trigger audio on a DF Player Pro MP3 player
/*
 * >  I have a pool table and I want a reaction to happen when the billiard
> ball falls into the designated slot.
>
>  As you know there are 6 pool table slots
> When one of the balls falls into any slot, a sensor reads the speed of
> this ball, and on this basis, a sound is played with LED lights,
>
>  for example, Let's give the sensor reaction in three different
> degrees - 1 - 2 - 3
>
>  1 - First-speed result     ( slow Pass ball )     = LED Flashing
> Twice Times.
>  2 - Second-speed result ( middle Pass ball ) = voice Out ( Nice ) +
> LED Flashing four times.
>  3 - Third-speed result   ( fast pass ball )      =  voice Out ( good
> shot ) + LED Flashing eight times.
>
>  must be installed 6 sensors - for each slot 1 sensor, All of these
> work the same way
>
>  Only one sensor is installed at the balls collection box
>
>  So that it is parallel to the last ball, this sensor disables all
> these features after five seconds - and Enables when the balls go out
>
>  the MP3 audio Voice I have Of course we will use a memory card,
>
>  Of course, we can control the length of time or pulse as desired
 */
 
#include <DFRobot_DF1201S.h>
#include <SoftwareSerial.h>

// I/O Connections:
// Arduino pin   - I/O description 
// Pin 2 - 7: ball sensor input
// Pin 8: ball tray empty
// Pin 9: LED output (needs drive transistor)
// Pin 10,11 - DF Robot triggers

#define LED_PIN LED_BUILTIN

#define ACTIVE 0
#define INACTIVE 1
// ON and OFF time for RELAY
#define PULSE_ON_TIME 500
#define PULSE_OFF_TIME 100
#define BALL_MASK 0xFD
#define LAST_BALL 8

#define BUF_SIZE 40
//static struct pt _pt1;
byte _pulseCount = 0;
byte _threadPulseCount = 0;

char buffer[50];
SoftwareSerial DF1201SSerial(10, 11);  //RX  TX
DFRobot_DF1201S DF1201S;

//***************************************** USER SETUP *****************************
// Number of LED flashes
const int FLASH_COUNT_SLOW = 2;
const int FLASH_COUNT_MEDIUM = 4;
const int FLASH_COUNT_FAST = 8;

// Times to cross sensor at various speeds
const int SLOW_MS = 8;
const int MEDIUM_MS = 6;
const int FAST_MS = 2;

// LED blink speed
const int BLINK_SPEED = 250;
// ***********************************************************************************

class BallState
{
public:
    // Ctor
    BallState()
    {
        reset();
    }

    // Return everything to defaults
    void reset()
    {
       leadingEdgeTime = 0;
       currentState = Idle;
       transitionTime = 0;
    }

    // Called on each sensor read
    // with current high/low sensor reading and timestamp
    // Returns true if valid ball crossing detected
    bool update(bool sensorState, unsigned long timeNow)
    {
        bool retVal = false;
        switch(currentState)
        {
            case Idle:
                // Ball just entered sensor
                if (sensorState == true)
                {
                    currentState = LeDetected;
                    leadingEdgeTime = timeNow;
                }
                break;
            case LeDetected:
                if (sensorState ==  false)
                {
                    // Trailing edge detected
                    transitionTime = timeNow - leadingEdgeTime;
                    currentState = TeDetected;
                    retVal = true;
                }
                else
                {
                    if (timeNow - leadingEdgeTime > TimeoutMs)
                    {
                        // Taking too long to cross sensor
                        reset();
                        currentState = Idle;
                    }
                }
                break;
            case TeDetected:
                reset();
                break;
        }
        return retVal;
    }

    // Time it took ball to cross sensor
    int transitionTime = 0;
    
private:
    unsigned long leadingEdgeTime = 0;
    const int TimeoutMs = 250;   
    
    enum States {Idle, LeDetected, TeDetected};
    States currentState;    
};

BallState _balls[6];

//**********************************************************************************

void setup() 
{
    // Initialize Serial port
    Serial.begin(115200);

    // Pin configuration
    pinMode(LAST_BALL, INPUT_PULLUP);
    
    // Configure pins D2-D7 as inputs
    DDRD = 0;

    // Enable internal pullups
    PORTD = BALL_MASK;   

    // Setup DFr
    Serial.println("Start Audio Player setup");
    DF1201SSerial.begin(115200);
    bool df1201Ready = false;
    for (int i=0; i < 5; i++)
    {   if (DF1201S.begin(DF1201SSerial))
        {
            df1201Ready = true;
            break;
        }
        Serial.println("DF1201 failed, please check the wire connection!");
        delay(1000);
    }
    
    if (df1201Ready)
    {
        Serial.println("DF1201 connected");
    }
    
    delay(100);
    /*Set volume to 20*/
    DF1201S.setVol(20);
    delay(100);
    
    /*Enter music mode*/
    DF1201S.switchFunction(DF1201S.MUSIC);
    // Wait for prompt to stop
    delay(2000);
    
    //Turn off the prompt tone (Power-down save) 
    DF1201S.setPrompt(false);
    delay(100);
    
    DF1201S.setPlayMode(DF1201S.SINGLE);
    delay(100);
    
    //Enable amplifier chip 
    DF1201S.enableAMP();
    delay(100);
  
    Serial.print("The number of files available to play:");
    //The number of files available to play
    Serial.println(DF1201S.getTotalFile());
  
    Serial.println("DF1201 configured");
    Serial.println("BallSpeed 1.0 ready");
}
    
void loop() 
{  
    const int FIRST_PIN = 2;
    byte balls = PIND & BALL_MASK;

    //delay(500);
    //sprintf(buffer,"balls 0x%x",balls);
    //Serial.println(buffer);
    
    // Disable speed measurement 5 seconds after last ball detected
    if (digitalRead(LAST_BALL) == HIGH)
    {
        // Balls still in collection area
        //return;
    }
    
    // Figure out which bit changed. Stop after the first one
    for (int i = 0; i < 5; i++)
    {            
        bool v = (1 << (i+FIRST_PIN)) & balls;
        if (_balls[i].update(v, millis()))
        {
            celebrate(i);
            break;
        }
    }   

    // Monitor the ball storage and reset everyone once the balls are gone
    
}

// Flash LEDs and play sound for this pocket
void celebrate(int pocket)
{
    sprintf(buffer,"Ball %d time: %d",pocket+1, _balls[pocket].transitionTime);
    Serial.println(buffer);

    const String files[] = {"/fast.mp3", "/medium.mp3", "/slow.mp3"};
    const int flashes[] = {FLASH_COUNT_FAST, FLASH_COUNT_MEDIUM,FLASH_COUNT_SLOW};

    int index = flashCountIndex(_balls[pocket].transitionTime);
    if (index < 3)
    {
        Serial.print("Playing ");Serial.println(files[index]);
        DF1201S.playSpecFile(files[index]);        
        
        for (int i = 0; i < flashes[index]; i++)
        {
            digitalWrite(LED_PIN, HIGH);
            delay(BLINK_SPEED);
            digitalWrite(LED_PIN, LOW);
            delay(BLINK_SPEED);        
        }
    }
    else
    {
        Serial.println("Index out of range");
    }
}

// Returns number of flashes based on transition time
int flashCountIndex(int time)
{
    if (time <= FAST_MS)
    {
        Serial.println("Fast");
        return 0;
    }
    else if (time <= MEDIUM_MS)
    {
        Serial.println("Medium");
        return 1;
    }
    else
    {
        Serial.println("Slow");
        return 2;
    }
}

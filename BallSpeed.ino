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

// v1.2 adds:
>  all The 6 Sensors Except Pin 8 Need To be disabled Exactly after the
> ball fell in the slot & Activate after 5 Seconds Of course I can
> increase it later, 
> 
>  The point of this
> I don't want to repeat the light and sound constantly

  v1.3 adds:
>>  I will install a sensor that detects the white ball at each ball
>> entrance
>>
>>  When the white ball enters, the speed sensor stops for a part of a
>> second, or as required
>>
>>  In this case, it is not encouraged when the white ball falls
>>
>>  And I will install an additional sensor that will operate a relay
>> when the white ball passes
>>
>>  so Now The number of 6 entrances when the white ball is detected
> Will
>> disables the system for parts of a second
>>The 7 white ball sensors connected to a separate common Arduino
> input pin
>
>  So that, when this 7 sensor is activated, the separate output relay
> works for a specific time, for example, a second or more
*/
 
#include <SoftwareSerial.h>

// I/O Connections:
// Arduino pin   - I/O description 
// Pin 2 - 7: ball sensor input
// Pin 8: ball tray empty
// Pin 9: LED output (needs drive transistor)
// Pin 10: DF Robot Player TX
// Pin 11: DF Robot Player RX
// Pin 12: White cue ball detect
// Pin 14: Cue ball relay output

#define LED_PIN 9
#define CUE_BALL_ENTER_PIN 12
#define CUE_BALL_EXIT_PIN 14
#define CUE_BALL_RELAY_PIN 15

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
SoftwareSerial DfPlayerSerial(10, 11);  //RX  TX

#define PLAYER_MINI

#ifdef PLAYER_PRO
 #include <DFRobot_DF1201S.h>
 DFRobot_DF1201S DfPlayer;
#else
 #include "DFRobotDFPlayerMini.h"
 DFRobotDFPlayerMini DfPlayer;
#endif

//***************************************** USER SETUP *****************************
// Number of LED flashes
const int FLASH_COUNT_SLOW = 2;
const int FLASH_COUNT_MEDIUM = 4;
const int FLASH_COUNT_FAST = 8;

// LED flash speed
const int FLASH_FAST = 200;
const int FLASH_MEDIUM = 250;
const int FLASH_SLOW = 300;

// Times to cross sensor at various speeds (Set time to -1 if not used. e.g., to ignore MEDIUM speed set MEDIUM_MS = -1)
const int SLOW_MS = 8;
const int MEDIUM_MS = 6;
const int FAST_MS = 2;

// Timeout after last ball in collection area
const int TIMEOUT_MS = 5000;

// Delay further detection after ball detected
const int BALL_SENSOR_DELAY = 5000;

// Audio volume
const int VOLUME = 20;

// Sensor ignore time after cue ball detected
const int CUE_BALL_DELAY_MS = 1000;
// Relay on time after cue ball exit trigger
const int RELAY_ACTIVE_MS = 2000;

// Relay states
#define ACTIVE LOW
#define INACTIVE HIGH
// ***********************************************************************************

enum BALL_STATES {IDLE, CHECKING, DONE};

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

        // Pause if cue ball detected
        if (digitalRead(CUE_BALL_ENTER_PIN) == HIGH) 
        {
            Serial.println("Cue ball enter detected");
            delay(CUE_BALL_DELAY_MS);
            return false;
        }
                  
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
    pinMode(CUE_BALL_ENTER_PIN, INPUT_PULLUP);
    pinMode(CUE_BALL_EXIT_PIN, INPUT_PULLUP);
    pinMode(CUE_BALL_RELAY_PIN, OUTPUT);
    
    // Configure pins D2-D7 as inputs
    DDRD = 0;

    // Enable internal pullups
    PORTD = BALL_MASK;   

    // Setup output pins last
    pinMode(LED_PIN, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    
    // Setup DF Robot Player
    Serial.println("Start Audio Player setup");
    
#ifdef PLAYER_PRO
    DfPlayerSerial.begin(115200);
#else
    DfPlayerSerial.begin(9600);
#endif

    bool dfPlayerReady = false;
    Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
    for (int i=0; i < 5; i++)
    {   if (DfPlayer.begin(DfPlayerSerial))
        {
            dfPlayerReady = true;
            break;
        }
        Serial.println("DFPlayer failed, please check the wire connection!");
        delay(1000);
    }
    
    if (dfPlayerReady)
    {
        Serial.println("DF Player connected");
    }
    
    delay(100);
#ifdef PLAYER_PRO
    /*Set volume to 20*/
    DfPlayer.setVol(VOLUME);
    delay(100);
    
    /*Enter music mode*/
    DfPlayer.switchFunction(DfPlayer.MUSIC);
    // Wait for prompt to stop
    delay(2000);
    
    //Turn off the prompt tone (Power-down save) 
    DfPlayer.setPrompt(false);
    delay(100);
    
    DfPlayer.setPlayMode(DfPlayer.SINGLE);
    delay(100);
    
    //Enable amplifier chip 
    DfPlayer.enableAMP();
    delay(100);
  
    Serial.print("The number of files available to play:");
    //The number of files available to play
    Serial.println(DfPlayer.getTotalFile());
  
    Serial.println("DF1201 configured");
#else
    /*Set volume to 20*/
    DfPlayer.volume(VOLUME);
    delay(100);
    Serial.print("The number of files available to play: ");
    Serial.println(DfPlayer.readFileCounts());
    Serial.println("DF Player Mini configured");
#endif
    Serial.println("BallSpeed 1.2 ready");
}
    
void loop() 
{  
    const int FIRST_PIN = 2;
    byte balls = PIND & BALL_MASK;

    static BALL_STATES state = IDLE;
    static unsigned long doneTime = 0;
    
    switch (state)
    {
        case IDLE:
          if (digitalRead(LAST_BALL) == LOW)
          {
              // All balls on table
              state = CHECKING;
              Serial.println("All balls on table");
          }
          break;
        case CHECKING:       
          // Figure out which bit changed. Stop after the first one
          for (int i = 0; i < 5; i++)
          {            
              bool v = (1 << (i+FIRST_PIN)) & balls;
              if (_balls[i].update(v, millis()))
              {
                  celebrate(i);
                  delay(BALL_SENSOR_DELAY);
                  break;
              }
          }   
          // Disable speed measurement 5 seconds after last ball detected
          if ( digitalRead(LAST_BALL) == HIGH)
          {
              // All balls in collection area
              Serial.println("All balls collected");
              doneTime = millis() + TIMEOUT_MS;
              state = DONE;
          }

          break;
        case DONE:              
          // Monitor the ball storage and reset everyone once the balls are gone
          if (millis() > doneTime)
          {
             Serial.println("Returning to idle state");
             state = IDLE;
          }
          break;
    }  
    // Service relay
    checkRelay();  
}

// Check cue ball exit and activate relay if triggered
void checkRelay()
{
    static bool savedState = false;
    static unsigned long releaseTime;    
    unsigned long timeNow = millis();
    
    if (digitalRead(CUE_BALL_EXIT_PIN) && (savedState == false) )
    {
        savedState = true;
        digitalWrite(CUE_BALL_RELAY_PIN, ACTIVE);
        releaseTime = timeNow + RELAY_ACTIVE_MS;
        Serial.println("Cue ball exit detected");
    }
    else if (savedState)
    {
       if (timeNow > releaseTime)
       {
          savedState = false;
          digitalWrite(CUE_BALL_RELAY_PIN, INACTIVE);
          Serial.println("Relay deactivated");
       }
    }    
}

// Flash LEDs and play sound for this pocket
void celebrate(int pocket)
{
    sprintf(buffer,"Ball %d time: %d",pocket+1, _balls[pocket].transitionTime);
    Serial.println(buffer);

    const String files[] = {"/fast.mp3", "/medium.mp3", "/slow.mp3"};
    const int flashes[] = {FLASH_COUNT_FAST, FLASH_COUNT_MEDIUM,FLASH_COUNT_SLOW};
    const int flashDelay[] {FLASH_FAST, FLASH_MEDIUM, FLASH_SLOW};
    
    int index = flashCountIndex(_balls[pocket].transitionTime);
    if ((index < 3) && (index >= 0))
    {
        Serial.print("Playing ");Serial.println(files[index]);

#ifdef PLAYER_PRO
        DfPlayer.playSpecFile(files[index]);        
#else
        DfPlayer.play(index+1);
#endif
        for (int i = 0; i < flashes[index]; i++)
        {
            digitalWrite(LED_PIN, HIGH);
            digitalWrite(LED_BUILTIN, HIGH);
            
            delay(flashDelay[index]);
            
            digitalWrite(LED_PIN, LOW);
            digitalWrite(LED_BUILTIN, LOW);
            
            delay(flashDelay[index]);        
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
    else if (time <= SLOW_MS)
    {
        Serial.println("Slow");
        return 2;
    }
    else
    {
        Serial.println("Very long time");
        return -1;
    }
}

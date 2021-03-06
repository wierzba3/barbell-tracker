#include <StandardCplusplus.h>
#include <vector>
#include <LiquidCrystal.h>

#define ENC_A 14
#define ENC_B 15
//#define ENC_A 2
//#define ENC_B 3


#define ENC_PORT PINC

using namespace std;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 9, 10);


/**
 * BUGS:
 * 
 * TODO
 * - figure out a way to remove the extra time steps from 
 * - figure out why encoder steps are being missed. 
 *     Could be the cheap encoder, or possibly too much CPU time spent on code, before the next read.
 *     Try using interupts instead of polling. See example: http://playground.arduino.cc/Main/RotaryEncoders#Example3
 */


bool debugging = false;
int printCnt = 0;

const int MAX_VALUE = 32767; //max integer value
const int MIN_VALUE = -32768; //min integer value
int repCounter = 0;
const double STEPS_PER_CM = 4.098; //the amount of turns of the rotary encoder per cm of the string pulled


int minValue = MAX_VALUE; //Variable that keeps track of the most recent minimum counter value, and the time at which it were hit.
int maxValue = MIN_VALUE; //Variable that keeps track of the most recent minimum counter value, and the time at which it were hit.
int counter = 0; //counter is incremented as the string is pulled, and decremented as the string retracts
int lastCounter = 0; //the previous counter value at which a time was recorded


vector<long> timeSteps; //A list of time values taken every <counterInterval> counter steps
vector<double> avgVelocityValues;
vector<double> maxVelocityValues;
vector<double> maxAccelerationValues;

bool stringRetracting = true; //true if the string is retracting (the lifter is in the concentric phase of the exercise)
bool stringPulling = false; //true if the string is being pulled (the lifter is in the eccentric phase of the exercise)

int8_t encoderStep; //holds the temporary value read from the quadrature encoder, -1, 0, or 1 which signify clockwise, invalid, counter-clockwise respectively

//settings: (create a way to modify these variables via user-input in the future)
long repEndWaitTime = 5 * 1000000; //the amount of time to wait at the top of the rep with no activity to count it as a rep complete (in micros)
int minDistance = 5 * STEPS_PER_CM; //the minimum amount of cm the string must move in the opposite direction to be determined as switched direction
int counterInterval = 10; //the value at which each time sample is taken. 1cm ~= 4.098 counts
//A distance from the end (in ticks) of the range of motion. All velocity values are scanned...
int reRackDistance = 120; //...if a velocity value is found that is less than <minVelocity>, all subsequent values are truncated
double minVelocity = 0.1;

void setup()
{
  /* Setup encoder pins as inputs */
  pinMode(ENC_A, INPUT);
  digitalWrite(ENC_A, HIGH);
  pinMode(ENC_B, INPUT);
  digitalWrite(ENC_B, HIGH);
  Serial.begin (115200);
  Serial.println("Start");
  Serial.println("");
  lcd.begin(16, 2);
  lcd.clear();
}

/* returns change in encoder state (-1,0,1) */
int8_t read_encoder()
{
  static int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
  static uint8_t old_AB = 0;
  old_AB <<= 2;                   //remember previous state
  old_AB |= ( ENC_PORT & 0x03 );  //add current state
  return ( enc_states[( old_AB & 0x0f )]);
}
 
void loop()
{
  encoderStep = read_encoder();

  //Code to take samples every pre-determined distance
  if(encoderStep) 
  {
//    if(debugging)
//    {
//      Serial.print(counter, DEC);
//      Serial.print(", ");
//      printCnt++;
//      if(printCnt > 13)
//      {
//        Serial.println("");
//        printCnt = 0;
//      }
//    }
    
    counter += encoderStep;

    if(counter < minValue) 
    {
      minValue = counter;
    }
    if(counter > maxValue)
    {
      maxValue = counter;
      if(counter > lastCounter + counterInterval)
      {
        lastCounter = counter;
        if(stringPulling) timeSteps.push_back(micros());
      }
    }
    
    //if the string is retracting and we notice an upward movement
    if(stringRetracting && encoderStep > 0)
    {
      //the distance moved from the bottom
      int diff = counter - minValue;
      
      //if the distance moved upwards is greater than the minimum required, we conclude the exercise is now in the upwards (eccentric) phase
      if(diff > minDistance)
      {
        switchDirection(true); //switch string state to pulling
        return;
      }
      
    }

    //if the string is being pulled and we notice a downward movement
    if(stringPulling && encoderStep < 0)
    {
      //the distance moved from the top
      int diff = maxValue - counter;
      //if the distance moved downwards is greater than the minimum required, we conclude that the exercise is now in the downards (concentric) phase
      if(diff > minDistance)
      {
        switchDirection(false); //switch string state to retracting
        return;
      }
    }

    if(stringPulling)
    {
      //special case, check if the time since the last sample is greater than the minimum (currently 5 seconds)
      if(timeSteps.size() > 0 && (micros() - timeSteps[timeSteps.size()-1]) > repEndWaitTime)
      {
        Serial.println("TIMEOUT");
        switchDirection(false); //switch string state to retracting
        return;
      }
    }
    
    
    
  }
}


void switchDirection(bool pulling)
{
  if(pulling)
  {
    stringRetracting = false;
    stringPulling = true;
    //RESET counter because inaccuraces with encoder readings may cause mis-steps, and possibly negative counter values.
    counter = minValue = lastCounter = 0;
    maxValue = MIN_VALUE;
  }
  else
  {
    calculateVelocity();
    stringRetracting = true;
    stringPulling = false;
    minValue = MAX_VALUE;
    maxValue = MIN_VALUE;
  }
}

void calculateVelocity()
{
  //last index of time steps vector
  int lastTimeIdx = timeSteps.size() - 1; 
  
  //the index at which we start checking for small velocity values to truncate at the end of the rep
  int rerackIndex = lastTimeIdx - (reRackDistance / counterInterval); 
  
  //remove the last time step, as the lifter will probably stop for breaths at the top, and it may read a new time step much later
  //samples are taken every ~1cm, so let's remove 2 smaples to be sure
  timeSteps.pop_back();
  timeSteps.pop_back();

  



  
  if(timeSteps.size() > 1)
  {
    repCounter++;
    Serial.println("");
    Serial.println("");
    Serial.print("Rep ");
    Serial.print(repCounter);
    Serial.println(":");

    double total = 0, maxVelocity = 0, maxAcceleration = 0;
    double prevVelocity = -1;
    long prevTime = timeSteps[0];
    int cnt = 0;

    if(debugging) Serial.print("values: [");
    //declare variables used in loop
    double dt, dt_sec, stepsPerSec, velocity, acceleration;
    for(int i = 1; i < timeSteps.size(); i++)
    {
      dt = timeSteps[i] - prevTime;
      dt_sec = dt / 1000000.0;
      if(dt_sec == 0) continue;
      stepsPerSec = counterInterval / dt_sec;
      velocity = (stepsPerSec / STEPS_PER_CM) / 100.0;

//      if(debugging)
//      {
//        Serial.print("dt=");
//        Serial.print(dt);
//        Serial.print("/dt_sec=");
//        Serial.print(dt_sec);
//        Serial.print("/stepsPerSec=");
//        Serial.print(stepsPerSec);
//        Serial.print("/velocity=");
//        Serial.print(velocity);
//      }

      //if we are at last portion of the lift, and we see a very small velocity, truncate the rest of the values (we assume the lifter finished and re-racked the weight)
      if(i >= rerackIndex && velocity < minVelocity) break;
      
      prevTime = timeSteps[i];
      total += velocity;
      if(velocity > maxVelocity) maxVelocity = velocity;


      if(prevVelocity > 0)
      {
        acceleration = (velocity - prevVelocity) / dt_sec;
        if(acceleration > maxAcceleration) maxAcceleration = acceleration;
        if(debugging)
        {
          Serial.print("velocity=");
          Serial.print(velocity);
          Serial.print("/prevVelocity=");
          Serial.print(prevVelocity);
          Serial.print("/dt_sec=");
          Serial.print(dt_sec);
          Serial.print("/acceleration=");
          Serial.print(acceleration);
        }
      }

      prevVelocity = velocity;
      cnt++;

      if(debugging && (i != timeSteps.size()-1)) Serial.print(", ");
    }
    if(debugging) Serial.println("]");
    
    double avgVelocity = total / cnt;
    Serial.print("Avg velocity = ");
    Serial.print(avgVelocity);
    Serial.println(" (m/s)");
    Serial.print("Peak Velocity: ");
    Serial.print(maxVelocity);
    Serial.println(" (m/s)");
    Serial.print("Peak Acceleration: " );
    Serial.print(maxAcceleration);
    Serial.println(" (m/s^2)");

    //if this is the first rep, clear the initial start message
    if(repCounter == 1) 
    {
      lcd.clear();
      lcd.setCursor(0, 0);
    }
    //1 row can fit 3 reps, if it's the 4th, set the cursor to the second row
    if(repCounter == 4) lcd.setCursor(0, 1);
    //the LCD display can hold a maximum of 6
    if(repCounter <= 6)
    {
      lcd.print(avgVelocity);
      lcd.print(",");
    }
    
    avgVelocityValues.push_back(avgVelocity);
    maxVelocityValues.push_back(maxVelocity);
    maxAccelerationValues.push_back(maxAcceleration);
  }
  else if(debugging) Serial.println("calculateVelocity() called but there were no timesteps");

  Serial.println("");
  timeSteps.clear();
  lastCounter = 0;
}


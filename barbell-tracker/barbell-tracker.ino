#include <StandardCplusplus.h>
#include <vector>

#define ENC_A 14
#define ENC_B 15
#define ENC_PORT PINC

using namespace std;

//max and min integer values
const int MAX_VALUE = 32767;
const int MIN_VALUE = -32768;

/**
 * TODO
 * - change time from millis to micros
 * - add a condition for the completion of the rep
 *     before it was if(went down minimum distance), now lets do: if(went down minimum distance OR minimum time has elasped without going up)
 *     this is for the last rep, where the lifter will rack the weight, and the rep will not be calculated because there is no more downward movement
 * - figure out why encoder steps are being missed. 
 *     Could be the cheap encoder, or possibly too much CPU time spent on code, before the next read.
 *     Try using interupts instead of polling. See example: http://playground.arduino.cc/Main/RotaryEncoders#Example3
 * - test lcd display writing
 */

int repCounter = 1;

/**
 * the amount of turns of the rotary encoder per cm of the string pulled
 */
const int TICKS_PER_CM = 4.098;


/**
 * the minimum amount of cm the string must move in the opposite direction to be determined as switched direction
 */
int minDistance = 30 * TICKS_PER_CM;

/**
 * Variables that keep track of the most recent minimum and maximum counter value, and the time at which they were hit.
 */
int minValue = MAX_VALUE;
int maxValue = MIN_VALUE;



/**
 * counter is incremented as the string is pulled, and decremented as the string retracts
 */
int counter = 0;
/**
 * the previous counter value at which a time was recorded
 */
int lastCounter = 0;

/**
 * the value at which each time sample is taken.
 */
int counterInterval = 5;

/**
 * A list of time values taken every <counterInterval> counter steps
 */
vector<long> timeSteps;
vector<double> avgVelocityValues;
vector<double> maxVelocityValues;

/**
 * true if the string is retracting (the lifter is in the concentric phase of the exercise)
 */
bool stringRetracting = true;

/**
 * true if the string is being pulled (the lifter is in the eccentric phase of the exercise)
 */
bool stringPulling = false;

/**
 * holds the temporary value read from the quadrature encoder, -1, 0, or 1
 * where -1 is clockwise, 0 is invalid state, 1 is counter-clockwise
 */
int8_t encoderStep;
 
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
}
 
void loop()
{
  encoderStep = read_encoder();
  if(encoderStep == 0) Serial.println("STEP MISSED");
  if(encoderStep) 
  {
//    Serial.print("Counter value: ");
//    Serial.println(counter, DEC);
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
        if(stringPulling) timeSteps.push_back(millis());
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
        stringRetracting = false;
        stringPulling = true;
        //RESET counter because inaccuraces with encoder readings may cause mis-steps, and possibly negative counter values.
        counter = 0;
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
        calculateVelocity();
        stringRetracting = true;
        stringPulling = false;
        minValue = MAX_VALUE;
        maxValue = MIN_VALUE;
      }
    }
    
    
  }
}
 
/* returns change in encoder state (-1,0,1) */
int8_t read_encoder()
{
  static int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
  static uint8_t old_AB = 0;
  /**/
  old_AB <<= 2;                   //remember previous state
  old_AB |= ( ENC_PORT & 0x03 );  //add current state
  return ( enc_states[( old_AB & 0x0f )]);
}

void calculateVelocity()
{
  //remove the last time step, as the lifter will probably stop for breaths at the top, and it may read a new time step much later
  timeSteps.pop_back();
  
  Serial.print("Rep ");
  Serial.print(repCounter);
  Serial.println(":");
  repCounter++;
  
  if(timeSteps.size() > 0)
  {

    double total = 0, maxVelocity = 0;
    long prev = timeSteps[0];
    int cnt = 0;
    for(int i = 1; i < timeSteps.size(); i++)
    {
      double dt = timeSteps[i] - prev;
      if(dt == 0) continue;
      double stepsPerSec = counterInterval * 1000.0 / dt;
      double metersPerSec = (stepsPerSec / TICKS_PER_CM) / 100.0;
      prev = timeSteps[i];
      total += metersPerSec;
      if(metersPerSec > maxVelocity) maxVelocity = metersPerSec;
      cnt++;
    }
    
    double avgVelocity = total / cnt;
    Serial.print("Rep complete. Avg velocity = ");
    Serial.print(avgVelocity);
    Serial.print(" (m/s)");
    Serial.print(",\t Peak Velocity: ");
    Serial.print(maxVelocity);
    Serial.println(" (m/s)");
    
    avgVelocityValues.push_back(avgVelocity);
    maxVelocityValues.push_back(maxVelocity);
    
  }

  Serial.println("");
  timeSteps.clear();
  lastCounter = 0;
}


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
 * - need to sample velocity for the full range of motion, if we cut out 1 ft from the top and bottom, that cuts out a majority of the data points
 *    I suggest setting a small minimum threashold of movement (say 5cm) before time points are recorded
 *    I also suggest setting a minimum distance moved to be considered a rep, and possibly allow the user to specify this.
 *    
 * - test lcd display writing
 */

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
int counterInterval = 10;

/**
 * A list of time values taken every <counterInterval> counter steps
 */
vector<long> timeSteps;
vector<double> tempVelocity;

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
  if(encoderStep) 
  {
    //Serial.print("Counter value: ");
    //Serial.println(counter, DEC);
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
  //TODO

  Serial.println("Rep:");
  if(timeSteps.size() > 0)
  {

    Serial.print("timeSteps values = [");
    long prev = timeSteps[0];
    for(int i = 1; i < timeSteps.size(); i++)
    {
      Serial.print(timeSteps[i]);
      if(i != timeSteps.size() - 1) Serial.print(", ");
    }
    Serial.println("]");

    Serial.print("dt values = [");
    prev = timeSteps[0];
    for(int i = 1; i < timeSteps.size(); i++)
    {
      double dt = timeSteps[i] - prev;
      if(dt == 0) continue;
      double stepsPerMillis = counterInterval / dt;
      Serial.print(dt);
      if(i != timeSteps.size() - 1) Serial.print(", ");
      prev = timeSteps[i];
    }
    Serial.println("]");
    
    Serial.print("stepsPerMillis values = [");
    prev = timeSteps[0];
    for(int i = 1; i < timeSteps.size(); i++)
    {
      double dt = timeSteps[i] - prev;
      if(dt == 0) continue;
      double stepsPerMillis = counterInterval / dt;
      Serial.print(stepsPerMillis);
      if(i != timeSteps.size() - 1) Serial.print(", ");
      prev = timeSteps[i];
    }
    Serial.println("]");


    
  }

  Serial.println("");
  tempVelocity.clear();
  timeSteps.clear();
  lastCounter = 0;
}


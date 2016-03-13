/* Rotary encoder read example */
#define ENC_A 14
#define ENC_B 15
#define ENC_PORT PINC

/**
 * TODO
 * keep track of the time at which the string reaches the min/max counter values
 * implement calculateVelocity function
 */

/**
 * the amount of turns of the rotary encoder per cm of the string pulled
 */
const int CM_PER_TURN = 4.098;
const int MAX_VALUE = 10000;
const int MIN_VALUE = -10000;

/**
 * the minimum amount of cm the string must move downwards to be determined as a rep that was started
 */
int minDistanceDown = 60;
int minDistanceUp = 10;

/**
 * Variables that keep track of the most recent minimum and maximum counter value
 */
int minValue = MAX_VALUE;
int maxValue = MIN_VALUE;

/**
 * true if the string is retracting (the lifter is in the concentric phase of the exercise)
 */
bool stringRetracting = true;

/**
 * true if the string is being pulled (the lifter is in the eccentric phase of the exercise)
 */
bool stringPulling = false;

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
}
 
void loop()
{
  /**
   * counter is incremented as the string is pulled, and decremented as the string retracts
   */
  static int counter = 0;
 
  encoderStep = read_encoder();
  if(encoderStep) 
  {
    Serial.print("Counter value: ");
    Serial.println(counter, DEC);
    counter += encoderStep;

    if(counter < minValue) minValue = counter;
    if(counter > maxValue) maxValue = counter;
    
    //if the string is retracting and we notice an upward movement
    if(stringRetracting && encoderStep > 0)
    {
      //the distance moved from the bottom
      int diff = counter - minValue;
      //if the distance moved upwards is greater than the minimum required, we conclude the exercise is now in the upwards (eccentric) phase
      if(diff > minDistanceUp)
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
      if(diff > minDistanceDown)
      {
        stringRetracting = true;
        stringPulling = false;
        calculateVelocity(minValue, maxValue, 0l);
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

void calculateVelocity(int min, int max, long time)
{
  //TODO
}


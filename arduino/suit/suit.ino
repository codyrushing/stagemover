#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#include <Adafruit_NeoPixel.h>
#define LEDPIN 9

Adafruit_NeoPixel strip = Adafruit_NeoPixel(6, LEDPIN, NEO_GRB + NEO_KHZ800);
/* Assign a unique ID to this sensor at the same time */
Adafruit_LSM303_Mag_Unified mag = Adafruit_LSM303_Mag_Unified(12345);
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);


const uint8_t smoothFactor = 5;
uint8_t currPixel = 0;
// accelerometer data stores
int accXVals[smoothFactor];
int accYVals[smoothFactor];
int accZVals[smoothFactor];
int magXVals[smoothFactor];
int magYVals[smoothFactor];
int magZVals[smoothFactor];

long lastToggleTime = 0;
int toggleInterval = 300;
long accLastToggleTime = 0;
int accToggleInterval = 1000;

sensors_event_t event;

// https://learn.adafruit.com/led-tricks-gamma-correction/the-quick-fix
const uint8_t PROGMEM gamma[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

void setup() 
{
  Serial.begin(9600); 
  
  /* Enable auto-gain */
  //mag.enableAutoRange(true);  
  
  // Try to initialise and warn if we couldn't detect the chip
  if (!mag.begin() )
  {
    Serial.println("Oops ... unable to initialize the LSM303. Check your wiring!");
    while (1);
  }


  /* Initialise the sensor */
  if(!accel.begin())
  {
    /* There was a problem detecting the LSM303 ... check your connections */
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while(1);
  }
  
  // init strip
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  // set zeroes in all accelerometer data stores
  for(uint8_t i=0; i<smoothFactor; i++){
    accXVals[i]=0;
    accYVals[i]=0;
    accZVals[i]=0;
    magXVals[i]=0;
    magYVals[i]=0;
    magZVals[i]=0;
  }
  
  sensor_t sensor;
  mag.getSensor(&sensor);
  accel.getSensor(&sensor);
  
}

void pushVal(int vals[], int val){
  for(uint8_t i=1; i<smoothFactor; i++){
    vals[i-1] = vals[i];
  }
  vals[smoothFactor-1] = val;
}

int getAvg(int vals[]) {
  int total = 0;
  for(uint8_t i=0; i<smoothFactor; i++){
    total = total + vals[i];
  }
  return (int) total/smoothFactor;
}

void setCorrectedColor(uint8_t pixel, uint8_t r, uint8_t g, uint8_t b){
  strip.setPixelColor(pixel, pgm_read_byte(&gamma[r]), pgm_read_byte(&gamma[g]), pgm_read_byte(&gamma[b]));
}

void getAccelData(){
  /* Get a new sensor event */ 
  sensors_event_t event; 
  accel.getEvent(&event);
  
  pushVal(accXVals, (int) event.acceleration.x);
  pushVal(accYVals, (int) event.acceleration.y);
  pushVal(accZVals, (int) event.acceleration.z);
  
}

float getJerk(int vals[]){
  float avg = getAvg(vals);
  float dev = 0;
  for(uint8_t i=0; i<smoothFactor; i++){
    dev += abs( vals[i] - avg );
  }
  return (float) dev/smoothFactor;
}

void getMagData(){
  /* Get a new sensor event */ 
  sensors_event_t event; 
  mag.getEvent(&event);

  pushVal(magXVals, (int) event.magnetic.x);
  pushVal(magYVals, (int) event.magnetic.y);
  pushVal(magZVals, (int) event.magnetic.z); 
}

uint8_t getColorValue(int accData, int jerk){
  Serial.println(jerk);
  return (uint8_t) map( 
    constrain( abs(accData) * (0.25 + constrain(jerk, 0, 10)), 0, 30),
    0, 
    50, 
    25, 
    255
  );
}

void fadePixel(uint8_t i, uint8_t toR, uint8_t toG, uint8_t toB){
  uint32_t c = strip.getPixelColor(i);
  uint8_t
    fromR = (uint8_t)(c >> 16),
    fromG = (uint8_t)(c >>  8),
    fromB = (uint8_t) c;
    
  uint8_t cycles = 10;
  uint8_t cycleIndex = 0;
    
  int fadeInterval = (int) toggleInterval / cycles;
  long lastFadeTime = 0;
  while(cycleIndex < cycles){
    if(millis() - lastFadeTime > fadeInterval){
      lastFadeTime = millis();

      cycleIndex++;
      
      // do color stuff
      setCorrectedColor( i, map(cycleIndex, 1, cycles, fromR, toR), map(cycleIndex, 1, cycles, fromG, toG), map(cycleIndex, 1, cycles, fromB, toB) );
      
    }
  }
  
}

void loop() {
  
  getAccelData();
  getMagData();
  
//  if(millis() - accLastToggleTime > accToggleInterval){
//    accLastToggleTime = millis();
//    Serial.print("Accel X: "); Serial.print(getAvg(accXVals)); Serial.print(" ");
//    Serial.print("Y: "); Serial.print(getAvg(accYVals));       Serial.print(" ");
//    Serial.print("Z: "); Serial.println(getAvg(accZVals));     Serial.print(" ");
//    Serial.print("Mag X: "); Serial.print(getAvg(magXVals));     Serial.print(" ");
//    Serial.print("Y: "); Serial.print(getAvg(magYVals));         Serial.print(" ");
//    Serial.print("Z: "); Serial.println(getAvg(magZVals));       Serial.print(" ");  
//  }
  
  // set toggleInterval based on Z orientation
  toggleInterval = (int) map( constrain(getAvg(magZVals), -80, 10), -80, 10, 120, 80 );
  
  if(millis() - lastToggleTime > toggleInterval){
    lastToggleTime = millis();
    uint8_t trailingPixel = currPixel > 0 ? currPixel-1 : strip.numPixels()-1;
    uint8_t fadeOutPixel = trailingPixel > 0 ? trailingPixel-1 : strip.numPixels()-1;
  
    uint8_t
      movementR = getColorValue(getAvg(accXVals), getJerk(accXVals)),
      movementG = getColorValue(getAvg(accYVals), getJerk(accYVals)),
      movementB = getColorValue(getAvg(accZVals), getJerk(accZVals));
  
    for(uint8_t i=0; i<strip.numPixels(); i++){
      uint8_t r = 0;
      uint8_t g = 0;
      uint8_t b = 0;
      
      if(i == currPixel){
        // full color
        r = movementR;
        g = movementG;
        b = movementB;
      } else if(i == trailingPixel){
        // trailing color
        r = (int) movementR/2;
        g = (int) movementG/2;
        b = (int) movementB/2;
      } else if(i == fadeOutPixel){
        r = (int) movementR/4;
        g = (int) movementG/4;
        b = (int) movementB/4;      
      }

      setCorrectedColor(i, r, g, b);
      strip.show();
    }
      
    currPixel++;
    if(currPixel == strip.numPixels()){
      currPixel = 0;
    }
    
  }
}

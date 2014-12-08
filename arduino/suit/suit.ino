#include <Wire.h>
#include <Adafruit_LSM303.h>
#include <Adafruit_NeoPixel.h>
#define LEDPIN 9
Adafruit_LSM303 lsm;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(6, LEDPIN, NEO_GRB + NEO_KHZ800);

const uint8_t smoothFactor = 10;
uint8_t currPixel = 0;
// accelerometer data stores
short accXVals[smoothFactor];
short accYVals[smoothFactor];
short accZVals[smoothFactor];
short magXVals[smoothFactor];
short magYVals[smoothFactor];
short magZVals[smoothFactor];

long lastToggleTime = 0;
int toggleInterval = 100;
int accLastToggleTime = 0;
int accToggleInterval = 250;

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
  // Try to initialise and warn if we couldn't detect the chip
  if (!lsm.begin())
  {
    Serial.println("Oops ... unable to initialize the LSM303. Check your wiring!");
    while (1);
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
}

void pushVal(short vals[], short val){
  for(uint8_t i=1; i<smoothFactor; i++){
    vals[i-1] = vals[i];
  }
  vals[smoothFactor-1] = val;
}

short getAvg(short vals[]) {
  int total = 0;
  for(uint8_t i=0; i<smoothFactor; i++){
    total = total + vals[i];
  }
  return (short) total/smoothFactor;
}

void setCorrectedColor(uint8_t pixel, uint8_t r, uint8_t g, uint8_t b){
  strip.setPixelColor(pixel, pgm_read_byte(&gamma[r]), pgm_read_byte(&gamma[g]), pgm_read_byte(&gamma[b]));
}

void getMovementData(){
  lsm.read();
  
  pushVal(accXVals, (short) lsm.accelData.x);
  pushVal(accYVals, (short) lsm.accelData.y);
  pushVal(accZVals, (short) lsm.accelData.z);
  pushVal(magXVals, (short) lsm.magData.x);
  pushVal(magYVals, (short) lsm.magData.y);
  pushVal(magZVals, (short) lsm.magData.z);
  
//  if(millis() - accLastToggleTime > accToggleInterval){
//    accLastToggleTime = millis();
//    Serial.print("Accel X: "); Serial.print(getAvg(accXVals)); Serial.print(" ");
//    Serial.print("Y: "); Serial.print(getAvg(accYVals));       Serial.print(" ");
//    Serial.print("Z: "); Serial.println(getAvg(accZVals));     Serial.print(" ");
//    Serial.print("Mag X: "); Serial.print(getAvg(magXVals));     Serial.print(" ");
//    Serial.print("Y: "); Serial.print(getAvg(magYVals));         Serial.print(" ");
//    Serial.print("Z: "); Serial.println(getAvg(magZVals));       Serial.print(" ");    
//  }

}

uint8_t getColorValue(short magData, short accData){
  return (uint8_t) map( constrain(magData, -400, 700), -700, 700, 0, map( constrain(accData, -2000, 2000), -2000, 2000, 150, 255 ) );
}

void loop() {
  
  getMovementData();
  
  if(millis() - lastToggleTime > toggleInterval){
    lastToggleTime = millis();
    uint8_t prevPixel = currPixel > 0 ? currPixel-1 : strip.numPixels()-1;
  
    for(uint8_t i=0; i<strip.numPixels(); i++){
      uint8_t r = 0;
      uint8_t g = 0;
      uint8_t b = 0;
      if(i == currPixel){
        // full color
        r = getColorValue(getAvg(magXVals), getAvg(accXVals));
        g = getColorValue(getAvg(magYVals), getAvg(accYVals));
        b = getColorValue(getAvg(magZVals), getAvg(accZVals));
      } else if(i == prevPixel){
        // trailing color
        r = (uint8_t) getColorValue( getAvg(magXVals), getAvg(accXVals) / 2 );
        g = (uint8_t) getColorValue( getAvg(magYVals), getAvg(accYVals) / 2 );
        b = (uint8_t) getColorValue( getAvg(magZVals), getAvg(accZVals) / 2 );
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

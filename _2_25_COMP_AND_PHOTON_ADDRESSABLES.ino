#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

//PIN DATA
int strip1Pin = 10;
int strip2Pin = 15;
Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(60, strip1Pin, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(60, strip2Pin, NEO_GRB + NEO_KHZ800);

//LED DATA
int ledLiftNums = 50;
int ledShoulderNums = 60;

//Serial Byte Reading
int currentByte = 0;
int beginString = 33-48; //!
int sevenSerial[7] = {0, 0, 0, 0, 0, 0, 0};
int primaryBit = 3;
int secondaryBit = 4;
int shoulderBit = 5;
bool reassigned = true;
/*
 * Bits 0, 1, 2 are analog bits for proportionality measurement.
 * Bit 3 is PRIMARY COLOR
 * Bit 4 is SECONDARY COLOR
 * Bit 5 is SHOULDER COLOR
 * Bit 6 is MODE
 */

//Color Values
int rVals[] = {0, 0,   0,   250, 170, 219, 255, 0, 0, 0};
int gVals[] = {0, 250, 0,   0,   170, 215, 255, 0, 0, 0};
int bVals[] = {0, 0,   250, 0,   170,  0,   255, 0, 0, 0};
int rPrime = 0; int gPrime = 0; int bPrime = 0;
int rSecond = 0; int gSecond = 0; int bSecond = 0;
int rShoulder = 0; int gShoulder = 0; int bShoulder = 0;

//PROCESSING VALUES
int analogVal = 0;
int targetLed = 0;
bool flashOn = true;
int counterFlash = 0;
int counterFall = 50;

bool troubleShootOn = true;

bool recvRIO = true;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  strip1.begin();
  strip1.setBrightness(250);
  strip1.show(); // Initialize all pixels to 'off'

    strip2.begin();
  strip2.setBrightness(250);
  strip2.show(); // Initialize all pixels to 'off'

  for(int i = 0; i < 50; i++){
    strip1.setPixelColor(i, 0, 250, 0);
    strip2.setPixelColor(i, 0, 250, 0);
  }
  strip1.show();
  strip2.show();

}

void loop() {
  if(Serial.available() > 0 && recvRIO == true){
    delay(10);
      serialShoot("Serial Rcvd USB");
      recvSerial();
      reassigned = true;
  } else if(Serial.available() > 0 && recvRIO == false){
    serialShoot("USB Command Ignored");
    currentByte = Serial.read();
    clearSerialBuffer();
  }

  if(Serial1.available() > 0){
    serialShoot("Serial Rcvd 1");
    delay(10);
    recvSerial1();
    reassigned = true;
  }

  assignValues();

  if(sevenSerial[6] == 9){
    recvRIO = !recvRIO;
  }
  if(sevenSerial[6] == 8){
    troubleShootOn = !troubleShootOn;
  }

  if(reassigned == true){
    reassigned = false;

    for(int i = 0; i < 7; i ++){
      serialShoot(sevenSerial[i]);
    }

    if(sevenSerial[6] != 6){
      counterFall = 50;
    }
      
    //Check and set to UNSTEPPED Mode
    switch(sevenSerial[6]){
      case 0:
      //LEDS OFF
      ledsOff();
        serialShoot("LEDs Off");
      break;
      case 1:
      //SOLID
      solidLeds(rPrime, gPrime, bPrime, rShoulder, gShoulder, bShoulder);
        serialShoot("Solid LEDs");
      break;
      case 2:
      //Proportionality
      propLeds(targetLed, rPrime, gPrime, bPrime, rSecond, gSecond, bSecond, rShoulder, gShoulder, bShoulder);
        serialShoot("Proportionality");
      break;
      case 3:
      //FLASH SHOULDERS
      flashLeds(false, true, rPrime, gPrime, bPrime, rSecond, gSecond, bSecond, rShoulder, gShoulder, bShoulder);
        serialShoot("Shoulder Flashing");
      break;
      case 4:
      //FLASH LIFT
      flashLeds(true, false, rPrime, gPrime, bPrime, rSecond, gSecond, bSecond, rShoulder, gShoulder, bShoulder);
        serialShoot("Lift Flashing");
      break;
      case 5:
      //FLASH BOTH
      flashLeds(true, true, rPrime, gPrime, bPrime, rSecond, gSecond, bSecond, rShoulder, gShoulder, bShoulder);
        serialShoot("Both Flashing");
      break;
      case 6:
      //FALLING
      propLeds(counterFall, rPrime, gPrime, bPrime, rSecond, gSecond, bSecond, rShoulder, gShoulder, bShoulder);
        serialShoot("Falling LEDs");
      reassigned = true;
      delay(50);
      if(counterFall < 0){
       // counterFall = 50;
      } else {
        counterFall--;
      }
      break;
      case 7:
      //FIESTA MODE
      
      break;
      case 8:
      serialShoot("RIO Turned ON");
      break;
      case 9:
      serialShoot("RIO Turned OFF");
      break;
    }
  }

}



/*////////////////////////////////////////////////////////////
 * SUB PROGRAMS
 /////////////////////////////////////////////////////////////
 */

void recvSerial(){ //If first bit is !, reads next 7 bits. otherwise
  currentByte = Serial.read()-48;
  if(currentByte == beginString){
    for(int i = 0; i<7; i++){
      sevenSerial[i] = Serial.read()-48;
      delay(1);
    }
    if(Serial.peek()-48 != beginString && Serial.available() > 0){
      clearSerialBuffer();
    }
  } else {
    clearSerialBuffer();
  }
  
}

void recvSerial1(){ //If first bit is !, reads next 7 bits. otherwise
  currentByte = Serial1.read()-48;
  if(currentByte == beginString){
    for(int i = 0; i<7; i++){
      sevenSerial[i] = Serial1.read()-48;
      delay(1);
    }
    if(Serial1.peek()-48 != beginString && Serial1.available() > 0){
      clearSerialBuffer1();
    }
  } else {
    clearSerialBuffer1();
  }
  
}

void clearSerialBuffer(){ //reads extra stuff
  serialShoot("Clear Buffer USB");
  while(currentByte != beginString){
      if(Serial.peek()-48 != beginString){
        currentByte = Serial.read()-48;
      } else {
        currentByte = Serial.peek()-48;
      }
      delay(5);
    }
}

void clearSerialBuffer1(){ //reads extra stuff
  serialShoot("Clear Buffer 1");
  while(currentByte != beginString){
      if(Serial1.peek()-48 != beginString){
        currentByte = Serial1.read()-48;
      } else {
        currentByte = Serial1.peek()-48;
      }
      delay(5);
    }
}

void assignValues(){
  rPrime = rVals[sevenSerial[primaryBit]];
  gPrime = gVals[sevenSerial[primaryBit]];
  bPrime = bVals[sevenSerial[primaryBit]];
  
  rSecond = rVals[sevenSerial[secondaryBit]];
  gSecond = gVals[sevenSerial[secondaryBit]];
  bSecond = bVals[sevenSerial[secondaryBit]];

  rShoulder = rVals[sevenSerial[shoulderBit]];
  gShoulder = gVals[sevenSerial[shoulderBit]];
  bShoulder = bVals[sevenSerial[shoulderBit]];

  analogVal = sevenSerial[0]*100 + sevenSerial[1]*10 + sevenSerial[2];
  targetLed = map(analogVal, 0, 255, -1, ledLiftNums-1);
}

void solidLeds(int r1, int g1, int b1, int r3, int g3, int b3){
  for(int i = 0; i < ledLiftNums; i++){
    strip1.setPixelColor(i, r1, g1, b1);
    strip2.setPixelColor(i, r1, g1, b1);
    //strip2.setPixelColor(i, 255, 255, 255);
  }
  for(int i = ledLiftNums; i < ledShoulderNums;  i++){
    strip1.setPixelColor(i, r3, g3, b3);
    strip2.setPixelColor(i, r3, g3, b3);
  }
  strip1.show(); 
  strip2.show();
}

void propLeds(int led, int r1, int g1, int b1, int r2, int g2, int b2, int r3, int g3, int b3){
  if(led != -1){
    for(int i = 0; i < led; i++){
      strip1.setPixelColor(i, r1, g1, b1);
      strip2.setPixelColor(i, r1, g1, b1);
    }
  }
  if(led != ledLiftNums){
    for(int i = led+1; i < ledLiftNums; i++){
      strip1.setPixelColor(i, r2, g2, b2);
      strip2.setPixelColor(i, r2, g2, b2);
    }
  }
  for(int i = ledLiftNums; i < ledShoulderNums; i++){
    strip1.setPixelColor(i, r3, g3, b3);
    strip2.setPixelColor(i, r3, g3, b3);
  }
  strip1.show();
  strip2.show();
}

void flashLeds(bool flashLift, bool flashShoulders, int r1, int g1, int b1, int r2, int g2, int b2, int r3, int g3, int b3){
  reassigned = true;
  if(flashLift == true && flashOn == true){
    for(int i = 0; i < ledLiftNums; i++){
      strip1.setPixelColor(i, r2, g2, b2);
      strip2.setPixelColor(i, r2, g2, b2);
    }
  } else {
    for(int i = 0; i < ledLiftNums; i++){
      strip1.setPixelColor(i, r1, g1, b1);
      strip2.setPixelColor(i, r1, g1, b1);
    }
  }
  if(flashShoulders == true && flashOn == true){
    for(int i = ledLiftNums; i < ledShoulderNums; i++){
      strip1.setPixelColor(i, r2, g2, b2);
      strip2.setPixelColor(i, r2, g2, b2);
    }
  } else {
    for(int i = ledLiftNums; i < ledShoulderNums; i++){
      strip1.setPixelColor(i, r3, g3, b3);
      strip2.setPixelColor(i, r3, g3, b3);
    }
  }
  flashOn = !flashOn;
    strip1.show();
    strip2.show();
    delay(250);
  }



void ledsOff(){
  for(int i = 0; i < 60; i++){
    strip1.setPixelColor(i, 0, 0, 0);
    strip2.setPixelColor(i, 0, 0, 0);
  }
  strip1.show();
  strip2.show();
}

void serialShoot(String currentSend){
  if(troubleShootOn == true){
    Serial.print(currentSend);
    Serial.println(" ");
  }
}


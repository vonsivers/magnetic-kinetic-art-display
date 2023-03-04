
/*
 code for magnetic viewing display based on 2D PCB stepper motor

 created March 2023 
 by Moritz v. Sivers

 */

#include <AccelStepper.h>
#include <MultiStepper.h>
#include <LittleFS.h>      // include the LittleFS library
#include <ESP8266WiFi.h>            // we need wifi to get internet access
#include <time.h>                   // time() ctime()

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

// X-Y position bed driven by 2 steppers
AccelStepper stepperX(AccelStepper::FULL4WIRE, D1, D2, D3, D4);
AccelStepper stepperY(AccelStepper::FULL4WIRE, D0, D6, D7, D8);

// Up to 10 steppers can be handled as a group by MultiStepper
MultiStepper steppers;

#define STBY_PIN D5  // motor controller standby pin

/* Configuration of NTP */
#define MY_NTP_SERVER "de.pool.ntp.org"            // set the best fitting NTP server (pool) for your location
#define MY_TZ "CET-1CEST,M3.5.0,M10.5.0/3"        // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

time_t now;                         // this is the epoch
tm tm;                              // the structure tm holds time information in a more convient way
 
uint8_t lastMinute;                 // save minute when clock was last updated

const int xPosMax = 120;    // max number of steps in x
const int yPosMax = 110;    // max number of steps in y

const int MaxSpeed=100;

const int xDir = 1;
const int yDir = -1;

int xPos=0, yPos=0;     // keep track of current position


// go to home position (lower left)
void homeXY() {
  stepperX.setMaxSpeed(30); // home slowly to prevent bouncing
  stepperY.setMaxSpeed(30);
  long positions[2]; // Array of desired stepper positions
  positions[0] = -xDir*xPosMax;
  positions[1] = -yDir*yPosMax;
  steppers.moveTo(positions);
  digitalWrite(STBY_PIN, HIGH);
  while(steppers.run()) {
    ESP.wdtFeed();  // feed the watchdog to prevent soft WDT reset
  }
  digitalWrite(STBY_PIN, LOW);
  stepperX.setCurrentPosition(0);
  stepperY.setCurrentPosition(0);

}

// draw pattern from gcode file
void drawGCode(String fileName, int offsetX=0) {

  // Open the G-code file for reading
  File gcodeFile = LittleFS.open(fileName, "r");

  // If the file exists, read it and extract X and Y coordinates
  if (gcodeFile) {
    digitalWrite(STBY_PIN, HIGH);
    while (gcodeFile.available()) {
      ESP.wdtFeed();  // feed the watchdog to prevent soft WDT reset
      String line = gcodeFile.readStringUntil('\n');
      if (line.startsWith("G1 X")) {
        long positions[2]; // Array of desired stepper positions
        positions[0] = constrain(line.substring(line.indexOf("X") + 1, line.indexOf("Y")).toInt()+offsetX,0,xPosMax)*xDir;
        positions[1] = (line.substring(line.indexOf("Y") + 1).toInt())*yDir;
        //Serial.print(positions[0]); // delays soft WDT reset
        //Serial.print(", ");
        //Serial.println(positions[1]);
        steppers.moveTo(positions);
        steppers.runSpeedToPosition(); // Blocks until all are in position -> causes soft WDT reset
      }
    }
    digitalWrite(STBY_PIN, LOW);
    gcodeFile.close();          // Close the file
  }
  else {
    Serial.println("Error opening G-code file");
  }

}

// draw patterns from sandify.org
void drawPatterns() {
  stepperX.setMaxSpeed(150);
  stepperY.setMaxSpeed(150);
  drawGCode("erase.gcode");
  delay(1000);
  stepperX.setMaxSpeed(80);
  stepperY.setMaxSpeed(80);
  drawGCode("rectangle.gcode");
  delay(1000);
  stepperX.setMaxSpeed(150);
  stepperY.setMaxSpeed(150);
  drawGCode("erase.gcode");
  delay(1000);
  stepperX.setMaxSpeed(80);
  stepperY.setMaxSpeed(80);
  drawGCode("spiral.gcode");
  delay(1000);
  stepperX.setMaxSpeed(150);
  stepperY.setMaxSpeed(150);
  drawGCode("erase.gcode");
  delay(1000);
  stepperX.setMaxSpeed(80);
  stepperY.setMaxSpeed(80);
  drawGCode("star.gcode");
  delay(1000);
  stepperX.setMaxSpeed(150);
  stepperY.setMaxSpeed(150);
  drawGCode("erase.gcode");
  delay(1000);
  stepperX.setMaxSpeed(80);
  stepperY.setMaxSpeed(80);
  drawGCode("heart.gcode");
  delay(1000);
  stepperX.setMaxSpeed(150);
  stepperY.setMaxSpeed(150);
  drawGCode("erase.gcode");
  delay(1000);
  stepperX.setMaxSpeed(80);
  stepperY.setMaxSpeed(80);
  drawGCode("clover.gcode");
  delay(1000); 
  stepperX.setMaxSpeed(150);
  stepperY.setMaxSpeed(150);
  drawGCode("erase.gcode");
  delay(1000);
  stepperX.setMaxSpeed(80);
  stepperY.setMaxSpeed(80);
  drawGCode("reuleaux.gcode");
  delay(1000);
}

// draw the current time
void updateDisplay() {

  uint16_t dig1 = tm.tm_hour / 10;
  uint16_t dig2 = tm.tm_hour % 10;
  uint16_t dig3 = tm.tm_min / 10;
  uint16_t dig4 = tm.tm_min % 10;

  String file1 = String(dig1) + ".gcode";
  String file2 = String(dig2) + ".gcode";
  String file3 = String(dig3) + ".gcode";
  String file4 = String(dig4) + ".gcode";

  stepperX.setMaxSpeed(100);
  stepperY.setMaxSpeed(100);
  drawGCode("erase_digits.gcode");
  homeXY();   // home after erasing because sometimes the position drifts
  delay(1000);
  stepperX.setMaxSpeed(100);
  stepperY.setMaxSpeed(100);
  drawGCode(file1, 0);
  drawGCode(file2, 25);
  //drawGCode("colon.gcode",0);
  drawGCode(file3, 60);
  drawGCode(file4, 85);
  
}

// runs the display as a clock
void runClock() {
  time(&now);                       // read the current time
  localtime_r(&now, &tm);           // update the structure tm with the current time
  if(tm.tm_min!=lastMinute) {
    updateDisplay();
    lastMinute = tm.tm_min;
  }
  delay(1000);
}

// setup function
void setup() {
  
  // define STBY pins
  pinMode(STBY_PIN, OUTPUT);

  // set all motors to sleep  
  digitalWrite(STBY_PIN, LOW);

  Serial.begin(115200);         // Start serial communication
  delay(100);

  LittleFS.begin();             // Mount the LittleFS file system

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;
  //reset settings - for testing
  //wm.resetSettings();

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //and goes into a blocking loop awaiting configuration
  if (!wm.autoConnect("MagneticClockAP")) {
    Serial.println("failed to connect Wifi and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(1000);
  }

  configTime(MY_TZ, MY_NTP_SERVER); // configure NTP server and timezone

  // Configure each stepper
  stepperX.setMaxSpeed(MaxSpeed);
  stepperY.setMaxSpeed(MaxSpeed);

  // Then give them to MultiStepper to manage
  steppers.addStepper(stepperX);
  steppers.addStepper(stepperY);

  delay(1000);
  
  // homing to lower left position
  homeXY();

  delay(1000);

  stepperX.setMaxSpeed(150);
  stepperY.setMaxSpeed(150);
  drawGCode("erase.gcode");
  delay(1000);

}
  

// code loop
void loop() { 

  //drawPatterns();
  runClock();
         
}

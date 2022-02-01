//This is where you enter the template id from the template you created in the Blynk web console
#define BLYNK_TEMPLATE_ID "TMPLKK0BXbuU"
//This is your template name from the web console
#define BLYNK_DEVICE_NAME "Fertigation Manager"

//You can use this to keep track of your firmware as you make changes
#define BLYNK_FIRMWARE_VERSION        "0.1.5"
#define BLYNK_PRINT Serial
#define APP_DEBUG
#include "BlynkEdgent.h"
#include <HX711.h>

//Here, we are telling the MCU which IO ports are being used for communication with the
//analog to digital converter (HX711) that interfaces with the load cell
const int LOADCELL1_DOUT_PIN = 39;
const int LOADCELL1_SCK_PIN = 14;
HX711 scale1;

const int LOADCELL2_DOUT_PIN = 35;
const int LOADCELL2_SCK_PIN = 32;
HX711 scale2;

const int LOADCELL3_DOUT_PIN = 33;
const int LOADCELL3_SCK_PIN = 25;
HX711 scale3;

const int LOADCELL4_DOUT_PIN = 26;
const int LOADCELL4_SCK_PIN = 12;
HX711 scale4;

//used to compare all timers to Millis()
unsigned long currentMillis = 0;
// for controlling irrigation intervals & weight sampling
unsigned long timer1;
// for updating the coutdown timer
unsigned long timer3;
// for scale calibration mode timeout
unsigned long timer4 = 0;
// for getting and writing all initial values from NVS
unsigned long timer5 = 0;
// for pump calibration mode
unsigned long timer6 = 0;
// for checking max value on scales for overflow protection
unsigned long timer7 = 0;
// displays minutes remaining until next fertigation event
unsigned long countDown;
// displays minutes remaining until next sampling event
unsigned long countDownSample;
// used to control when weight sampling should happen
bool allowWeightSampling = true;
//used to control when weight sampling and automatic adjustments should be done.  I Don't want the first fertigation event to
//be adaptive.
int compleatedFertEvents = 0;
//used to control when drain pumps should enter the automatic pumping mode
int drainPumpMode = 0;
//used to turn off pumps only once after the pump calibration timer has expired
bool turnOffPumps = false;
//turns off the scales if the user forgets to turn them off after calibration
bool turnOffScales = false;
//Used to sync app and hardware on startup and after network interuption, and get all values from NVS on startup
int restoreAndSync = 0;
//used as a switch to turn on or off the "drain after every cycle" feature
bool drainAfterCycle = false;
//used as a switch to turn on and off the "overflow protection" feature
bool checkFullTray = false;
//used to activate a notification that max weight values have been recorded
bool sendNotification = false;
// this is the adjustable variable that is used to set the interval between fertigation events
long fertInterval;
// this is the amount of time to wait after a fertigation event before checking the scales and compare the runoff to the input
long timeToSample = 0;
// these variables are used to tell the pumps how long to run to deliver a specific amount of water
unsigned long pump1RunTime;
unsigned long pump2RunTime;
unsigned long pump3RunTime;
unsigned long pump4RunTime;
// these variables are used to keep track of how much water should be delivered each fertigation cycle
int pump1Ml = 0;
int pump2Ml = 0;
int pump3Ml = 0;
int pump4Ml = 0;
// these variables are used to compare a previous station setpoint to the current station setpoint
int previous1 = 0;
int previous2 = 0;
int previous3 = 0;
int previous4 = 0;
// these are the calibration factors for the delivery pumps
int pump1Cal;
int pump2Cal;
int pump3Cal;
int pump4Cal;
// this is the user defined amount of runoff that they are targeting
int runOffPercent = 10;
// this acts as a switch and lets us control whether we want to weigh runoff after a fertigation cycle or not
bool adaptWater1 = false;
bool adaptWater2 = false;
bool adaptWater3 = false;
bool adaptWater4 = false;
// this lets us manually turn on the load cells and view the weight readings.  Used for scale calibration
bool calMode1 = false;
bool calMode2 = false;
bool calMode3 = false;
bool calMode4 = false;
// these are the scale calibration factors
int scale1CalFactor = 1000;
int scale2CalFactor = 1000;
int scale3CalFactor = 1000;
int scale4CalFactor = 1000;
// these are the IO ports on the MCU that are used for the pumps.  If you are using your own board, make sure to change these to match your hardware
int m1Pin = 23;
int m2Pin = 22;
int m3Pin = 21;
int m4Pin = 19;
int m5Pin = 18;
int m6Pin = 17;
int m7Pin = 16;
int m8Pin = 2;
// this is used to compare scale readings to know when to turn off the drain pumps
int trayWeight;
// used to compare scale readings to runoff targets
int runoff1;
int runoff2;
int runoff3;
int runoff4;
// this is the lower limit for runoff volume
int target1;
int target2;
int target3;
int target4;
// this is the upper limit for runoff volume
int target5;
int target6;
int target7;
int target8;
// used to keep track of the daily total volume of water delivered.  This value is sent to the server once per power cycle.  I use this value to populate my chart
int dTot1;
int dTot2;
int dTot3;
int dTot4;
// these iare for drip tray overflow protection.  Once recorded, they will be used to compare the current weight to a maximum weight.
long scale1Max;
long scale2Max;
long scale3Max;
long scale4Max;
// this is used to adjust the amout of water that is delivered on the first fertigation cycle after the system is powered on
int firstEvent;

/* All of the following "BLYNK_WRITE(Vxx)" are how the you send information from the app to the hardware.
    Each "(Vxx)" represents a button or numeric input in the app.
    Any time a button is pushed or a value is input in the app, these blocks of code are ran.
    Each button and value input in the app has an associated "datastream."
    If a button is pushed that is assigned to the datastream V4, then BLYNK_WRITE(V4) is exicuted once.
*/

BLYNK_WRITE(V4)//turn on pump 5
{
  // this condition ensures that the pump cannot be manually activated while the system is going through its automated draining process
  if (drainPumpMode == 0 || drainPumpMode > 4) {
    // turns on or off the pump
    digitalWrite(m5Pin, param.asInt());
  }
  else {
    // we have to keep the app and hardware in sync.  If this wasn't here and the user tried to activate the pump
    // while drainPumpMode = 2 for example, the button on the app would show the pump in the "on" state, while the pump is actually turned off.
    Blynk.virtualWrite(V4, 0);
  }
}

BLYNK_WRITE(V44)//turn on pump 6. See BLYNK_WRITE(V4), same notes
{
  if (drainPumpMode == 0 || drainPumpMode > 4) {
    digitalWrite(m6Pin, param.asInt());
  }
  else {
    Blynk.virtualWrite(V44, 0);
  }
}

BLYNK_WRITE(V45)//turn on pump 7. See BLYNK_WRITE(V4), same notes
{
  if (drainPumpMode == 0 || drainPumpMode > 4) {
    digitalWrite(m7Pin, param.asInt());
  }
  else {
    Blynk.virtualWrite(V45, 0);
  }
}

BLYNK_WRITE(V46)//turn on pump 8. See BLYNK_WRITE(V4), same notes
{
  if (drainPumpMode == 0 || drainPumpMode > 4) {
    digitalWrite(m8Pin, param.asInt());
  }
  else {
    Blynk.virtualWrite(V46, 0);
  }
}

BLYNK_WRITE(V5)//fertigation interval input
{
  fertInterval = param.asLong();
  // this must be here to keep values of less than 41 minutes from being sent. The fertigation interval must be longer than the sample period.
  if (fertInterval < 41) {
    fertInterval = 41;
  }
  // turn the fertigation interval into milliseconds
  fertInterval = fertInterval * 1000 * 60;
  // record the fertigation interval to NVS
  preferences.putLong("fertInterval", fertInterval);
}

BLYNK_WRITE(V7)//this option will enable/disable going through the automated drip tray draining process when the system is first powered on
{
  drainPumpMode = param.asInt();
  // save the users preference to NVS
  preferences.putInt("drainPumpMode", drainPumpMode);
}

BLYNK_WRITE(V8)// this is the sample period input (the time after a fertigation event to weigh the runoff).
{
  timeToSample = param.asLong();
  if (timeToSample < 3) {
    timeToSample = 3;
  }
  // convert it to milliseconds
  timeToSample = timeToSample * 1000 * 60;
  // record the sample period to NVS
  preferences.putLong("timeToSample", timeToSample);
}

BLYNK_WRITE(V10)//this is the amount to deliver per fertigation event station 1
{
  pump1Ml = param.asInt();
  // record amount to NVS
  preferences.putInt("pump1Ml", pump1Ml);
}

BLYNK_WRITE(V11)//this is the amount to deliver per fertigation event station 2
{
  pump2Ml = param.asInt();
  preferences.putInt("pump2Ml", pump2Ml);
}

BLYNK_WRITE(V12)//this is the amount to deliver per fertigation event station 3
{
  pump3Ml = param.asInt();
  preferences.putInt("pump3Ml", pump3Ml);
}

BLYNK_WRITE(V13)//this is the amount to deliver per fertigation event station 4
{
  pump4Ml = param.asInt();
  preferences.putInt("pump4Ml", pump4Ml);
}

BLYNK_WRITE(V15)//runoff percentage setpoint
{
  runOffPercent = param.asInt();
  if (runOffPercent < 5) {
    runOffPercent = 5;
  }
  preferences.putLong("runOffPercent", runOffPercent);  // record value to NVS
}

BLYNK_WRITE(V16)//turn on or off adaptive watering
{
  adaptWater1 = param.asInt();
  preferences.putBool("adaptWater1", adaptWater1);
}

BLYNK_WRITE(V17)//turn on or off adaptive watering
{
  adaptWater2 = param.asInt();
  preferences.putBool("adaptWater2", adaptWater2);
}

BLYNK_WRITE(V18)//turn on or off adaptive watering
{
  adaptWater3 = param.asInt();
  preferences.putBool("adaptWater3", adaptWater3);
}

BLYNK_WRITE(V19)//turn on or off adaptive watering
{
  adaptWater4 = param.asInt();
  preferences.putBool("adaptWater4", adaptWater4);
}

BLYNK_WRITE(V20)//turn on or off delivery pump. Used for calibration purposes
{
  // turn on or off the pump
  digitalWrite(m1Pin, param.asInt());
  // set a timer that will automatically turn the pump off after 10 seconds
  timer6 = currentMillis;
  // used as a switch to ensure the timer block of code is only executed once
  turnOffPumps = true;
}

BLYNK_WRITE(V21)// see BLYNK_WRITE(V20) for notes
{
  digitalWrite(m2Pin, param.asInt());
  timer6 = currentMillis;
  turnOffPumps = true;
}

BLYNK_WRITE(V22)// see BLYNK_WRITE(V20) for notes
{
  digitalWrite(m3Pin, param.asInt());
  timer6 = currentMillis;
  turnOffPumps = true;
}

BLYNK_WRITE(V23)// see BLYNK_WRITE(V20) for notes
{
  digitalWrite(m4Pin, param.asInt());
  timer6 = currentMillis;
  turnOffPumps = true;
}

BLYNK_WRITE(V24)//calibration factor for each pump
{
  pump1Cal = param.asInt();
  // this prevents values of less than 100 from being used as a calibration factor
  if (pump1Cal < 100) {
    pump1Cal = 100;
  }
  preferences.putInt("pump1Cal", pump1Cal);
}

BLYNK_WRITE(V25)// see BLYNK_WRITE(V24) for notes
{
  pump2Cal = param.asInt();
  if (pump2Cal < 100) {
    pump2Cal = 100;
  }
  preferences.putInt("pump2Cal", pump2Cal);
}

BLYNK_WRITE(V26)// see BLYNK_WRITE(V24) for notes
{
  pump3Cal = param.asInt();
  if (pump3Cal < 100) {
    pump3Cal = 100;
  }
  preferences.putInt("pump3Cal", pump3Cal);
}

BLYNK_WRITE(V27)// see BLYNK_WRITE(V24) for notes
{
  pump4Cal = param.asInt();
  if (pump4Cal < 100) {
    pump4Cal = 100;
  }
  preferences.putInt("pump4Cal", pump4Cal);
}

BLYNK_WRITE(V28)// turn on the scales for calibration purposes
{
  calMode1 = param.asInt();
  // this sets a 10 minute timer, after which the scales will automatically be turned off
  timer4 = currentMillis;
  // this keeps the timer block of code from running over and over again
  turnOffScales = true;
}

BLYNK_WRITE(V29)// tare scales
{
  // we dont want the scales to be set to zero unless they are active in calibration mode
  if (calMode1) {
    scale1.tare(20);
  }
  // in the app, the button mode is set to a switch, so after the tare is complete the switch must be turned off
  Blynk.virtualWrite(V29, LOW);
}

BLYNK_WRITE(V31)// inputs scale calibration factors
{
  scale1CalFactor = param.asInt();
  // this tells the scales to divide their raw reading by the scale calibration factor
  scale1.set_scale(scale1CalFactor);
  // record the calibration factor to NVS
  preferences.putInt("scale1CalFactor", scale1CalFactor);
}

BLYNK_WRITE(V32)// see BLYNK_WRITE(V31) for notes
{
  scale2CalFactor = param.asInt();
  scale2.set_scale(scale2CalFactor);
  preferences.putInt("scale2CalFactor", scale2CalFactor);
}

BLYNK_WRITE(V34)// see BLYNK_WRITE(V31) for notes
{
  scale3CalFactor = param.asInt();
  scale3.set_scale(scale3CalFactor);
  preferences.putInt("scale3CalFactor", scale3CalFactor);
}

BLYNK_WRITE(V35)// see BLYNK_WRITE(V31) for notes
{
  scale4CalFactor = param.asInt();
  scale4.set_scale(scale4CalFactor);
  preferences.putInt("scale4CalFactor", scale4CalFactor);
}

BLYNK_WRITE(V38)// see BLYNK_WRITE(V28) for notes
{
  calMode2 = param.asInt();
  timer4 = currentMillis;
  turnOffScales = true;
}

BLYNK_WRITE(V39)// see BLYNK_WRITE(V28) for notes
{
  calMode3 = param.asInt();
  timer4 = currentMillis;
  turnOffScales = true;
}

BLYNK_WRITE(V40)// see BLYNK_WRITE(V28) for notes
{
  calMode4 = param.asInt();
  timer4 = currentMillis;
  turnOffScales = true;
}

BLYNK_WRITE(V41)// see BLYNK_WRITE(V29) for notes
{
  if (calMode2) {
    scale2.tare(20);
  }
  Blynk.virtualWrite(V41, LOW);
}

BLYNK_WRITE(V42)// see BLYNK_WRITE(V29) for notes
{
  if (calMode3) {
    scale3.tare(20);
  }
  Blynk.virtualWrite(V42, LOW);
}

BLYNK_WRITE(V43)// see BLYNK_WRITE(V29) for notes
{
  if (calMode4) {
    scale4.tare(20);
  }
  Blynk.virtualWrite(V43, LOW);
}

BLYNK_WRITE(V47)// this enables/disables a feature that will drain the drip trays after every fertigation cycle
{
  drainAfterCycle = param.asInt();
  preferences.putBool("drainAfterCycle", drainAfterCycle);
}

BLYNK_WRITE(V52)// this is where the maximum values are recorded for the overflow protection feature
{
  scale1Max = scale1.read_average(10);
  scale2Max = scale2.read_average(10);
  scale3Max = scale3.read_average(10);
  scale4Max = scale4.read_average(10);
  preferences.putLong("scale1Max", scale1Max);
  preferences.putLong("scale2Max", scale2Max);
  preferences.putLong("scale3Max", scale3Max);
  preferences.putLong("scale4Max", scale4Max);
  // if you look at the main loop, if sendNotification = true, a notification is sent to the user that the values have been sucessfully recorded.
  sendNotification = true;
  // turn off "record max weight" button after the weights have been recorded
  Blynk.virtualWrite(V52, LOW);
}

BLYNK_WRITE(V53)// turns the overflow protection feature on and off
{
  checkFullTray = param.asInt();
  preferences.putBool("checkFullTray", checkFullTray);
}

BLYNK_WRITE(V54)// this feature allows the user to adjust the volume of water delivered on the first fertigation cycle after system start up
{
  firstEvent = param.asInt();
  if (firstEvent < 50) {
    firstEvent = 50;
  }
  preferences.putInt("firstEvent", firstEvent);
}

// Create a freeRTOS task to handle all the irrigation logic. This loop will continue to run even if blynk looses connection with the server or wifi.
// If everything contained in this task were to be moved to the main loop, if Blynk lost connection the loop would stop and no fertigation events would
// take place until the connection was re-established.
TaskHandle_t Task1;

void Task1code( void * pvParameters) {
  // create an infinite loop
  for (;;) {
    currentMillis = millis();
    // restoreAndSync will always equal zero on start up, so this block of code will only be ran once. This is for getting values that have been previously stored in NVS
    if (restoreAndSync == 0) {
      delay(500);// I noticed that if I didn't have this short delay here, preferences.get wouldn't work.  Don't know why
      scale1CalFactor = preferences.getInt("scale1CalFactor", 1000);
      scale2CalFactor = preferences.getInt("scale2CalFactor", 1000);
      scale3CalFactor = preferences.getInt("scale3CalFactor", 1000);
      scale4CalFactor = preferences.getInt("scale4CalFactor", 1000);
      scale1.set_scale(scale1CalFactor);
      scale2.set_scale(scale2CalFactor);
      scale3.set_scale(scale3CalFactor);
      scale4.set_scale(scale4CalFactor);
      fertInterval = preferences.getLong("fertInterval", 10800000);
      timeToSample = preferences.getLong("timeToSample", 1200000);
      pump1Ml = preferences.getInt("pump1Ml", 0);
      pump2Ml = preferences.getInt("pump2Ml", 0);
      pump3Ml = preferences.getInt("pump3Ml", 0);
      pump4Ml = preferences.getInt("pump4Ml", 0);
      runOffPercent = preferences.getInt("runOffPercent", 10);
      adaptWater1 = preferences.getBool("adaptWater1", false);
      adaptWater2 = preferences.getBool("adaptWater2", false);
      adaptWater3 = preferences.getBool("adaptWater3", false);
      adaptWater4 = preferences.getBool("adaptWater4", false);
      pump1Cal = preferences.getInt("pump1Cal", 300);
      pump2Cal = preferences.getInt("pump2Cal", 300);
      pump3Cal = preferences.getInt("pump3Cal", 300);
      pump4Cal = preferences.getInt("pump4Cal", 300);
      drainPumpMode = preferences.getInt("drainPumpMode", 0);
      drainAfterCycle = preferences.getBool("drainAfterCycle", false);
      dTot1 = preferences.getInt("dTot1", 0);
      dTot2 = preferences.getInt("dTot2", 0);
      dTot3 = preferences.getInt("dTot3", 0);
      dTot4 = preferences.getInt("dTot4", 0);
      scale1Max = preferences.getLong("scale1Max", 0);
      scale2Max = preferences.getLong("scale2Max", 0);
      scale3Max = preferences.getLong("scale3Max", 0);
      scale4Max = preferences.getLong("scale4Max", 0);
      checkFullTray = preferences.getBool("checkFullTray", false);
      firstEvent = preferences.getInt("firstEvent", 100);
      //for setting the first fertigation event 5 minutes from startup. Is there a better way to do this?
      timer1 = fertInterval - 300000;
      timer1 = timer1 * -1;
      restoreAndSync = 1;
    }

    //This block of code is for converting the station setpoints to a time that the pump needs to run for.
    //When the system is first powered on, compleatedFertEvents = 0, so the pump runtime is calculated here to take into account
    //the adjustment for the first fertigation cycle, if any.
    if (compleatedFertEvents == 0) {
      pump1RunTime = 10000 / pump1Cal * pump1Ml * firstEvent * .01;
      pump2RunTime = 10000 / pump2Cal * pump2Ml * firstEvent * .01;
      pump3RunTime = 10000 / pump3Cal * pump3Ml * firstEvent * .01;
      pump4RunTime = 10000 / pump4Cal * pump4Ml * firstEvent * .01;
    }

    //after the first fertigation cycle, compleatedFertEvents > 0 so the pump runtime is calculated here for every subsequent cycle.
    else {
      pump1RunTime = 10000 / pump1Cal * pump1Ml;
      pump2RunTime = 10000 / pump2Cal * pump2Ml;
      pump3RunTime = 10000 / pump3Cal * pump3Ml;
      pump4RunTime = 10000 / pump4Cal * pump4Ml;
    }

    //when this condition becomes true, a fertigation event takes place
    if (currentMillis - timer1 > fertInterval) {
      //this is to keep a running total of the amount of water delivered per power cycle
      //this section takes into account the fact that the first event might have a different delivery amount than subsequent events
      if (compleatedFertEvents == 0) {
        dTot1 = dTot1 + pump1Ml * firstEvent * .01;
        dTot2 = dTot2 + pump2Ml * firstEvent * .01;
        dTot3 = dTot3 + pump3Ml * firstEvent * .01;
        dTot4 = dTot4 + pump4Ml * firstEvent * .01;
      }
      //this does the same as above except for all events after the first event
      else {
        dTot1 = dTot1 + pump1Ml;
        dTot2 = dTot2 + pump2Ml;
        dTot3 = dTot3 + pump3Ml;
        dTot4 = dTot4 + pump4Ml;
      }
      //save the daily totals to NVS
      preferences.putInt("dTot1", dTot1);
      preferences.putInt("dTot2", dTot2);
      preferences.putInt("dTot3", dTot3);
      preferences.putInt("dTot4", dTot4);
      //tare all scales
      scale1.tare(20);
      scale2.tare(20);
      scale3.tare(20);
      scale4.tare(20);
      //run all pumps in sequential order
      digitalWrite(m1Pin, HIGH);
      delay(pump1RunTime);
      digitalWrite(m1Pin, LOW);
      digitalWrite(m2Pin, HIGH);
      delay(pump2RunTime);
      digitalWrite(m2Pin, LOW);
      digitalWrite(m3Pin, HIGH);
      delay(pump3RunTime);
      digitalWrite(m3Pin, LOW);
      digitalWrite(m4Pin, HIGH);
      delay(pump4RunTime);
      digitalWrite(m4Pin, LOW);
      //reset the interval timer
      timer1 = currentMillis;
      //fertigation event is over so allow weight sampling
      allowWeightSampling = true;
      //keep track of which fertigation cycle the system is in
      compleatedFertEvents++;
    }

    //the following is the process of weighing the runoff and automatically adjusting the station setpoints.
    if (currentMillis - timer1 > timeToSample && allowWeightSampling && compleatedFertEvents > 1) {
      //If the runoff collected after a fertigation cycle is less than the user defined runoff percentage, the station setpoint for that
      //station is increased by 10%.  If the runoff collected is above the runoff setpoint + 10 percentage points, the station setpoint
      //is reduced by 2%.
      target1 = runOffPercent * .01 * pump1Ml;
      target2 = runOffPercent * .01 * pump2Ml;
      target3 = runOffPercent * .01 * pump3Ml;
      target4 = runOffPercent * .01 * pump4Ml;
      target5 = (runOffPercent + 10) * .01 * pump1Ml;
      target6 = (runOffPercent + 10) * .01 * pump2Ml;
      target7 = (runOffPercent + 10) * .01 * pump3Ml;
      target8 = (runOffPercent + 10) * .01 * pump4Ml;

      //the runoff should only be weighed if the adaptive feature is turned on, and if the station setpoint is larger than 0
      if (adaptWater1 && pump1Ml > 0) {
        runoff1 = scale1.get_units(20);
        if (runoff1 < target1) {
          pump1Ml = pump1Ml * 1.1;
          preferences.putInt("pump1Ml", pump1Ml);
        }
        if (runoff1 > target5) {
          pump1Ml = pump1Ml * .98;
          preferences.putInt("pump1Ml", pump1Ml);
        }
      }

      if (adaptWater2 && pump2Ml > 0) {
        runoff2 = scale2.get_units(20);
        if (runoff2 < target2) {
          pump2Ml = pump2Ml * 1.1;
          preferences.putInt("pump2Ml", pump2Ml);
        }
        if (runoff2 > target6) {
          pump2Ml = pump2Ml * .98;
          preferences.putInt("pump2Ml", pump2Ml);
        }
      }

      if (adaptWater3 && pump3Ml > 0) {
        runoff3 = scale3.get_units(20);
        if (runoff3 < target3) {
          pump3Ml = pump3Ml * 1.1;
          preferences.putInt("pump3Ml", pump3Ml);
        }
        if (runoff3 > target7) {
          pump3Ml = pump3Ml * .98;
          preferences.putInt("pump3Ml", pump3Ml);
        }
      }

      if (adaptWater4 && pump4Ml > 0) {
        runoff4 = scale4.get_units(20);
        if (runoff4 < target4) {
          pump4Ml = pump4Ml * 1.1;
          preferences.putInt("pump4Ml", pump4Ml);
        }
        if (runoff4 > target8) {
          pump4Ml = pump4Ml * .98;
          preferences.putInt("pump4Ml", pump4Ml);
        }
      }
      //sampling is over so don't allow it to continue
      allowWeightSampling = false;
      //drain the drip trays if this is enabled
      if (drainAfterCycle) {
        drainPumpMode = 1;
      }
    }

    //this block of code is to activate the drain pumps after the first fertigation cycle that is non-adaptive,
    //if enabled in drain pump settings.
    if (currentMillis - timer1 > timeToSample && allowWeightSampling && compleatedFertEvents == 1) {
      if (drainAfterCycle) {
        drainPumpMode = 1;
      }
      allowWeightSampling = false;
    }

    //this keeps a display on the app updated with how many minutes remain in the fertigation interval
    countDown = currentMillis - timer1;
    countDown = fertInterval - countDown;
    countDown = countDown / 60000;

    //this is to keep a display on the app updated with how many minutes remain until the scales are sampled for runoff weight.
    //the reason I included this if statement is because the first fertigation event is "non-adaptive."
    //I want the displat to stay at 0 until after the second fertigation event
    if (allowWeightSampling && compleatedFertEvents > 1) {
      countDownSample = currentMillis - timer1;
      countDownSample = timeToSample - countDownSample;
      countDownSample = countDownSample / 60000;
    }

    //when drainPumpMode equals 1, the automated drip tray draining cycle begins, and progresses sequentially from station 1 to 4.
    if (drainPumpMode == 1) {
      //I dont want the drain pump to activate if that station is not in use (station setpoint set to 0)
      if (pump1Ml > 0) {
        digitalWrite(m5Pin, HIGH);
        //take a weight reading, which takes about a second (blocking code)
        trayWeight = scale1.get_units(20);
        //take another reading and compare it to the first reading.
        //once the first and second readings are within 10 grams of each
        //other (the pump has either finished pumping the trays or has become clogged), turn the pump off.
        if (trayWeight - scale1.get_units(20) < 10) {
          digitalWrite(m5Pin, LOW);
          //move on to the next drip tray
          drainPumpMode = 2;
        }
      }
      else {
        drainPumpMode = 2;
      }
    }

    if (drainPumpMode == 2) {
      if (pump2Ml > 0) {
        digitalWrite(m6Pin, HIGH);
        trayWeight = scale2.get_units(20);
        if (trayWeight - scale2.get_units(20) < 10) {
          digitalWrite(m6Pin, LOW);
          drainPumpMode = 3;
        }
      }
      else {
        drainPumpMode = 3;
      }
    }

    if (drainPumpMode == 3) {
      if (pump3Ml > 0) {
        digitalWrite(m7Pin, HIGH);
        trayWeight = scale3.get_units(20);
        if (trayWeight - scale3.get_units(20) < 10) {
          digitalWrite(m7Pin, LOW);
          drainPumpMode = 4;
        }
      }
      else {
        drainPumpMode = 4;
      }
    }

    if (drainPumpMode == 4) {
      if (pump4Ml > 0) {
        digitalWrite(m8Pin, HIGH);
        trayWeight = scale4.get_units(20);
        if (trayWeight - scale4.get_units(20) < 10) {
          digitalWrite(m8Pin, LOW);
          drainPumpMode = 5;
        }
      }
      else {
        //exits the drip tray draining process
        drainPumpMode = 5;
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);
  BlynkEdgent.begin();
  //this is called in blynk's library, but I found that if I didn't have it here my preferences.get wouldn't work.
  preferences.begin("blynk", false);
  //create task1
  xTaskCreatePinnedToCore(
    Task1code,   /* Task function. */
    "Task1",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Task1,      /* Task handle to keep track of created task */
    1);          /* pin task to core 1 */
  delay(500);
  pinMode(m1Pin, OUTPUT); //MOTOR 1
  pinMode(m2Pin, OUTPUT); //MOTOR 2
  pinMode(m3Pin, OUTPUT); //MOTOR 3
  pinMode(m4Pin, OUTPUT); //MOTOR 4
  pinMode(m5Pin, OUTPUT); //MOTOR 5
  pinMode(m6Pin, OUTPUT); //MOTOR 6
  pinMode(m7Pin, OUTPUT); //MOTOR 7
  pinMode(m8Pin, OUTPUT); //MOTOR 8
  //pinMode(13, OUTPUT); //heater
  digitalWrite(m1Pin, LOW);
  digitalWrite(m2Pin, LOW);
  digitalWrite(m3Pin, LOW);
  digitalWrite(m4Pin, LOW);
  digitalWrite(m5Pin, LOW);
  digitalWrite(m6Pin, LOW);
  digitalWrite(m7Pin, LOW);
  digitalWrite(m8Pin, LOW);
  //digitalWrite(13, LOW);//heater
  scale1.begin(LOADCELL1_DOUT_PIN, LOADCELL1_SCK_PIN);//start the scales
  scale1.set_scale(scale1CalFactor);
  scale2.begin(LOADCELL2_DOUT_PIN, LOADCELL2_SCK_PIN);
  scale2.set_scale(scale2CalFactor);
  scale3.begin(LOADCELL3_DOUT_PIN, LOADCELL3_SCK_PIN);
  scale3.set_scale(scale3CalFactor);
  scale4.begin(LOADCELL4_DOUT_PIN, LOADCELL4_SCK_PIN);
  scale4.set_scale(scale4CalFactor);
}

void loop() {

  BlynkEdgent.run();

  //this keeps the app synced with the hardware.  If a change is made in the app while the system is either turned off
  //or not connected to the server, when it is powered on or reconnects, I want the hardware to update the app with the current values.
  if (Blynk.connected() == true && restoreAndSync == 1) {
    Blynk.virtualWrite(V4, 0);//updates manual drain pump to "off"
    Blynk.virtualWrite(V44, 0);//updates manual drain pump to "off"
    Blynk.virtualWrite(V45, 0);//updates manual drain pump to "off"
    Blynk.virtualWrite(V46, 0);//updates manual drain pump to "off"
    Blynk.virtualWrite(V5, fertInterval / 60000);
    Blynk.virtualWrite(V8, timeToSample / 60000);
    Blynk.virtualWrite(V9, 0);//Sets sample countdown display to zero when device is powered on.
    Blynk.virtualWrite(V10, pump1Ml);
    Blynk.virtualWrite(V11, pump2Ml);
    Blynk.virtualWrite(V12, pump3Ml);
    Blynk.virtualWrite(V13, pump4Ml);
    Blynk.virtualWrite(V15, runOffPercent);
    Blynk.virtualWrite(V16, adaptWater1);
    Blynk.virtualWrite(V17, adaptWater2);
    Blynk.virtualWrite(V18, adaptWater3);
    Blynk.virtualWrite(V19, adaptWater4);
    Blynk.virtualWrite(V20, 0);//sets pump cal mode to off when device is powered on
    Blynk.virtualWrite(V21, 0);//sets pump cal mode to off when device is powered on
    Blynk.virtualWrite(V22, 0);//sets pump cal mode to off when device is powered on
    Blynk.virtualWrite(V23, 0);//sets pump cal mode to off when device is powered on
    Blynk.virtualWrite(V24, pump1Cal);
    Blynk.virtualWrite(V25, pump2Cal);
    Blynk.virtualWrite(V26, pump3Cal);
    Blynk.virtualWrite(V27, pump4Cal);
    Blynk.virtualWrite(V28, 0);//scale 1 cal mode set to off when device is powered on
    Blynk.virtualWrite(V29, 0);//scale 1 tare button set to off when device is powered on
    Blynk.virtualWrite(V31, scale1CalFactor);
    Blynk.virtualWrite(V32, scale2CalFactor);
    Blynk.virtualWrite(V34, scale3CalFactor);
    Blynk.virtualWrite(V35, scale4CalFactor);
    Blynk.virtualWrite(V7, drainPumpMode);//for drain on start up
    Blynk.virtualWrite(V47, drainAfterCycle);//for drain after every cycle
    Blynk.virtualWrite(V38, 0);//scale 2 cal mode set to off when device is powered on
    Blynk.virtualWrite(V39, 0);//scale 3 cal mode set to off when device is powered on
    Blynk.virtualWrite(V40, 0);//scale 4 cal mode set to off when device is powered on
    Blynk.virtualWrite(V41, 0);//scale 2 tare button set to off when device is powered on
    Blynk.virtualWrite(V42, 0);//scale 3 tare button set to off when device is powered on
    Blynk.virtualWrite(V43, 0);//scale 4 tare button set to off when device is powered on
    Blynk.virtualWrite(V48, dTot1);//send the previous days total nutrient solution consumption to the server so the chart can be populated
    Blynk.virtualWrite(V49, dTot2);
    Blynk.virtualWrite(V50, dTot3);
    Blynk.virtualWrite(V51, dTot4);
    Blynk.virtualWrite(V52, 0);//set "record max weight" to off when device powers on
    Blynk.virtualWrite(V53, checkFullTray);//writes overflow protection to last known value
    Blynk.virtualWrite(V54, firstEvent);
    dTot1 = 0;//reset the daily total to 0 so the current day can start accumulating
    dTot2 = 0;
    dTot3 = 0;
    dTot4 = 0;
    restoreAndSync = 2;
  }

  //if the hardware looses connection with the server, I want the above section of code to run again so the app and hardware stay in sync
  if (Blynk.connected() == false) {
    restoreAndSync = 1;
  }

  //this sends a notification that max weights have been recorded
  if (sendNotification) {
    Blynk.logEvent("record_max_weight");
    sendNotification = false;
  }

  //checkFullTray is the switch that activates the overflow protection feature, and I want to check the scales for weight every 1 minute while this feature is turned on
  if (checkFullTray && currentMillis - timer7 > 60000) {
    //if the reading on the scale is greater than the max weight recorded by the user,
    if (scale1.read_average(10) > scale1Max) {
      //set the station setpoint to 0 so no further irrigation events occur so as to not overflow the drip trays
      pump1Ml = 0;
      //keep the app updated with hardware
      Blynk.virtualWrite(V10, pump1Ml);
      //send a notification that the drip trays are full
      Blynk.logEvent("station_1_tray_full");
    }
    else {
      //once the full drip tray has been emptied by the user, reset the station setpoint to its previous value.
      pump1Ml = preferences.getInt("pump1Ml", 0);
      //update the app
      Blynk.virtualWrite(V10, pump1Ml);
    }

    if (scale2.read_average(10) > scale2Max) {
      pump2Ml = 0;
      Blynk.virtualWrite(V11, pump2Ml);
      Blynk.logEvent("station_2_tray_full");
    }
    else {
      pump2Ml = preferences.getInt("pump2Ml", 0);
      Blynk.virtualWrite(V11, pump2Ml);
    }

    if (scale3.read_average(10) > scale3Max) {
      pump3Ml = 0;
      Blynk.virtualWrite(V12, pump3Ml);
      Blynk.logEvent("station_3_tray_full");
    }
    else {
      pump3Ml = preferences.getInt("pump3Ml", 0);
      Blynk.virtualWrite(V12, pump3Ml);
    }

    if (scale4.read_average(10) > scale4Max) {
      pump4Ml = 0;
      Blynk.virtualWrite(V13, pump4Ml);
      Blynk.logEvent("station_4_tray_full");
    }
    else {
      pump4Ml = preferences.getInt("pump4Ml", 0);
      Blynk.virtualWrite(V13, pump4Ml);
    }
    //reset the 1 minute timer
    timer7 = currentMillis;
  }

  //this updates the countdown display oin the app once every five seconds
  if (currentMillis - timer3 > 5000) {
    timer3 = currentMillis;
    Blynk.virtualWrite(V1, countDown);
    Blynk.virtualWrite(V9, countDownSample);
  }

  //if an automatic adjustment is made in task1, this updates the app.  I was just updating the values at a regular interval along with the
  //countdown above, but I found that if I was in the process of making a change to a station setpoint when the interval expired and the value was
  //sent, whatever I was typing in on the app was overwritten by the server sending the value to my phone. So this is the ugly way I get around that.
  if (pump1Ml != previous1) {
    Blynk.virtualWrite(V10, pump1Ml);
    previous1 = pump1Ml;
  }
  if (pump2Ml != previous2) {
    Blynk.virtualWrite(V11, pump2Ml);
    previous2 = pump2Ml;
  }
  if (pump3Ml != previous3) {
    Blynk.virtualWrite(V12, pump3Ml);
    previous3 = pump3Ml;
  }
  if (pump4Ml != previous4) {
    Blynk.virtualWrite(V13, pump4Ml);
    previous4 = pump4Ml;
  }

  //this turns the delivery pumps off after 10 seconds during the pump calibration process.
  if (currentMillis - timer6 > 10000 && turnOffPumps) {
    digitalWrite(m1Pin, LOW);
    digitalWrite(m2Pin, LOW);
    digitalWrite(m3Pin, LOW);
    digitalWrite(m4Pin, LOW);
    Blynk.virtualWrite(V20, 0);
    Blynk.virtualWrite(V21, 0);
    Blynk.virtualWrite(V22, 0);
    Blynk.virtualWrite(V23, 0);
    turnOffPumps = false;
  }

  //this is for scale calibration mode
  if (calMode1) {
    Blynk.virtualWrite(V30, scale1.get_units(20));
  }

  if (calMode2) {
    Blynk.virtualWrite(V33, scale2.get_units(20));
  }

  if (calMode3) {
    Blynk.virtualWrite(V36, scale3.get_units(20));
  }

  if (calMode4) {
    Blynk.virtualWrite(V37, scale4.get_units(20));
  }

  //if a user forgets to turn off a scale after calibration, this will automatically turn it off after 10 minutes
  if (currentMillis - timer4 > 600000 && turnOffScales) {
    Blynk.virtualWrite(V28, 0);
    Blynk.virtualWrite(V38, 0);
    Blynk.virtualWrite(V39, 0);
    Blynk.virtualWrite(V40, 0);
    calMode1 = false;
    calMode2 = false;
    calMode3 = false;
    calMode4 = false;
    turnOffScales = false;
  }
}

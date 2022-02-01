# Fertigation-Manager
Arduino firmware program that automates the drain to waste hydroponic method
The Fertigation Manager is an irrigation controller that automates the drain-to-waste hydroponic method of growing plants.  This method consists of deliveing a known amount of nutrient soulution to a
soilless media, and having between 5% and 30% of that delivered solution end up as runoff. This method relies on multiple irrigation events per day. The hardware consists of a custom esp32 based PCB, 8 small
dc pumps, and 4 load cells (scales).  The logic is this:  Deliver a known amount of water to each station....allow some time to pass for runoff to collect in drip trays that sit on scales.....weigh the runoff and
compare it to the input to determine the percentage of delivered water that ended up as runoff......adjust the station setpoint (delivery volume) if the collected runoff is above or below the user defined amount.
In addition to the basic interval timer functionality, the system has many variables that can be adjusted by the user.  The use of Blynk is strictly for adjusting
variable values, and for monitoring the performance of the system.  I do not use or rely on Blynk's servers to store any variable values.  The whole system must continue to function regardless of connection
status with Blynk's servers.  The main loop handles anything that has to do with Blynk.  A second "loop" also runs on a different core of the esp32 using a task setup with freeRTOS.  The freeRTOS loop is
where the irrigation logic is handled.  First, I will describe what happens in the freeRTOS (irrigation logic) loop.

Note: the plants being grown are not in contact with the scales, they are suspended above the scales and drip trays.

1.) A freeRTOS task is setup that creates "task1."
2.) The first thing that happens in task1 is recovery of all variable values from NVS.  This should only happen once per power cycle, and I use variable and an if statement to accomplish this.
  The last thing to happen in this if statement is the manipulation of a timer so that the first "fertigation event" happens 5 minutes after startup.
3.) Next, after 5 minutes have passed, the first fertigation event takes place.  There are 3 main things that happen during a fertigation event.  First, all scales are set to zero (tare). Second, 4 pumps
  are ran (delivery pumps) in sequential order for a predetermined amount of time to deliver a specific amount of water.  Third, a timer is set for the weighing of the runoff.
4.) After the fertigation event, some time must pass to allow for water to collect in the drip trays.  I refer to this as the "sample period." Once the user defined sample period has passed, each scale
   is read (depending on if certain conditions are met).  The scale reading is then compared to the amount of water that was delivered in the fertigation event.  If the amount collected is above/below
   the user defined "runoff setpoint", the controller with then increase or decrease how much it delivers on the next fertigation event.
5.) The last thing to happen in this loop is the drip tray draining process.  The 4 remaining pumps (drain pumps) are activated in sequential order.  Immediately after the first pump is activated, the scale
   for that station is read.  Next, the scale read again.  Since taking a reading takes about 1 second, I compare the first reading to the second reading.  If the difference between the two readings is
   greater than 10 grams, this indicates that the drain pump is still moving water and should continue to pump.  If the reading is less than 10 grams, this indicates that the pump is no longer pumping
   and should be turned off.  The next drain pump is then activated and the process continues.

Next, I will describe in general the things that happen in the main loop.

1.) The first thing that happens is the process of keeping the hardware and the app synchronized.  If a change is made in the app while the hardware is turned off or not connected to the server,
    when the hardware comes back online it needs to update the server with it's current values.  I want the hardware to tell the server what the values should be, not the server telling the hardware
    what to set the values to.
2.) The scales are checked once a minute to make sure that the weight on them does not exceed a maximum value that was recorded in BLYNK_WRITE(V52).  This is a feature that will alert the user to a potentially
    dangerous situation.  Since the drain pump logic relies on checking scales for a stable value to know when the trays are emptied, it can't differentiate between an empty drip tray and a clogged pump.  If a
    drain pump becomes clogged, the runoff will start accumulating in the drip tray and will eventually overflow and cause a mess, or water damage.  This "overflow protection" feature can be enabled/disabled
    through manipulation of the variable "h."  In the event that the weight on the scale exceeds the maximum value recorded, the station setpoint will automatically be set to zero, and the user will be sent a
    notification and an email once per hour.  To restore the system to normal operation, when the excessive weight is removed from the scale, the previous station setpoint will be restored.
3.) 2 count down timer values are sent to the server at a regular interval.  This is to update a display on the app that shows users how many minutes remain until the next fertigation event, and the sample period.
4.) Next, if a station setpoint is automatically changed by the controller following a fertigation event, the updated value for that station is sent to the server.  This is how the user keeps track of what the
    system is doing.  The station setpoint inputs on the app act as both an input and a display.
5.) The last two things that happen in this loop are for calibration purposes.  The feed pumps must be calibrated so the controller knows how much water it is delivering.  This is accomplished by the user
    manually activating a feed pump.  When the user does this, a 10 second timer is started.  at the end of 10 seconds, the controller turns off the pump.  The user then weighs how much water was delivered
    in that 10 second timeframe and enters that number into the app as a calibration factor.  Now that the controller knows how much liquid the pump can move in 10 seconds, it can determine how long to run to
    deliver a specific amount of liquid.  The very last portion of this loop is for the scale calibration.  The scales can be manually turned on by the user, and their value can be seen on a display in the app.
    The user places a known weight on the scale, and then adjusts the scale calibration factor until the display on the app reads the correct value of the object.

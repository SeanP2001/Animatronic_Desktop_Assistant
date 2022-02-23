/*
 *  Animatronic Desktop Robot
 *  Sean Price
 *  13/04/2021
 */

#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


//------------------------------------------P R O G R A M   V A R I A B L E S------------------------------------------------
LiquidCrystal_I2C lcd(0x27,16,2);           // 1602 LCD Setup

//OLED DISPLAY SETUP
#define SCREEN_WIDTH 128                                                   // OLED display width, in pixels
#define SCREEN_HEIGHT 32                                                   // OLED display height, in pixels
#define OLED_RESET     4                                                   // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C                                                //< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  // initialise OLED display

Servo arm;                                  // servo name: arm

// BUTTONS
int minusButton = 7;                        // minus button pin number
int startStop = 3;                          // start/stop button pin number
int plusButton = 2;                         // plus button pin number

int minusButtonState;                       // Current state of the minus button
int lastMinusButtonState = LOW;             // The last state of the minus button

int startButtonState;                       // Current state of the start button
int lastStartButtonState = LOW;             // The last state of the start button

int plusButtonState;                        // Current state of the plus button
int lastPlusButtonState = LOW;              // The last state of the plus button

unsigned long lastMinusDebounceTime = 0;    // When the button changed state
unsigned long lastStartDebounceTime = 0;    // When the button changed state
unsigned long lastPlusDebounceTime = 0;     // When the button changed state
unsigned long debounceDelay = 50;           // debounce time

//TIMER
boolean running = false;                    // Not running

unsigned long maxTime = 7200000;            // maximum allowed timer (ms)
unsigned long defaultTime = 1500000;        // default timer length (ms)
unsigned long timeAdjust = 300000;          // +/- adjustment (ms)
unsigned long time = defaultTime;           // current timer (ms)
int seconds = 0;                            // seconds
int mins = 0;                               // minutes
int hrs = 0;                                // hours

//ARM
int servo_position = 0;                     // this variable controls the arm position

//NOTIFICATIONS
int notifNo = 0;                            // the number of unread notifications
String notif = "";                          // The notification text
bool newData = false;                       // a new notification has been received

int x = 0;                                  // stores the randomly generated eye blink number
 
//--------------------------------------------------F U N C T I O N S--------------------------------------------------------
void checkMinusButton();                    // Checks the minus button
void checkStartStopButton();                // Checks the start/stop button
void checkPlusButton();                     // Checks the plus button

void startTimer();                          // start the timer
void stopTimer();                           // stop the timer
void resetTimer();                          // reset the timer
void minusTime();                           // takes time away from the timer
void plusTime();                            // adds time to the timer

void displayTime(int time);                 // format and display the time

void waveArm();                             // wave the robot's arm

void checkNotif();                          // check if there is a notification
void printNotif();                          // print notification info
void dismissNotif();                        // dismisses the notification

void blinkEyes();                           // blinks the robot's eyes
void eyesClosed();                          // shows closed eyes
void eyesOpen();                            // shows open eyes

//------------------------------------------------------S E T U P------------------------------------------------------------
void setup() 
{
  pinMode(minusButton, INPUT);              // set minus button to an input
  pinMode(startStop, INPUT);                // set start/stop button to an input
  pinMode(plusButton, INPUT);               // set plus button to an input

  arm.attach (8);                           // arm servo on pin 8

  lcd.init();                               // initialise the LCD
  lcd.backlight();                          // Turn on the LCD Backlight
  
  Serial.begin(9600);                       // Begin serial communication with the PC


  //SETUP OLED DISPLAY
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) 
  {
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.display();
}

//-------------------------------------------------------M A I N-------------------------------------------------------------
void loop() 
{
  displayTime(time);                          // display the time
  eyesOpen();                                 // start with the eyes open
  while(1)                                    // then loop the following forever
  {
    blinkEyes();                              // blink the eyes
    checkStartStopButton();                   // check the start/stop button
    checkNotif();                             // check for incoming notifications
    
    if(!running)                              // if the timer is not running
    {
      if(time >= timeAdjust)                  // if pressing the minus button will not push the timer below 0
        checkMinusButton();                   // check the minus button
      if(time <= (maxTime - timeAdjust))      // if pressing the plus button will not push the timer above the max
      	checkPlusButton();                    // check the plus button
         
      if ((newData == true) && time > 0)      // if there is a notification waiting to be shown and the timer is not in the completed phase (but is not running)
      {
        printNotif();                         // display the notification
      }
    }
  
    if(running)                               // if the timer is running
    {
      if (time>0)                             // and it has not reached 0
      {
        time = time - 1000;                   // take 1 second from the timer
        delay(1000);                          // wait 1 second
      }
      else                                    // when the timer reaches 0
      {
        lcd.setCursor(0,1);
        lcd.print("Timer Complete  ");        // print "Timer Complete"
      }
      displayTime(time);                      // display the new time
    }
  
    if((time <= 0) || ((!running) && notifNo > 0))  // if the timer completes or there is a notification when the timer isn't running
      waveArm();                                    // wave the robot's arm
  }
}

//------------------------------------------------C H E C K   S T A R T-----------------------------------------------------
//Check the start button (WITH DEBOUNCE)
void checkStartStopButton()
{
  int start = digitalRead(startStop);                                                           // save the current button state
  
  if (start != lastStartButtonState)                                                            // if the button state has changed 
  {
    lastStartDebounceTime = millis();                                                           // save the time it changed
  }

  if ((millis() - lastStartDebounceTime) > debounceDelay)                                       // if the debounce delay time has passed since the last state change 
  { 
    if (start != startButtonState)                                                              // if the button state has changed
    {
      startButtonState = start;                                                                 // save the current button state

      if((digitalRead(startStop) == HIGH) && time <= 0)                                         // If the button is pressed and the timer is complete
        resetTimer();                                                                           // reset the timer
      else if((digitalRead(startStop) == HIGH) && (!running) && notifNo > 0)                    // If the button is pressed, the timer isn't running and there is a notification
        dismissNotif();                                                                         // dismiss the notification
      else if((digitalRead(startStop) == HIGH) && (!running))                                   // If the button is pressed and it's not running
      {
        while(((digitalRead(startStop)) == HIGH) && (millis() - lastStartDebounceTime) < 3100)  // see how long it was pressed for
        {
          
        }
        if ((millis() - lastStartDebounceTime) > 3000)                                          // If it was pressed for more than 3 seconds
          resetTimer();                                                                         // reset the timer
        else                                                                                    // If it was a short press
          startTimer();                                                                         // Start the timer
      }   
      else if((digitalRead(startStop) == HIGH) && (running))                                    // If the button is pressed and it's running
        stopTimer();                                                                            // Stop the timer      
    }
  } 
  lastStartButtonState = start;                                                                 // save the button state
}

//------------------------------------------------C H E C K   M I N U S-----------------------------------------------------
//Check the minus button (WITH DEBOUNCE)
void checkMinusButton()
{
  int minus = digitalRead(minusButton);                         // save the current button state
  
  if (minus != lastMinusButtonState)                            // if the button state has changed 
  {
    lastMinusDebounceTime = millis();                           // save the time it changed
  }

  if ((millis() - lastMinusDebounceTime) > debounceDelay)       // if the debounce delay time has passed since the last state change 
  { 
    if (minus != minusButtonState)                              // if the button state has changed
    {
      minusButtonState = minus;                                 // save the current button state
      if(digitalRead(minusButton) == HIGH)                      // If the button is pressed
        minusTime();                                            // take time from the timer
    }
  } 
  lastMinusButtonState = minus;                                 // save the button state
}

//------------------------------------------------C H E C K   P L U S-----------------------------------------------------
//Check the plus button (WITH DEBOUNCE)
void checkPlusButton()
{
  int plus = digitalRead(plusButton);                           // save the current button state
  
  if (plus != lastPlusButtonState)                              // if the button state has changed 
  {
    lastPlusDebounceTime = millis();                            // save the time it changed
  }

  if ((millis() - lastPlusDebounceTime) > debounceDelay)        // if the debounce delay time has passed since the last state change 
  { 
    if (plus != plusButtonState)                                // if the button state has changed
    {
      plusButtonState = plus;                                   // save the current button state
      if(digitalRead(plusButton) == HIGH)                       // If the button is pressed
        plusTime();                                             // add time to the timer
    }
  } 
  lastPlusButtonState = plus;                                   // save the button state
}

//------------------------------------------------------S T A R T------------------------------------------------------------
// Start timer
void startTimer()
{
  running = true;                              // mark the timer as running
  lcd.setCursor(0,1);
  lcd.print("Timer Started   ");                
  displayTime(time);                           // display the time
}

//-------------------------------------------------------S T O P-------------------------------------------------------------
// Stop timer
void stopTimer()
{
  running = false;                               // mark the timer as not running
  lcd.setCursor(0,1);
  lcd.print("Timer Stopped   "); 
  displayTime(time);                             // display the time
}

//------------------------------------------------------R E S E T------------------------------------------------------------
void resetTimer()
{
  running = false;                      // mark as not running
  lcd.setCursor(0,1);
  lcd.print("Timer Reset     "); 
  time = defaultTime;                   // reset the timer
  displayTime(time);

  delay(3000);                          // wait 3 seconds

  lcd.setCursor(0,1);                   // leave the botttom line blank
  lcd.print("                ");
  
  if (notifNo > 0)                      // If there are any notifications
    printNotif();                       // display them
}
//-------------------------------------------------P L U S   F I V E-----------------------------------------------------
void plusTime()
{
  time = time + timeAdjust;                      // add to the timer
  displayTime(time);                             // display the time
}  

//------------------------------------------------M I N U S   F I V E-----------------------------------------------------
void minusTime()
{
  time = time - timeAdjust;                      // take from the timer
  displayTime(time);                             // display the time
}

//-----------------------------------------------D I S P L A Y   T I M E-----------------------------------------------------
// display the Time 
void displayTime(unsigned long time)
{
  seconds = time / 1000;                         // convert ms to seconds
   
  while (seconds >= 60)                          // convert time over 60 seconds into minutes
  {
    mins = seconds / 60;
    seconds = seconds - (60 * mins);
  }
  if (time < 60000)
  {
    mins = 0;
  }
  
  while (mins >= 60)                             // convert time over 60  minutes into hours
  {
    hrs = mins / 60;
    mins = mins - (60 * hrs);
  }
  if (time < 3600000)
  {
    hrs = 0;
  }


  // PRINT TIME
  lcd.setCursor(4,0);
  if(hrs<10)lcd.print("0");                      // always 2 digits
  lcd.print(hrs);                                //Print Hours
  lcd.print(":");
  if(mins<10)lcd.print("0");
  lcd.print(mins);                               //Print Mins
  lcd.print(":");
  if(seconds<10)lcd.print("0");
  lcd.print(seconds);                            //Print Secs
}

//--------------------------------------------------W A V E   A R M--------------------------------------------------------
void waveArm() 
{
  for (servo_position = 0; servo_position <=90; servo_position++)      // move the arm 90 deg clockwise
  {
    arm.write(servo_position);
    delay(10);
  }

  for (servo_position=90; servo_position >= 0; servo_position--)       // move the arm 90 deg anti-clockwise
  {
    arm.write(servo_position);
    delay(10);
  }
}

//--------------------------------------------------C H E C K   N O T I F I C A T I O N--------------------------------------------------------
void checkNotif() 
{
  if (Serial.available() > 0)                      // if there has been a notification
  {
    notif = Serial.readString();                   // Save the message
    newData = true;                                // Flag new data
    notifNo++;                                     // increase notification num
  }
}

//--------------------------------------------------P R I N T   N O T I F I C A T I O N--------------------------------------------------------
void printNotif()
{
  lcd.setCursor(0, 1);                            // clear the bottom line 
  lcd.print("                ");
  
  lcd.setCursor(0, 1);   
  for(int i = 0; i < (notif.length()-1); i++)     // (printed as chars to avoid printing the newline char)
  {   
    lcd.print(notif[i]);                          // print the notification text (left aligned)
  }

  lcd.setCursor(12, 1);                           // on the right
  lcd.print(" ");
  
  lcd.print(notifNo);                             // print the number of unread notifications

  if (notifNo < 10)                               // fix spacing 
  {
    lcd.print("  ");
  }
  if (notifNo < 100)
  {
    lcd.print(" ");
  }

  newData = false;                                // the new data has been printed
}

//--------------------------------------------------D I S M I S S   N O T I F I C A T I O N--------------------------------------------------------
void dismissNotif()
{
  lcd.setCursor(0, 1);                            // Clear the message on screen
  lcd.print("                ");

  notifNo = 0;                                    // Set the number of notifications to 0
}

//------------------------------------------------B L I N K   E Y E S-----------------------------------------------------
void blinkEyes()
{
  if ((millis() % 100) == 0)                      // every 100 ms
  { 
    x = rand() % 40;                              // there is a 1 in 40 chance
    display.clearDisplay();
    if (x == 5)
      eyesClosed();                               // that the eyes will close
    else                                          // otherwise
     eyesOpen();                                  // they remain open
  }
}

//------------------------------------------------E Y E S   C L O S E D-----------------------------------------------------
void eyesClosed()
{
  display.fillRect(18, 15, 28, 3, SSD1306_WHITE); // Left Eye 
  display.fillRect(82, 15, 28, 3, SSD1306_WHITE); // Right Eye 
  display.display();
}

//------------------------------------------------E Y E S   O P E N-----------------------------------------------------
void eyesOpen()
{
  display.drawCircle(32, 16, 14, SSD1306_WHITE);  // Left Eye Outline
  display.fillCircle(32, 16, 3, SSD1306_WHITE);   // Left Eye Pupil
  display.drawCircle(96, 16, 14, SSD1306_WHITE);  // Right Eye Outline
  display.fillCircle(96, 16, 3, SSD1306_WHITE);   // Right Eye Pupil
  display.display();
}

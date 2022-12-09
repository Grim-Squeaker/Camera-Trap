#include <TimerOne.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ClickEncoder.h>
#include "LowPower.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int currentMenu = 0;
int currentMenuSize = 7;
int menuitem = 0;
int frame = 1;
int lastMenu = 0;
bool oLedOn = true;
//MainMenuItems
String mainMenu[7] = {"NumShots:", "Delay:","BtwShots:","Sensor:", "Night:","Cutoff:", "<Start>"};//change number of elements in loop
//CameraMenuItems

boolean up = false;
boolean down = false;
boolean middle = false;

const int OnBoardLedPin = 13;

//CamTrap Variables
const int WakeUpPin = 3;
const int CameraTriggerPin = 9;
const int CameraHalfPressPin = 8;
const int PhotoresistorPin = A3;
int numShots = 1;
int stDelay = 0;
int btwDelay = 2000;
bool lightSen = false;
bool nightOnly = false;
int lightCutoffNum = 200;
bool refreshDisplay = true;


bool isStartup = true;
bool isTriggered = false;

ClickEncoder *encoder;
int16_t last, value;

void setup() {
  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    for(;;); // Don't proceed, loop forever
  }

  encoder = new ClickEncoder(A1,A2,A0); //DT, CLK, SW
  encoder->setAccelerationEnabled(false);
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds
  // Clear the buffer
  display.clearDisplay();
  display.setTextWrap(false);
  display.display();

  pinMode(WakeUpPin, INPUT);
  pinMode(CameraTriggerPin, OUTPUT);
  pinMode(OnBoardLedPin, OUTPUT);
  
  delay(2000);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr); 
  last = encoder->getValue();
  
}

void loop() {  
  if(oLedOn){
    
    //Serial.println(analogRead(PhotoresistorPin));
    //oLedCurrentTime++;
    /*if(oLedCurrentTime > oLedTimer){
      display.ssd1306_command(SSD1306_DISPLAYOFF);
      oLedOn = false;
    }*/
    if(currentMenu == 0 && refreshDisplay){
      Main_Menu();
      refreshDisplay = false;
    }
      readRotaryEncoder();
    ClickEncoder::Button b = encoder->getButton();
       if (b != ClickEncoder::Open) {
       switch (b) {
        
          case ClickEncoder::Clicked:
             middle=true;
            break;
        }
      }
    
    if(up){
      up = false;
      switch(currentMenu){
        case 0:
          if(menuitem < currentMenuSize-1){
          menuitem++;
          refreshDisplay =true;
          }
          break;
        case 1:
          numShots++;
          displayIntMenuPage(mainMenu[0], numShots);
          break;
        case 2:
            stDelay += 500;
            displayIntMenuPage(mainMenu[1], stDelay);
          break;
        case 3:
            btwDelay += 500;
            displayIntMenuPage(mainMenu[2], btwDelay);
          break;
        case 4:
            lightCutoffNum += 10;
            displayIntMenuPage(mainMenu[5], lightCutoffNum);
          break;
      }
    }
    if(down){
      down = false;
      
      switch(currentMenu){
        case 0:
          if(menuitem >= 1){
            menuitem--;
            refreshDisplay =true;
          }
          break;
        case 1:
          if(numShots >0){numShots--;}
          displayIntMenuPage(mainMenu[0], numShots);
          break;
        case 2:
    
       displayIntMenuPage(mainMenu[1], stDelay);
          break;
        case 3:
          if(btwDelay > 0){btwDelay -= 500;}
          displayIntMenuPage(mainMenu[2], btwDelay);
          break;
        case 4:
          if(lightCutoffNum > 0){
            lightCutoffNum -= 10;
            displayIntMenuPage(mainMenu[5], lightCutoffNum);
          }
          break;
          }
    }
    if (middle) //Middle Button is Pressed
      {
        middle = false;
        if(currentMenu != 0){
          menuitem = 0;
          currentMenu = 0;
          refreshDisplay =true;
        }
        else{
          switch(menuitem){
            case 0:
              currentMenu = 1;
              displayIntMenuPage(mainMenu[0], numShots);
              break;
            case 1:
              currentMenu = 2;
              displayIntMenuPage(mainMenu[1], stDelay);
              break;
            case 2:
              currentMenu = 3;
              displayIntMenuPage(mainMenu[2], btwDelay);
              break;  
            case 3:
              lightSen = !lightSen;
              refreshDisplay =true;
              break;
            case 4:
              nightOnly = !nightOnly;
              refreshDisplay =true;
              break;
            case 5:
              currentMenu = 4;
              displayIntMenuPage(mainMenu[5], lightCutoffNum);
              break;
            case 6:
              display.ssd1306_command(SSD1306_DISPLAYOFF);
              oLedOn = false;
              break;
          }
        }
       }
    }   
  else{
    if(middle){
      middle = false;
      display.ssd1306_command(SSD1306_DISPLAYON);
      oLedOn = true;
      }
     if(isStartup){
          int var = 0;
          while(var < 30){
            digitalWrite(OnBoardLedPin, HIGH);
            delay(500);  
            digitalWrite(OnBoardLedPin,LOW);
            delay(500);
            var++;
          }
      isStartup = false;
      digitalWrite(OnBoardLedPin,LOW);
    }
      bool PIRvalue = digitalRead(WakeUpPin);
      if(isTriggered == false){
        Serial.println("Going To Sleep");
        Serial.flush();
        attachInterrupt(1, wakeUp, HIGH);
        LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
        detachInterrupt(1);
      }
      else{
          CheckLighting();
      }
  }
}

void Main_Menu(){
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.println("Main Menu");

  //Switch to move cursor through main menu
  switch(menuitem){
      case 0:
            displayMenuFieldInt(mainMenu[0], true, numShots,2, false);
            displayMenuFieldInt(mainMenu[1], false, stDelay, 3, false);
            displayMenuFieldInt(mainMenu[2], false, btwDelay, 4, true);
            break;
          case 1:
            displayMenuFieldInt(mainMenu[0], false, numShots, 2, false);
            displayMenuFieldInt(mainMenu[1], true, stDelay, 3, false);
            displayMenuFieldInt(mainMenu[2], false, btwDelay, 4, true);
            break;
          case 2:
            displayMenuFieldInt(mainMenu[0], false, numShots, 2, false);
            displayMenuFieldInt(mainMenu[1], false,stDelay, 3, false);
            displayMenuFieldInt(mainMenu[2], true, btwDelay, 4, true);
            break;
          case 3:
            displayMenuFieldInt(mainMenu[1], false, stDelay, 2, false);
            displayMenuFieldInt(mainMenu[2], false, btwDelay, 3, true);
            displayMenuFieldBool(mainMenu[3], true, lightSen, 4, false);
            break;
          case 4:
            displayMenuFieldInt(mainMenu[2], false, btwDelay, 2, true);
            displayMenuFieldBool(mainMenu[3], false, lightSen, 3, false);
            displayMenuFieldBool(mainMenu[4], true, nightOnly, 4, false);
            break;
          case 5:
            displayMenuFieldBool(mainMenu[3], false, lightSen, 2, false);
            displayMenuFieldBool(mainMenu[4], false, nightOnly, 3, false);
            displayMenuFieldInt(mainMenu[5], true, lightCutoffNum, 4, false);
            break;
          case 6:
            displayMenuFieldBool(mainMenu[4], false, nightOnly, 3, false);
            displayMenuFieldInt(mainMenu[5], false, lightCutoffNum, 4, false);
            display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
            display.println(mainMenu[6]);
            break;
  }
  display.display();
}


void displayMenuFieldInt(String item, bool selected, int var, int page, bool scroll){
   if(selected)
    {
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
      if(scroll){
        switch(page){
            case 2:
              display.startscrollleft(0x02, 0x03);
            break;
            case 3:
              display.startscrollleft(0x04, 0x05);
            break;
            case 4:
              display.startscrollleft(0x06, 0x07);
            break;
            }
            }
            else{
              display.stopscroll();
            }
    }else
    {
      display.setTextColor(SSD1306_WHITE);
    }
    display.print(item);
    display.println(var);
}

void displayMenuItem(String _item, boolean selected)
{
  if(selected)
    {
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    }else
    {
      display.setTextColor(SSD1306_WHITE);
    }
    display.println(_item);
}
void displayMenuFieldBool(String item, boolean selected, boolean value,int page, bool scroll)
{
  if(selected){
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
      if(scroll){
        switch(page){
            case 2:
              display.startscrollleft(0x02, 0x03);
            break;
            case 3:
              display.startscrollleft(0x04, 0x05);
            break;
            case 4:
              display.startscrollleft(0x06, 0x07);
            break;
            }
            }
            else{
              display.stopscroll();
            }
      }
    else{
      display.setTextColor(SSD1306_WHITE);
      
    }
    display.print(item);
    if(value){
      display.println("ON");
    }
    else{
      display.println("OFF");
    }
}

void timerIsr() {
  encoder->service();
}

void displayIntMenuPage(String menuItem, int value)
{
    display.setTextSize(2);
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(menuItem);
    display.setTextSize(3);
    display.println(value);
    display.setTextSize(2);
    if(currentMenu == 4){
      display.println(analogRead(PhotoresistorPin));
    }
    display.display();

}


void readRotaryEncoder()
{
  value += encoder->getValue();
  if (value/2 < last) {
    last = value/2;
    down = true;
    delay(150);
  }else if (value/2 >last) {
    last = value/2;
    up = true;
    delay(150);
  }
}

void wakeUp()
{
  isTriggered = true;
}

void CheckLighting(){
      if(lightSen){
        int PRvalue = analogRead(PhotoresistorPin);
        if(nightOnly){
          if(PRvalue < lightCutoffNum){
            TriggerCamera(); 
          }
          else{isTriggered = false;}
        }
        else{
          if(PRvalue > lightCutoffNum){
            TriggerCamera(); 
          }
          else{isTriggered = false;}
        }
      }
      else{
        TriggerCamera();
      }
      isTriggered = false;
}


void TriggerCamera(){
    Serial.println("CameraTriggered");
    digitalWrite(CameraHalfPressPin, HIGH);
    digitalWrite(OnBoardLedPin, HIGH);
    delay(200);
    digitalWrite(CameraHalfPressPin, LOW);
    digitalWrite(OnBoardLedPin, LOW);
    delay(stDelay);
    for(int i = 0; i < numShots; i++){
      digitalWrite(CameraTriggerPin, HIGH);
      digitalWrite(OnBoardLedPin, HIGH);
      delay(500);
      digitalWrite(CameraTriggerPin, LOW);
      digitalWrite(OnBoardLedPin, LOW);
      delay(btwDelay); 
    }
 }

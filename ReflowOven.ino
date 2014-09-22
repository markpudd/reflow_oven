#include "U8glib.h"
#include <SPI.h>

int profile1Time[6] = {0,60,150,180,240,300};
int profile1Temp[6] = {0,150,175,220,30,0};


const int TEMP_MAX = 250;

const int GRAPH_WIDTH_PX=128;
const int GRAPH_HEIGHT_PX=54;

const int DISPLAY_WIDTH_PX=128;
const int DISPLAY_HEIGHT_PX=64;

const int HEATER_RELAY_PIN = 6;
const int START_STOP_PIN = 7;
const int SELECT_PIN = 8;
const int OTHER_PIN = 9;

int buttonStates[3] = {1,1,1};

const int START_BUTTON = 0;     
const int SELECT_BUTTON = 1;     
const int OTHER_BUTTON = 2;     
  
const int DATA_OUT=11; //MOSI
const int DATA_IN=12; //MISO 
const int SPI_CLOCK=13; //sck
const int HEAT_SEL=10; //sck

int timePos;
int screenMode;
boolean inCycle;
int targetTemp;
// stats

int overRuns;
int underRuns;
float maxOver;
float maxUnder;
float averageDeviation;

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);


char textBuf[20];

void setup() {
    inCycle=false;
    pinMode(HEATER_RELAY_PIN, OUTPUT); 
    digitalWrite(HEATER_RELAY_PIN, LOW);	

    u8g.setFont(u8g_font_unifont);
	
    screenMode = 0;


    pinMode(START_STOP_PIN, INPUT_PULLUP); 

    pinMode(SELECT_PIN, INPUT_PULLUP); 
    pinMode(OTHER_PIN, INPUT_PULLUP); 

    // SPI initialization
    pinMode(DATA_OUT, OUTPUT);
    pinMode(DATA_IN, INPUT);
    pinMode(SPI_CLOCK,OUTPUT);
    pinMode(HEAT_SEL, OUTPUT);  
    digitalWrite(HEAT_SEL, HIGH);  
    SPI.begin(); 
    SPI.setClockDivider(SPI_CLOCK_DIV64); 
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE1);
    Serial.begin(57600);
    delay(10);  
  
  
    Serial.begin(57600);
}
	
int readCelsius() {
    unsigned int b1;
    unsigned int b2;
 
    digitalWrite(HEAT_SEL, LOW); 
    b1 = SPI.transfer(0x00);
    b2 = SPI.transfer(0x00);
    digitalWrite(HEAT_SEL, HIGH); 

    b1 = b1 << 8;
    b1 = b2 | b1;
    b1 = b1 >> 3;

    return b1 >> 2;
}


void loop() {
        unsigned long currentMillis = millis();
	int * profileTime = profile1Time;
	int * profileTemp = profile1Temp;
        updateScreen(profileTime,profileTemp);
	if(inCycle) {
		updateHeater(profileTime,profileTemp);
          	timePos++;
          	if(timePos >= profileTime[5]*5) {
                      Serial.println(profileTime[5]);
          		stop();
          	}
	} else { 
		digitalWrite(HEATER_RELAY_PIN, LOW);	
	}
      //  Serial.println(millis()-currentMillis);
	checkButtons();
	delay(250-(millis()-currentMillis));

}

void start() {
	targetTemp =0;
	timePos = 0;
	screenMode = 1;
	inCycle = true;
}

void stop() {
	timePos = 0;
	screenMode = 0;
	inCycle = false;
	digitalWrite(HEATER_RELAY_PIN, LOW);	
}


void checkButtons() {
	int newButtonStates[3]; 
	boolean pressed[3] ={false,false,false}; 
	
	newButtonStates[START_BUTTON] = digitalRead(START_STOP_PIN);     
	newButtonStates[SELECT_BUTTON] = digitalRead(SELECT_PIN);     
	newButtonStates[OTHER_BUTTON] = digitalRead(OTHER_PIN);      

	for(int i=0;i<3;i++) {
	    if (newButtonStates[i] != buttonStates[i]) {
	      if (newButtonStates[i] == LOW) {
			  pressed[i] = true;
		  } else {
		  	  pressed[i] = false;
		  }
	  }
	  buttonStates[i] = newButtonStates[i];	
	}
	if(pressed[START_BUTTON]) {

			if(!inCycle)
				start();
			else {
                          Serial.println("STOP");
				stop();
                        }
		
	} 
	
}
 
void updateHeater(int *profileTimes, int* profileTemps) {
	int currentSegment=0;
	while (currentSegment<6) {
		if(profileTimes[currentSegment]>(timePos/4)) {
			break;
		}
		currentSegment++;
	}
    targetTemp =  map(timePos/4,profileTimes[currentSegment-1],
						  profileTimes[currentSegment],
						  profileTemps[currentSegment-1],
						  profileTemps[currentSegment]);
	
	if(readCelsius()>targetTemp) {
		digitalWrite(HEATER_RELAY_PIN, LOW);	
	} else {
		digitalWrite(HEATER_RELAY_PIN, HIGH);	
	}
}

void drawGraph(int *profileTimes, int* profileTemps) {
	int profileLen=profileTimes[5];
	int ppx;
	int ppy;
	
	//  Bunch of divide by 2 in here otherwise overflow.   bit shift may be slighty more efficent but less readable....
	for(int i=0;i<6;i++) {
		int px = map(profileTimes[i],0,profileLen,0,GRAPH_WIDTH_PX);
		int py = DISPLAY_HEIGHT_PX - map(profileTemps[i],0,TEMP_MAX,0,GRAPH_HEIGHT_PX);
		if(i>0) {
			u8g.drawLine(ppx, ppy, px, py);
		}
		ppx=px;
		ppy=py;
	}
}

void drawProgress(int *profileTimes,int pos) {
	int profileLen=profileTimes[5];
	int px = map(pos,0,profileLen,0,GRAPH_WIDTH_PX);
	u8g.drawLine(px, DISPLAY_HEIGHT_PX, px, DISPLAY_HEIGHT_PX-GRAPH_HEIGHT_PX);
}

void updateScreen(int *profileTimes, int* profileTemps) {
	// profile select screen
	if(screenMode==0) {
		u8g.drawStr( 0, 0, "Board Toaster");
                int temp = readCelsius();
		String displayString = String("BTemp: ")+String(temp);
                 Serial.println(displayString.c_str());
		 u8g.drawStr( 0, 20, displayString.c_str());
		
	}
	// Run screen 1
	if(screenMode==1) {
                int temp = readCelsius();
		String displayString = String("Temp: ")+temp;
                displayString=displayString+String("/");
                displayString=displayString+targetTemp;
 
                Serial.println(displayString.c_str());
		u8g.drawStr( 0, 0, displayString.c_str());
		drawGraph(profileTimes,profileTemps);
		drawProgress(profileTimes,timePos/4);
	}
	
	
}



//Arduino to control watering system
//six zones controlled with valves and moisture sensors

#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <PCD8544.h> 
#include <Rtc_Pcf8563.h>
#include "RTClib.h"

int valveZ[5] = {2,3,4,5,6};   //pins for valves for each zone
                  //pin 6;      //reserved for grass sprinklers    
                        //reserved for grass sprinklers   
int sensorZ[4]= {A0, A1, A2, A3}; //pins for moisture sensors for each zones 
int X = A6;  //X - axis of joystick
int Y = A7;  //Y
// A5, A4 reserved for clock
int powerSupplyMS = 7;  //power for Moisture sensors
double sensorSens[4] = { 1,1,1,1 }; //default sensors sensitivity
int warningLED = 13;
int averageMVSZ[4] = { 0,0,0,0 };
String status = "....Idling....";
																			//	LR][UD	LR][UD
String menu[4][2] = {	{"Grass W Time", "Sensor Z 1"},					//level [0][0]	[0][1]
						{"Garden W Time", "Sensor Z 2"},				//level [1][0]	[1][1] 
						{"Moisture Sens", "Sensor Z 3"},				//level [2][0]	[2][1]
						{"Adjust Clock", "Sensor Z 4"}					//level [3][0]	[3][1]
};

int UDMove = 0;
int LRMove = 0;

int grassT = 45; //EDIT 60;  //default watering time 1 HOUR 
int gardenT = 45; //EDIT 30; //default watering time 1/2 = HOUR GARDEN

//rtc pcf8563 pins A4,A5
Rtc_Pcf8563 rtc;   //Object RTC
PCD8544 display = PCD8544(8, 9, 10, 12, 11);
DateTime dt;
bool GDMS = false;
int ont = 0;

void setup() {
      pinMode(sensorZ[1], INPUT);
      pinMode(sensorZ[2], INPUT);
      pinMode(sensorZ[3], INPUT);
      pinMode(sensorZ[0], INPUT);
      
      pinMode(valveZ[1], OUTPUT);
      pinMode(valveZ[2], OUTPUT);
      pinMode(valveZ[3], OUTPUT);
      pinMode(valveZ[4], OUTPUT);
      pinMode(valveZ[0], OUTPUT);

      pinMode (powerSupplyMS, OUTPUT);
      digitalWrite(powerSupplyMS, LOW);
	  pinMode(warningLED, OUTPUT);

	  pinMode(X, INPUT);
	  pinMode(Y, INPUT);
	  drawMenu(LRMove, UDMove);
	  //rtc.setDate(3, 1, 3, 0, 19);
      //Serial.begin(9600);
	  display.begin();
	  MainScreen();
}

void waterGrass(int wateringTime){
  
  if (rtc.getHour() == wateringTime/60 && rtc.getMinute() >= wateringTime%60)  {
	  Serial.println("Watering Grass OFF");
	  digitalWrite(valveZ[4], LOW);
	  status = "....Idling....";
  }
  else  {
	  digitalWrite(valveZ[4], HIGH);
	  Serial.println("Watering Grass ON");
	  status = "Watering Grass";
  }
}

int runningTime(int moistVal, int sens){  // malkoto e mokro
  //int span = 0;

  if(moistVal > 650 * sens){      // full water
	  return 2;// span = 2;
  }
  else if(moistVal <= 450 * sens){          // no water
	  return 0;//span = 0;
  }
  else{		//half water
	  return 1;// span = 1;
  }
  //return span;
}

void PrintGDMS(int times) {
	display.clear();
	display.print("Collecting ");
	display.setCursor(0, 1);
	display.print("data  from");
	display.setCursor(0, 2);
	display.print("sensors...");
	display.setCursor(0, 3);
	GetDataMS(times);
	delay(2000);
	display.clear();
	display.print("Done!");
	for (int i = 0; i < 4; i++) {
		display.setCursor(0, i+1);
		display.print("Sensor ");
		display.print(i);
		display.print("-");
		display.print(averageMVSZ[i]);
	}

	delay(2500);
}

void GetDataMS(int repeatTimes) {
	status = "GettingData MS";
	for (int i = 0; i < 4; i++) { //reset averageMVSZ
		averageMVSZ[i] = 0;
	}
	for (int j = 0; j < repeatTimes; j++) { //get value from moisture sensors for each zones
		digitalWrite(powerSupplyMS, HIGH);
		delay(3000);

		for (int i = 0; i < 4; i++) {
			int moistValueZ = analogRead(sensorZ[i]);
			averageMVSZ[i] = averageMVSZ[i] + moistValueZ;
		}
		digitalWrite(powerSupplyMS, LOW);
		display.clear();
		MainScreen();
		delay(1000);   //real value 300000 reads value from sensors each 10 minues TODO
	}
	for (int i = 0; i < 4; i++) {
		averageMVSZ[i] = averageMVSZ[i] / repeatTimes;
	}
	status = "....Idling....";
}

void waterGarden(int wateringTime) {
	for (int i = 0; i < 4; i++) {

		if (runningTime(averageMVSZ[i], sensorSens[i]) == 2) {

			if (rtc.getHour() == ont + (wateringTime / 60) && rtc.getMinute() >= wateringTime % 60) {
				digitalWrite(valveZ[i], LOW);
				status = "....Idling....";
			}
			else {
				digitalWrite(valveZ[i], HIGH);
				status = "WateringGarden";
			}
		}
		if (runningTime(averageMVSZ[i], sensorSens[i]) == 1) {

			if (rtc.getHour() == ont + (wateringTime / 2 / 60) && rtc.getMinute() >= (wateringTime / 2) % 60)
			{
				digitalWrite(valveZ[i], LOW);
				status = "....Idling....";
				GDMS = false;
			}
			else {
				digitalWrite(valveZ[i], HIGH);
				status = "WateringGarden";
			}
		}
		if (runningTime(averageMVSZ[i], sensorSens[i]) == 0) {

			
				digitalWrite(valveZ[i], LOW);
				status = "....Idling....";
				GDMS = false;
		}
	}
}

void drawMenu(int menuLevel, int activeMenu) {
	display.clear();
	display.setCursor(0, 0);
	display.println("SETUP MENU");
	display.setCursor(0, 1);
	for (int i = 0; i < 4; i++) {
		if (menu[i][menuLevel] == ""){
			continue;
		}
		if (i == activeMenu) {
			display.setCursor(0, i+1);
			display.print("*");
		}
		display.setCursor(7, i+1);
		display.print(menu[i][menuLevel]);
	}
	display.setCursor(0, 5);
	display.print("esc|dn|up|ent");
}

void navigation() {
	int buttonLR;
	int buttonUD;
	bool notPressed = true;
	drawMenu(LRMove, UDMove);

	do{	
		buttonLR = analogRead(X);		//LR = menu level
		buttonUD = analogRead(Y);		//UD = active menu
		delay(500);
		if (buttonLR > 850) {         //button rigth
			buttonLR = 500;
			buttonUD = 500;
		  //enter active submenu
			if (LRMove == 0){
				if (UDMove == 0){
					grassT = setupGrassWateringTime(grassT,"Grass");
					UDMove = 0;
					LRMove = 0;
					drawMenu(LRMove, UDMove);
				}
				if (UDMove == 1){
					gardenT = setupGrassWateringTime(gardenT, "Garden");
					UDMove = 0;
					LRMove = 0;
					drawMenu(LRMove, UDMove);
				}
				if (UDMove == 2){
					UDMove = 0;
					LRMove++;
					drawMenu(LRMove, UDMove);
				}
				if (UDMove == 3){
					adjustClock();
					UDMove = 0;
					LRMove = 0;
					drawMenu(LRMove, UDMove);
				}
			}
			else if (LRMove == 1){
				if (UDMove == 0){
					sensorSens[0] = SensorSensitivity(sensorSens[0]);
					UDMove = 0;
					//LRMove = 0;
					drawMenu(LRMove, UDMove);
				}
				if (UDMove == 1){
					sensorSens[1] = SensorSensitivity(sensorSens[1]);
					UDMove = 0;
					//LRMove = 0;
					drawMenu(LRMove, UDMove);
				}
				if (UDMove == 2){
					sensorSens[2] = SensorSensitivity(sensorSens[2]);
					UDMove = 0;
					//LRMove = 0;
					drawMenu(LRMove, UDMove);
				}
				if (UDMove == 3){
					sensorSens[3] = SensorSensitivity(sensorSens[3]);
					UDMove = 0;
					//LRMove = 0;
					drawMenu(LRMove, UDMove);
				}
			}
		}
		if (buttonLR < 150) {         //button left
		  //leave active 
			buttonLR = 500;
			buttonUD = 500;
			
			LRMove--;
			UDMove = 0;
			if (LRMove >= 0) {
				drawMenu(LRMove, UDMove);
			}
			display.clear();
			MainScreen();
		}
		if (buttonUD < 150) {         //button up
		  //move selector up
			buttonLR = 500;
			buttonUD = 500;
			if (UDMove > 0)	{
				UDMove--;
			}
			drawMenu(LRMove, UDMove);
			
		}
		if (buttonUD > 850) {         //button down
		  //move selector down
			buttonLR = 500;
			buttonUD = 500;
			if (UDMove < 3) {
				UDMove++;
			}
			drawMenu(LRMove, UDMove);
			//TODO
		}

	} while (LRMove >= 0);
	LRMove = 0;
}

int setupGrassWateringTime(int currentWateringTime, String logo) {	//Set up grass watering time form 0,15 to 2 hours 

	int buttonUD;
	int buttonLR;
	
	display.clear();
	display.setCursor(0, 1);
	display.print(logo);
	display.setCursor(0, 2);
	display.print("Watering Time");
	display.setCursor(0, 3);
	display.print(" = ");
	display.print(currentWateringTime);
	display.print("  min");
	display.setCursor(0, 5);
	display.print("   |dn|up|set");
	do
	{
		
		buttonUD = analogRead(Y);
		buttonLR = analogRead(X);
		delay(500);
		
		if (buttonUD > 850) {         //button up
		//move selector up
			buttonLR = 500;
			buttonUD = 500;
			if (currentWateringTime < 120)	{
				currentWateringTime += 15;
			}
			display.setCursor(17, 3);
			display.print(currentWateringTime);
			display.print(" ");
		}
		if (buttonUD < 150) {         //button down
		  //move selector down
			buttonLR = 500;
			buttonUD = 500;
			if (currentWateringTime > 15)	{
				currentWateringTime -= 15;
			}
			display.setCursor(17, 3);
			display.print(currentWateringTime);
			display.print(" ");
		}
		
	} while (buttonLR < 850);

	display.setCursor(0, 1);
	display.print(logo);
	display.setCursor(0, 2);
	display.print("Watering Time");
	display.setCursor(0, 3);
	display.print("set to ");
	display.print(currentWateringTime);
	display.print(" min");

	delay(2000);
	return currentWateringTime;

}

double SensorSensitivity(double currentSens) {
	int buttonUD;
	int buttonLR;
	display.clear();
	display.setCursor(0, 1);
	display.print("Sensor");
	display.setCursor(0, 2);
	display.print("sensitivity");
	display.setCursor(0, 3);
	display.print("=  ");
	display.print(currentSens);
	display.println(" %");
	display.setCursor(0, 5);
	display.print("   |dn|up|set");
	do
	{

		buttonUD = analogRead(Y);
		buttonLR = analogRead(X);
		delay(500);
		if (buttonUD > 850) {         //button up
		//move selector up
			if (currentSens < 2){
				currentSens += 0.2; //plus 20% to
			}
			display.setCursor(0, 3);
			display.print("=  ");
			display.print(currentSens);
			display.println(" %");
		}
		if (buttonUD < 150) {         //button down
		  //move selector down
			if (currentSens > 0.4)
			{
				currentSens -= 0.2; //minus 20%
			}
			display.setCursor(0, 3);
			display.print("=  ");
			display.print(currentSens);
			display.println(" %");
		}
	} while (buttonLR < 850);
	display.setCursor(0, 1);
	display.print("Sensor");
	display.setCursor(0, 2);
	display.print("sensitivity");
	display.setCursor(0, 3);
	display.print("set to ");
	display.print(currentSens);
	display.println(" %");
	delay(2000);
	return currentSens;
}

void adjustClock() {
	int buttonLR;
	int buttonUD;
	LRMove = 0;
	UDMove = 0;
	int year = rtc.getYear();
	int month = rtc.getMonth();
	int day = rtc.getDay();
	int hour = rtc.getHour();
	int minute = rtc.getMinute();
	display.clear();
	display.setCursor(15,0);
	display.print("DATE/TIME");
	display.setCursor(0, 2);
	display.print("*");
	display.print(day);
	display.print("/");
	display.print(month);
	display.print("/");
	display.print(year); 
	display.setCursor(0, 3);
	display.print(hour);
	display.print(":");
	display.print(minute);
	display.setCursor(0, 5);
	display.print("   |dn|up|ent");
	
	do
	{
		buttonUD = analogRead(Y);
		buttonLR = analogRead(X);
		delay(500);
		if (buttonUD > 850) {         //button up
			switch (LRMove)
			{
			case 2: if(year < 99) { year++; }
					break;
			case 1: if (month < 12) {month++; }
					break;
			case 0: if (day < 31) { day++; }
					break;
			case 3: if (hour < 23) { hour++; }
					break;
			case 4: if (minute < 60) { minute++; }
				break;
			}

			display.setCursor(0, 2);
			display.clearLine();
			if (LRMove == 0) {
				display.print("*");
			}
			display.print(day);
			display.print("/");
			if (LRMove == 1) {
				display.print("*");
			}
			display.print(month);
			display.print("/");
			if (LRMove == 2) {
				display.print("*");
			}
			display.print(year);
			display.setCursor(0, 3);
			display.clearLine();
			if (LRMove == 3) {
				display.print("*");
			}
			display.print(hour);
			display.print(":");
			if (LRMove == 4) {
				display.print("*");
			}
			display.print(minute); 
			
		}
		if (buttonUD < 150) {         //button down
			switch (LRMove)
			{
			case 2: if (year > 19) { year--; }
					break;
			case 1: if (month > 1) { month--; }
					break;
			case 0: if (day > 1) { day--; }
					break;
			case 3: if (hour > 0) { hour--; }
					break;
			case 4: if (minute > 0) { minute--; }
				break;
			}
			display.setCursor(0, 2);
			display.clearLine();
			if (LRMove == 0) {
				display.print("*");
			}
			display.print(day);
			display.print("/");
			if (LRMove == 1) {
				display.print("*");
			}
			display.print(month);
			display.print("/");
			if (LRMove == 2) {
				display.print("*");
			}
			display.print(year);
			display.setCursor(0, 3);
			display.clearLine();
			if (LRMove == 3) {
				display.print("*");
			}
			display.print(hour);
			display.print(":");
			if (LRMove == 4) {
				display.print("*");
			}
			display.print(minute);
		}
		
		if (buttonLR > 850) {         //button rigth
			LRMove++;
			display.setCursor(0, 2);
			display.clearLine();
			if (LRMove == 0) {
				display.print("*");
			}
			display.print(day);
			display.print("/");
			if (LRMove == 1) {
				display.print("*");
			}
			display.print(month);
			display.print("/");
			if (LRMove == 2) {
				display.print("*");
			}
			display.print(year);
			display.setCursor(0, 3);
			display.clearLine();
			if (LRMove == 3) {
				display.print("*");
			}
			display.print(hour);
			display.print(":");
			if (LRMove == 4) {
				display.print("*");
			}
			display.print(minute);
		}
		if (buttonLR < 150) {         //button left
			LRMove--;
			display.setCursor(0, 2);
			display.clearLine();
			if (LRMove == 0) {
				display.print("*");
			}
			display.print(day);
			display.print("/");
			if (LRMove == 1) {
				display.print("*");
			}
			display.print(month);
			display.print("/");
			if (LRMove == 2) {
				display.print("*");
			}
			display.print(year);
			display.setCursor(0, 3);
			display.clearLine();
			if (LRMove == 3) {
				display.print("*");
			}
			display.print(hour);
			display.print(":");
			if (LRMove == 4) {
				display.print("*");
			}
			display.print(minute);
		}
		
	} while (LRMove < 5);
	LRMove = 0;
	UDMove = 0;
	rtc.setDate(day, 1, month, 0, year);
	rtc.setTime(hour, minute, 0);
	display.clear();
	display.setCursor(0, 4);
	display.print("DATE/TIME SET!");
	delay(1000);
} //ok

void PrintStatus() {
	display.clear();
	display.print("SYSTEM STATUS");
	display.setCursor(0, 1);
	display.print("GrassWT = ");
	display.print(grassT);
	display.print("m");
	display.setCursor(0, 2);
	display.print("GardenWT = ");
	display.print(gardenT);
	display.print("m");
	display.setCursor(0, 3);
	display.print("Sensor sensit");
	display.setCursor(0, 4);
	display.print("|1-");
	display.print(sensorSens[0]);
	display.print("|2-");
	display.print(sensorSens[1]);
	display.print("|3-");
	display.print(sensorSens[2]);
	display.print("|4-");
	display.print(sensorSens[3]);
	delay(5000);
}

void MainScreen() {
	Serial.println("Main Screen");
	display.print("AUTO  WATERING");
	display.print("    SYSTEM");
	display.setCursor(0, 2);
	display.print("Time:");
	display.print(rtc.getHour());
	display.print(":");
	display.print(rtc.getMinute());
	display.print(":");
	display.print(rtc.getSecond());
	display.setCursor(0, 3);
	display.print(status);
	display.setCursor(0, 5);
	display.print("esc|sta|ms|men");
}

void loop() {
	
	if (rtc.getHour() == 23 && !GDMS)
	{
		GetDataMS(6);
		GDMS = true;
	}
	
	if (rtc.getHour() == 0){

		waterGrass(grassT);
		ont = grassT / 60;
		if (grassT % 60 != 0) {
			ont++;
		}
	}
	
	if (rtc.getHour() == ont){
		waterGarden(gardenT);
	}

	int buttonLR = analogRead(X);
	int buttonUD = analogRead(Y);
	
	delay(500);
	if (buttonLR > 850){
		buttonLR = 500;
		buttonUD = 500;
		navigation();
		display.clear();
		MainScreen();
	}
	if (buttonUD < 150)
	{
		buttonLR = 500;
		buttonUD = 500;
		PrintStatus();
		display.clear();
		MainScreen();
	}
	if (buttonUD > 850)
	{
		buttonLR = 500;
		buttonUD = 500;
		PrintGDMS(2);
		Serial.println("MS Data printed");
		display.clear();
		MainScreen();
	}
	delay(500);
	display.setCursor(0, 2);
	display.clearLine();
	display.print("Time:");
	display.print(rtc.getHour(),10);
	display.print(":");
	display.print(rtc.getMinute(),10);
	display.print(":");
	display.print(rtc.getSecond(),10);
	display.setCursor(0, 3);
	display.print(status);
}
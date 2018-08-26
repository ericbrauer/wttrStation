#include "dht11.h"
#include <ShiftLCD.h>

dht11 DHT;
// initialize the library with the numbers of the interface pins
ShiftLCD lcd(2, 4, 3);

#define DHT11_PIN 5
void printToLcd() {

	// DISPLAY DATA
	lcd.setCursor(0, 0);
	lcd.print("Temp: ");
	lcd.setCursor(6, 0);
	lcd.print(DHT.temperature);
	lcd.setCursor(9,0);
	lcd.print(char(223));
	lcd.print("C");
	lcd.setCursor(0,1);
	lcd.print("Humid: ");
	lcd.setCursor(7,1);
	lcd.print(DHT.humidity);
	lcd.setCursor(10,1);
	lcd.print("%");
}
void setup(){
	Serial.begin(9600);
	Serial.println("DHT TEST PROGRAM ");
	Serial.print("LIBRARY VERSION: ");
	Serial.println(DHT11LIB_VERSION);
	Serial.println("Type,\tstatus,\tHumidity (%),\tTemperature (C)");
	// set up the LCD's number of rows and columns: 
    lcd.begin(16, 2);
}

void loop(){
	int chk;
	int gas;
	Serial.print("DHT11, \t");
	chk = DHT.read(DHT11_PIN); // READ DATA
	gas = analogRead(0);
	switch (chk) {
		case DHTLIB_OK:
			Serial.print("OK,\t");
			break;
		case DHTLIB_ERROR_CHECKSUM:
			Serial.print("Checksum error,\t");
			lcd.print("ChkSum Err");
			break;
		case DHTLIB_ERROR_TIMEOUT:
			Serial.print("Time out error,\t");
			lcd.print("Timeout Err");
			break;
		default:
			Serial.print("Unknown error,\t");
			lcd.print("Error??");
			break;
	}
	printToLcd();
	Serial.print(DHT.humidity,1);
	Serial.print(",\t");
	Serial.println(DHT.temperature,1);
	Serial.print("Gas: ");
	Serial.println(gas,DEC);
	delay(1000);
}
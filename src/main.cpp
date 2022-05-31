#include <Arduino.h>

//Libraries
#include <DHT.h>
#include <ShiftLCD.h>

//Constants

// for humidity
#define DHTPIN D5     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)

// for barometer
#define ALTITUDE 76 // altitute of Toronto, in metres.


DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino

// initialize the library with the numbers of the interface pins
ShiftLCD lcd(D6, D7, D8); // update these pins for the new scheme

//Variables
int chk;
float hum;  //Stores humidity value
float temp; //Stores temperature value

void printToLcd(float temp, float humid, float bmp_temp, float pres) {
	// DISPLAY DATA
	lcd.setCursor(0, 0);
	lcd.print("T: ");
	lcd.setCursor(3, 0);
	if (bmp_temp != 0)
		lcd.print(bmp_temp,1);
	else
		lcd.print(temp);
	lcd.setCursor(7,0);
	lcd.print(char(223));
	lcd.print("C");
	lcd.setCursor(0,1);
	lcd.print("H: ");
	lcd.setCursor(3,1);
	lcd.print(humid);
	lcd.setCursor(7,1);
	lcd.print("%");
	lcd.setCursor(10,1);
	lcd.print(pres,1);
	lcd.print("mb");
}

void setup()
{
  Serial.begin(115200);
  dht.begin();

	// set up the LCD's number of rows and columns: 
    lcd.begin(16, 2);
	  lcd.setCursor(0, 0);
}

void loop()
{
    delay(2000);
    //Read data and store it to variables hum and temp
    hum = dht.readHumidity();
    temp= dht.readTemperature();
    //Print temp and humidity values to serial monitor
    Serial.print("Humidity: ");
    Serial.print(hum);
    Serial.print(" %, Temp: ");
    Serial.print(temp);
    Serial.println(" Celsius");
    delay(10000); //Delay 2 sec.
    printToLcd(temp, hum, 0, 0);
}
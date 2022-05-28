#include <Arduino.h>

//Libraries
#include <DHT.h>
#include <ShiftLCD.h>

//Constants

// for humidity
#define DHTPIN D8     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)

// for barometer
#define ALTITUDE 76 // altitute of Toronto, in metres.


DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino

// initialize the library with the numbers of the interface pins
// ShiftLCD lcd(2, 6, 3);  update these pins for the new scheme

//Variables
int chk;
float hum;  //Stores humidity value
float temp; //Stores temperature value

void setup()
{
  Serial.begin(115200);
  dht.begin();
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
}
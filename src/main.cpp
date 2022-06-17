#include <Arduino.h>

//Libraries
#include <DHT.h>
#include <ShiftLCD.h>
#include <SFE_BMP180.h>

//Constants

// for humidity
#define DHTPIN D5     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)

// for barometer
#define ALTITUDE 76 // altitute of Toronto, in metres.

// Uncomment for more Serial Messages
// #define DEBUG 

DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino

// initialize the library with the numbers of the interface pins
ShiftLCD lcd(D6, D7, D8); // update these pins for the new scheme

// initialize pressure sensor, with I2C
SFE_BMP180 pressure;  // on NodeMCU SCL == D1 SDA == D2

//Variables
int chk;
float hum;  //Stores humidity value
float temp; //Stores temperature value

// apparently double is 4 bytes?? so no difference
double T, P; // Temperature and pressure from BMP180
double p0; // also used by BMP180

// for trend checking
int prev_refresh;
float prev_hum; 
float prev_temp;
double prev_T, prev_P;

float delta_hum;
float delta_temp;
float delta_T, delta_P;

void printToLcd(float temp, float humid, float bmp_temp, float pres) {
	// DISPLAY DATA
	lcd.setCursor(0, 0);
	// lcd.print("T: ");
	// lcd.setCursor(3, 0);
	if (bmp_temp != 0)
		lcd.print(bmp_temp,1);
	else
		lcd.print(temp);
	lcd.setCursor(4,0);
	lcd.print(char(223)); // refer to https://www.sparkfun.com/datasheets/LCD/HD44780.pdf
	// page 17 because that's the chip I have, can't use page 18 :(
	lcd.print("C");
	lcd.setCursor(7,0);
	if (delta_T > 0) {
		lcd.print("+"); // should be 'up'
	}
	else if (delta_T < 0) {
		lcd.print("-"); // should be 'down'
	}
	else { lcd.print(" "); }
	lcd.setCursor(9,0);
	// lcd.print("H: ");
	// lcd.setCursor(3,1);
	lcd.print(humid);
	lcd.setCursor(13,0);
	lcd.print("%");
	// HUMIDITY
	lcd.setCursor(15,0);
	if (delta_hum > 0) {
		lcd.print("+"); // should be 'up'
	}
	else if (delta_hum < 0) {
		lcd.print("-"); // should be 'down'
	}
	else { lcd.print(" "); }
	lcd.setCursor(1,1);
	lcd.print(pres,1);
	lcd.setCursor(7,1);
	lcd.print("mb");
	lcd.setCursor(11,1);
	if (delta_P != 0) {
		lcd.print(delta_P,1);
	}
}

double get_bmp_temp() {
	char bmp_status;
	bmp_status = pressure.startTemperature();

	if (bmp_status != 0) // if it returns zero, there's an error. otherwise return ms to wait for reading.
	{
		#ifdef DEBUG
			Serial.print("Delay in (ms) is: "); 
			Serial.println(int(pressure_status));
		#endif
		delay(bmp_status); 
		bmp_status = pressure.getTemperature(T);
		if (bmp_status != 0)
		{
			#ifdef DEBUG
				Serial.print("BMP reports temperature of ");
				Serial.print(T,2);
				Serial.println(" deg C");
			#endif
			return T;
		}
		else Serial.println("error getting temperature!");
	}
	else Serial.println("error starting temperature reading!");
	return 0;
}

double get_bmp_pres() {
	char bmp_status;

	bmp_status = pressure.startPressure(3); // 3 is oversampling setting, high res and long wait.
	if (bmp_status != 0) {
		#ifdef DEBUG
			Serial.print("Delay in ms is: ");
			Serial.println(int(pressure_status));
		#endif
		delay(bmp_status);

		bmp_status = pressure.getPressure(P,T);
		if (bmp_status != 0) {
			p0 = pressure.sealevel(P,ALTITUDE);
			#ifdef DEBUG
				Serial.print("BMP180 is reporting a relative pressure of ");
				Serial.print(p0,2);
				Serial.println(" hPa.");
			#endif
			return P;
		}
		else Serial.println("error getting pressure!");

	}
	else Serial.println("error starting pressure reading!");
	return 0;
}

float get_delta(float prev, float current) {
	float result;
	result = current - prev;
	#ifdef DEBUG
		Serial.print("Current is ");
		Serial.print(current);
		Serial.print(" and that is a change of ");
		Serial.println(result);
	#endif
	return result;
}

float get_delta(double prev, double current) {
	float result;
	result = current - prev;
	#ifdef DEBUG
		Serial.print("Current is ");
		Serial.print(current);
		Serial.print(" and that is a change of ");
		Serial.println(result);
	#endif
	return result;
}

void setup()
{
    delta_hum = 0;
    delta_temp = 0;
    delta_P = 0;
    delta_T = 0;
    Serial.begin(115200);
    dht.begin();
	
  // setup Barometer
	Serial.print("Initialzing Barometer....");
	if (pressure.begin())
		Serial.println("OK.");
	else
		Serial.println("BMP180 has failed to initialize!");

	// set up the LCD's number of rows and columns: 
    lcd.begin(16, 2);
	lcd.setCursor(0, 0);
	//get initial values for comparison
	prev_hum = dht.readHumidity(); 
	prev_temp = dht.readTemperature();
	prev_P = get_bmp_pres();
	prev_T = get_bmp_temp();
    delay(2000);
	prev_refresh = 0;  // start at end of counter so that prev temp taken at outset
	lcd.print(char(30)); // should be 'up'
}

void loop()
{
	//Read data and store it to variables hum and temp
    hum = dht.readHumidity();
    temp= dht.readTemperature();
	T = get_bmp_temp();
	P = get_bmp_pres();
	Serial.print("BMP reports: ");
	Serial.println(T);
    //Print temp and humidity values to serial monitor
    Serial.print("Humidity: ");
    Serial.print(hum);
    Serial.print(" %, Temp: ");
    Serial.print(temp);
    Serial.print(" Celsius, ");
	Serial.print("Pressure: ");
	Serial.print(P);
	Serial.println(" hPa.");
	if (prev_refresh >= 60) {  // twenty minute interval = 120
		delta_hum = get_delta(prev_hum, hum);
		delta_P = get_delta(prev_P, P);
		delta_temp = get_delta(prev_temp, temp);
		delta_T = get_delta(prev_T, T);
		Serial.print("Pressure change of ");
		Serial.println(delta_P);
		prev_hum = hum;  // update values for comparison
		prev_temp = temp;
		prev_P = P;
		prev_T = T;
		prev_refresh = 0;  // set for twenty minute intervals
	}
	else {
		prev_refresh++;
	}
    printToLcd(temp, hum, T, P);
    delay(10000); //Delay 2 sec.
}
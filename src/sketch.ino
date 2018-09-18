#include "dht11.h"
#include <ShiftLCD.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <SD.h>
#include <TimeLib.h> // https://www.pjrc.com/teensy/td_libs_Time.html
#include <Wire.h>
#include <SFE_BMP180.h>

#define MEGA // Use the MEGA 2560 and different pinouts
// TODO: output the pressure and more accurate temp to LCD.
// TODO: if there's no internet, stop the NTP requests to stop delays
// TODO: output to sd also, and slope of pressure on lcd

/*
Any Arduino pins labeled:  SDA  SCL
Uno, Redboard, Pro:        A4   A5
Mega2560, Due:             20   21
*/

byte mac[] = {
  0x90, 0xA2, 0xDA, 0x0D, 0x10, 0x78 };

IPAddress timeServer(158, 69, 125, 231);

const int timeZone = -4;
// TODO make this not be static OR be able to change with cursor

dht11 DHT;
SFE_BMP180 pressure;

#define ALTITUDE 76 // altitute of Toronto, in metres.

// initialize the library with the numbers of the interface pins
ShiftLCD lcd(2, 6, 3);

#define SD_CS_PIN 4
#define DHT11_PIN 5

unsigned int localPort = 8888;       // local port to listen for UDP packets

int minuteCounter = 0;

File wttrLog;

// A UDP instance to let us s20jend and receive packets over UDP
EthernetUDP Udp;


void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  if(digits < 10)
    lcd.print('0');
  lcd.print(digits);
}


void printToLcd() {
	// DISPLAY DATA
	lcd.setCursor(0, 0);
	lcd.print("T: ");
	lcd.setCursor(3, 0);
	if (T != 0)
		lcd.print(T,1)
	else
		lcd.print(DHT.temperature);
	lcd.setCursor(7,0);
	lcd.print(char(223));
	lcd.print("C");
	lcd.setCursor(0,1);
	lcd.print("H: ");
	lcd.setCursor(3,1);
	lcd.print(DHT.humidity);
	lcd.setCursor(5,1);
	lcd.print("%");
	if (timeStatus() != timeNotSet) {
		// TODO Make this work with the printDigits function.
		lcd.setCursor(11,0);
		if (hour() < 10)
			lcd.print(" ");
		lcd.print(hour());
		lcd.setCursor(13,0);
		lcd.print(":");
		if (minute() < 10) {
			lcd.print("0");
		}
		lcd.print(minute());
	}
}
void setup(){
	Serial.begin(9600);
	Serial.println("DHT TEST PROGRAM ");
	Serial.print("LIBRARY VERSION: ");
	Serial.println(DHT11LIB_VERSION);
	Serial.print("Initializing SD card....");
	if (!SD.begin(SD_CS_PIN))
		Serial.println("SD Card initialization failed!");
	else 
		Serial.println("initialization complete.");
	
	// TODO: it'd be good to not try and do all these writes if init has failed.
	wttrLog = SD.open("wttrLog.csv", FILE_WRITE);
	//wttrLog.println("UTC,Humidity,Temperature");
	wttrLog.println("beginning of log");
	wttrLog.close();	
	
	// setup Barometer
	if (pressure.begin())
		Serial.println("BMP180 successfully initialized");
	else
		Serial.println("BMP180 has failed to initialize!");

	// start Ethernet and UDP
	if (Ethernet.begin(mac) == 0) {
		Serial.println("Failed to configure Ethernet using DHCP");
		// TODO: raise flag to indicate on LCD.
	}
	Serial.print("IP Address is ");
	Serial.println(Ethernet.localIP());
	
	// set up the LCD's number of rows and columns: 
    lcd.begin(16, 2);
	lcd.setCursor(0, 0);
	lcd.print(Ethernet.localIP());
	delay(3000);
	lcd.clear();

	Udp.begin(localPort);
	setSyncProvider(getNtpTime);
	Serial.println("Type,\tstatus,\tHumidity (%),\tTemperature (C)");
}

void loop(){
	int chk;
	// these variables used by BMP180 Barometer
	char pressure_status;
	double T,P,p0,a;

	// humidity sensor diagnostic
	chk = DHT.read(DHT11_PIN); // READ DATA
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

	// If ntp has failed, keep trying.
	// TODO: wouldn't it be cool if we cycled through an array of ntp addresses, rather than keep trying the one? 
	if (timeStatus() != timeNotSet)
		Serial.println("Time is set properly! :D");
	else {
		setSyncProvider(getNtpTime);
	}


	pressure_status = pressure.startTemperature();
	if (pressure_status != 0) // if it returns zero, there's an error. otherwise return ms to wait for reading.
	{
		delay(pressure_status); 
		pressure_status = pressure.getTemperature(T);
		if (pressure_status != 0)
		{
			Serial.print("BMP reports temperature of ");
			Serial.print(T,2);
			Serial.println(" deg C");

			pressure_status = pressure.startPressure(3); // 3 is oversampling setting, high res and long wait.
			if (pressure_status != 0) {
				delay(pressure_status);

				pressure_status = pressure.getPressure(P,T);
				if (pressure_status != 0) {
					p0 = pressure.sealevel(P,ALTITUDE);
					Serial.print("BMP180 is reporting a relative pressure of ");
					Serial.print(p0,2);
					Serial.println(" hPa.");
				}
				else Serial.println("error getting pressure!");

			}
			else Serial.println("error starting pressure reading!");
		
		}
		else Serial.println("error getting temperature!");
	}
	else Serial.println("error starting temperature reading!");

	if (minuteCounter >= 60) {
		minuteCounter = 0;
		// setSyncProvider(getNtpTime); // send an NTP packet to a time server
		Serial.println("Writing to Log now.");
		wttrLog = SD.open("wttrLog.csv", FILE_WRITE);
		//wttrLog.print(UTCString());
		wttrLog.print(year());
		wttrLog.print("-");
		wttrLog.print(month());
		wttrLog.print("-");
		wttrLog.print(day());
		wttrLog.print("T");
		wttrLog.print(hour());
		wttrLog.print(":");
		wttrLog.print(minute());
		wttrLog.print(":");
		wttrLog.print(second());
		wttrLog.print("-04:00");
		wttrLog.print(",\"");
		wttrLog.print(DHT.humidity,1);
		wttrLog.print("\",\"");
		wttrLog.print(DHT.temperature,1);
		wttrLog.println("\"");
		wttrLog.close();
	}
	else
		minuteCounter++;
	printToLcd();
	Serial.print(UTCString());
	Serial.print(",\t");
	Serial.print(DHT.humidity,1);
	Serial.print(",\t");
	Serial.print(DHT.temperature,1);
	// Serial.print(",\t");
	Ethernet.maintain();
	delay(1000);
}

String UTCString() {
	// TODO leading zeroes for month, date, minute, timezone.
	String datastring = "\"" + String(year()) + "-" + String(month()) + "-" + String(day()) + "T" + hour() + ":" + minute() + ":" + second() + timeZone + ":00\"";
	return datastring;
}

/*----------- NTP Code for TimeLib.h ------------------------------------------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

#include "dht11.h"
#include <ShiftLCD.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <SD.h>
#include <TimeLib.h> // https://www.pjrc.com/teensy/td_libs_Time.html

// TODO: Fix the timing, or import a library.
// TODO: if internet fails, indicate but proceed with everything else.

byte mac[] = {
  0x90, 0xA2, 0xDA, 0x0D, 0x10, 0x78 };

dht11 DHT;
// initialize the library with the numbers of the interface pins
ShiftLCD lcd(2, 4, 3);

#define DHT11_PIN 5

// The following are defs required for NTP

unsigned int localPort = 8888;       // local port to listen for UDP packets

const char timeServer[] = "time.nist.gov"; // time.nist.gov NTP server

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

int minuteCounter = 0;

unsigned long epoch;

time_t t;

File wttrLog;

// A UDP instance to let us s20jend and receive packets over UDP
EthernetUDP Udp;

unsigned long handleNTPResponse() {
	Serial.println("We have received a response");
	// We've received a packet, read the data from it
	Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

	// the timestamp starts at byte 40 of the received packet and is four bytes,
	// or two words, long. First, extract the two words:

	unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
	unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
	// combine the four bytes (two words) into a long integer
	// this is NTP time (seconds since Jan 1 1900):
	unsigned long secsSince1900 = highWord << 16 | lowWord;
	// now convert NTP time into everyday time:
	// Serial.print("Unix time = ");
	// Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
	const unsigned long seventyYears = 2208988800UL;
	// subtract seventy years:
	return secsSince1900 - seventyYears;
}
	


void epochToUTC() {
		// print the hour, minute and second:
		Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
		Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
		Serial.print(':');
		if (((epoch % 3600) / 60) < 10) {
		// In the first 10 minutes of each hour, we'll want a leading '0'
		Serial.print('0');
		}
		Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
		Serial.print(':');
		if ((epoch % 60) < 10) {
		// In the first 10 seconds of each minute, we'll want a leading '0'
		Serial.print('0');
		}
		Serial.println(epoch % 60); // print the second
	}

	// send an NTP request to the time server at the given address
void sendNTPpacket(const char * address) {
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
	Udp.beginPacket(address, 123); // NTP requests are to port 123
	Udp.write(packetBuffer, NTP_PACKET_SIZE);
	Udp.endPacket();
	Serial.println("NTP packet sent.");
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  if(digits < 10)
    lcd.print('0');
  lcd.print(digits);
}


void printToLcd() {

	// DISPLAY DATA
	lcd.setCursor(0, 0);
	lcd.print("Temp: ");
	lcd.setCursor(6, 0);
	lcd.print(DHT.temperature);
	lcd.setCursor(8,0);
	lcd.print(char(223));
	lcd.print("C");
	lcd.setCursor(0,1);
	lcd.print("Humid: ");
	lcd.setCursor(7,1);
	lcd.print(DHT.humidity);
	lcd.setCursor(9,1);
	lcd.print("%");
	if (timeStatus()) {
		lcd.setCursor(11,0);
		lcd.print((hour() - 4));
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
	if (!SD.begin(4)) {
		Serial.println("initialization failed.");

	}
	Serial.println("initialization complete.");
	wttrLog = SD.open("wttrLog.csv", FILE_WRITE);
	wttrLog.println("Beginning of log.");
	wttrLog.close();	
	Serial.println("Type,\tstatus,\tHumidity (%),\tTemperature (C)");
	// start Ethernet and UDP
    //Ethernet.init(10);
	if (Ethernet.begin(mac) == 0) {
		Serial.println("Failed to configure Ethernet using DHCP");
		// raise flag to indicate on LCD.
	}
	Udp.begin(localPort);
	// set up the LCD's number of rows and columns: 
    lcd.begin(16, 2);
	sendNTPpacket(timeServer); // send an NTP packet to a time server
	t = now();
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
	// Every hour or so, ping for an update of NTP.
	if (minuteCounter >= 60) {
		minuteCounter = 0;
		sendNTPpacket(timeServer); // send an NTP packet to a time server
	}
	else
		minuteCounter++;
	printToLcd();
	Serial.print(DHT.humidity,1);
	Serial.print(",\t");
	Serial.println(DHT.temperature,1);
	Serial.print("Gas: ");
	Serial.println(gas,DEC);
	delay(1000);
	if (Udp.parsePacket()) { 
		epoch = handleNTPResponse();
		setTime(epoch); 
	}
	Serial.print("Unix Time = ");
	Serial.println(now());
	wttrLog = SD.open("wttrLog.csv", FILE_WRITE);
	wttrLog.print("Unix Time = ");
	wttrLog.println(now());
	wttrLog.close();
	Ethernet.maintain();
}
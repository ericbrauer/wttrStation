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

IPAddress timeServer(132, 163, 4, 101);

const int timeZone = -4;

dht11 DHT;
// initialize the library with the numbers of the interface pins
ShiftLCD lcd(2, 4, 3);

#define DHT11_PIN 5

// The following are defs required for NTP

unsigned int localPort = 8888;       // local port to listen for UDP packets

//const char timeServer[] = "time.nist.gov"; // time.nist.gov NTP server

//const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

//byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

int minuteCounter = 0;

//unsigned long epoch;

time_t prevDisplay = 0;

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
	if (timeStatus() != timeNotSet) {
		lcd.setCursor(11,0);
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
	if (!SD.begin(4)) {
		Serial.println("SD Card initialization failed!");

	}
	Serial.println("initialization complete.");
	wttrLog = SD.open("wttrLog.csv", FILE_WRITE);
	wttrLog.println("Beginning of log.");
	wttrLog.close();	
	// start Ethernet and UDP
	if (Ethernet.begin(mac) == 0) {
		Serial.println("Failed to configure Ethernet using DHCP");
		// raise flag to indicate on LCD.
	}
	Serial.print("IP Address is ");
	Serial.println(Ethernet.localIP());
	// set up the LCD's number of rows and columns: 
    lcd.begin(16, 2);
	lcd.setCursor(0, 0);
	lcd.print(Ethernet.localIP());
	Serial.println("Type,\tstatus,\tHumidity (%),\tTemperature (C)");
	Udp.begin(localPort);
	setSyncProvider(getNtpTime);
	//sendNTPpacket(timeServer); // send an NTP packet to a time server
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
	if (timeStatus() != timeNotSet) {
		if (now() != prevDisplay) {
			prevDisplay = now();

		}
	}
	if (minuteCounter >= 3600) {
		minuteCounter = 0;
		setSyncProvider(getNtpTime); // send an NTP packet to a time server
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
	Serial.print("Unix Time = ");
	Serial.println(now());
	wttrLog = SD.open("wttrLog.csv", FILE_WRITE);
	wttrLog.print("Unix Time = ");
	wttrLog.println(now());
	wttrLog.close();
	Ethernet.maintain();
}

/*-----------NTP Code for TimeLib.h------------------------------------------*/

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

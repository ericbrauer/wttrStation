# wttrStation
Simple weather tracking station (temperature, humidity, more TBA) using Arduino. 

# Features
- Temperature and humidity implemented so far using DHT11.
- LCD displays aforementioned readings
- Gas sensor is iffy, most likely will swap it out for a barometer
- Syncs time with NTP using timeLib library.
- Stores values every minute or so to SD CArd.
- [ ] Implement Web Server in order to be able to download Stored readings
- [x] Implement Barometer
- [ ] How to handle power/network outages? Start a new file, so that nothing get overwritten?
- [ ] Improve on the tz issue. Deadline: November, ha!
- [ ] Might be nice to lay everything on a PCB, and figure out a way for sensors to remain outside.
- [ ] Extra controls: manual time adjustments, brightness of LCD, display IP address.
- [ ] Brightness sensor. Record TOD or cloud cover.

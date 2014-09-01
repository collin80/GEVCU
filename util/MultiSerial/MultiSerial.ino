/*
 Transmit serial data back and forth between two ports
 in order to flash new firmware to ichip with the ichip config tool.

 Note: Steps 1-5 have to be completed within 30 seconds (30000ms)
 
 Steps:
 1) reset arduino (e.g. power cycle)
 2) start ichip software
 3) menu "serial ports", select com port of ardiuno, set baud rate to 9600
 4) click on "ichip uploader via serial"
 5) select EBI flash 16 Megabit (if window doesn't appear, start over, act faster)
 6) Click FW Update, select .imf file and wait until finished, close ichip program
 7) power cycle arduino/gevcu and wait 20-30sec
 8) menu "serial ports", set baud rate to 115200
 9) go to configuration page

 check-out http://www.youtube.com/watch?v=vS60htz6h1g at 00:35:00

 */

bool flag = true;
void setup() {
	SerialUSB.begin(9600); // use SerialUSB only as the programming port doesn't work
	Serial2.begin(9600); // use Serial3 for GEVCU2, use Serial2 for GEVCU3+4
//	pinMode(43, INPUT);
	pinMode(18, OUTPUT);
	digitalWrite(18, LOW);
}

void loop() {
	while (Serial2.available()) {
		SerialUSB.write(Serial2.read());
	}
	while (SerialUSB.available()) {
		Serial2.write(SerialUSB.read());
	}
  
        if (millis() > 6000) {
            digitalWrite(18, HIGH);
        }

	if (flag && millis() > 30000) {
		SerialUSB.begin(115200);
		Serial2.begin(115200);
		flag = false;
	}
}

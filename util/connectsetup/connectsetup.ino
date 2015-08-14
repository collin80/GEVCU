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
char inbyte;
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
          inbyte=SerialUSB.read();
          if (inbyte=='~'){sendcommandlist();}
		else {Serial2.write(inbyte);}
	}
  
      if (millis() > 6000) {
           digitalWrite(18, HIGH);
        }

	
}

void sendcommandlist()
{
  SerialUSB.println("Sending commands....");
  
sendmessage("AT+iFD");
sendmessage("AT+iHIF=1");
sendmessage("AT+iBDRA");
sendmessage("AT+iWLCH=8");
sendmessage("AT+iWLSI=GEVCU1");
sendmessage("AT+iDIP=192.168.1.46");
sendmessage("AT+iSNET=255.255.255.0");
sendmessage("AT+iDPSZ=8");
sendmessage("AT+iWST0=4");
sendmessage("AT+iWPP0=<usatoday>");
sendmessage("AT+iRPG=secret");
sendmessage("AT+iWPWD=secret");

sendmessage("AT+iAWS=1");
sendmessage("AT+iSTAP=1");
sendmessage("AT+iDOWN");

  SerialUSB.println("Command list completed....");
}

void sendmessage(char* message)
{
  Serial2.println(message);
    delay(500);
    while (Serial2.available()) {SerialUSB.write(Serial2.read());}

}

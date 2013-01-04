/*
  MCP2515.cpp - Library for Microchip MCP2515 CAN Controller
  
  Author: David Harding
  Maintainer: RechargeCar Inc (http://rechargecar.com)
  
  Created: 11/08/2010
  
  For further information see:
  
  http://ww1.microchip.com/downloads/en/DeviceDoc/21801e.pdf
  http://en.wikipedia.org/wiki/CAN_bus


  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "Arduino.h"
#include "SPI.h"
#include "MCP2515.h"
#include "MCP2515_defs.h"

MCP2515::MCP2515(byte CS_Pin, byte RESET_Pin, byte INT_Pin) {
  pinMode(CS_Pin, OUTPUT);
  digitalWrite(CS_Pin,HIGH);
  pinMode(RESET_Pin,OUTPUT);
  digitalWrite(RESET_Pin,HIGH);
  pinMode(INT_Pin,INPUT);
  digitalWrite(INT_Pin,HIGH);
  
  _CS = CS_Pin;
  _RESET = RESET_Pin;
  _INT = INT_Pin;
}

/*
  Initialize MCP2515
  
  int CAN_Bus_Speed = transfer speed in kbps
  int Freq = MCP2515 oscillator frequency in MHz
  int SJW = Synchronization Jump Width Length bits - 1 to 4 (see data sheet)
  
  returns baud rate set
  
  Sending a bus speed of 0 kbps initiates AutoBaud and returns zero if no
  baud rate could be determined.  There must be two other active nodes on the bus!
*/
int MCP2515::Init(int CAN_Bus_Speed, byte Freq) {
  if(CAN_Bus_Speed>0) {
    if(_init(CAN_Bus_Speed, Freq, 1, false)) {
	    return CAN_Bus_Speed;
    }
  } else {
	int i=0;
	byte interruptFlags = 0;
	for(i=5; i<1000; i=i+5) {
	  if(_init(i, Freq, 1, true)) {
		// check for bus activity
		Write(CANINTF,0);
		delay(500); // need the bus to be communicating within this time frame
		if(Interrupt()) {
		  // determine which interrupt flags have been set
		  interruptFlags = Read(CANINTF);
		  if(!(interruptFlags & MERRF)) {
		    // to get here we must have received something without errors
		    Mode(MODE_NORMAL);
			return i;
		  }
		}
	  }
	}
  }
  return 0;
}

int MCP2515::Init(int CAN_Bus_Speed, byte Freq, byte SJW) {
  if(SJW < 1) SJW = 1;
  if(SJW > 4) SJW = 4;
  if(CAN_Bus_Speed>0) {
    if(_init(CAN_Bus_Speed, Freq, SJW, false)) {
	    return CAN_Bus_Speed;
    }
  } else {
	int i=0;
	byte interruptFlags = 0;
	for(i=5; i<1000; i=i+5) {
	  if(_init(i, Freq, SJW, true)) {
		// check for bus activity
		Write(CANINTF,0);
		delay(500); // need the bus to be communicating within this time frame
		if(Interrupt()) {
		  // determine which interrupt flags have been set
		  interruptFlags = Read(CANINTF);
		  if(!(interruptFlags & MERRF)) {
		    // to get here we must have received something without errors
		    Mode(MODE_NORMAL);
			return i;
		  }
		}
	  }
	}
  }
  return 0;
}

bool MCP2515::_init(int CAN_Bus_Speed, byte Freq, byte SJW, bool autoBaud) {
  
  // Reset MCP2515 which puts it in configuration mode
  Reset();
  
  // Calculate bit timing registers
  byte BRP;
  float TQ;
  byte BT;
  float tempBT;

  float NBT = 1.0 / (float)CAN_Bus_Speed * 1000.0; // Nominal Bit Time
  for(BRP=0;BRP<8;BRP++) {
    TQ = 2.0 * (float)(BRP + 1) / (float)Freq;
    tempBT = NBT / TQ;
	if(tempBT<=25) {
	  BT = (int)tempBT;
	  if(tempBT-BT==0) break;
	}
  }
  
  byte SPT = (0.7 * BT); // Sample point
  byte PRSEG = (SPT - 1) / 2;
  byte PHSEG1 = SPT - PRSEG - 1;
  byte PHSEG2 = BT - PHSEG1 - PRSEG - 1;

  // Programming requirements
  if(PRSEG + PHSEG1 < PHSEG2) return false;
  if(PHSEG2 <= SJW) return false;
  
  byte BTLMODE = 1;
  byte SAM = 0;
  
  // Set registers
  byte data = (((SJW-1) << 6) | BRP);
  Write(CNF1, data);
  Write(CNF2, ((BTLMODE << 7) | (SAM << 6) | ((PHSEG1-1) << 3) | (PRSEG-1)));
  Write(CNF3, (B10000000 | (PHSEG2-1)));
  Write(TXRTSCTRL,0);
  
  if(!autoBaud) {
    // Return to Normal mode
	if(!Mode(MODE_NORMAL)) return false;
  } else {
    // Set to Listen Only mode
	if(!Mode(MODE_LISTEN)) return false;
  }
  // Enable all interupts
  Write(CANINTE,255);
  
  // Test that we can read back from the MCP2515 what we wrote to it
  byte rtn = Read(CNF1);
  return (rtn==data);
}

void MCP2515::Reset() {
  digitalWrite(_CS,LOW);
  SPI.transfer(CAN_RESET);
  digitalWrite(_CS,HIGH);
}

void MCP2515::Reset(byte hardReset) {
  // byte hardReset does nothing, it simply allows for a function overload
  digitalWrite(_RESET,LOW);
  delay(1);
  digitalWrite(_RESET,HIGH);
}

byte MCP2515::Read(byte address) {
  digitalWrite(_CS,LOW);
  SPI.transfer(CAN_READ);
  SPI.transfer(address);
  byte data = SPI.transfer(0x00);
  digitalWrite(_CS,HIGH);
  return data;
}

void MCP2515::Read(byte address, byte data[], byte bytes) {
  // allows for sequential reading of registers starting at address - see data sheet
  byte i;
  digitalWrite(_CS,LOW);
  SPI.transfer(CAN_READ);
  SPI.transfer(address);
  for(i=0;i<bytes;i++) {
    data[i] = SPI.transfer(0x00);
  }
  digitalWrite(_CS,HIGH);
}

Frame MCP2515::ReadBuffer(byte buffer) {
 
  // Reads an entire RX buffer.
  // buffer should be either RXB0 or RXB1
  
  Frame message;
  
  digitalWrite(_CS,LOW);
  SPI.transfer(CAN_READ_BUFFER | (buffer<<1));
  byte byte1 = SPI.transfer(0x00); // RXBnSIDH
  byte byte2 = SPI.transfer(0x00); // RXBnSIDL
  byte byte3 = SPI.transfer(0x00); // RXBnEID8
  byte byte4 = SPI.transfer(0x00); // RXBnEID0
  byte byte5 = SPI.transfer(0x00); // RXBnDLC

  message.srr=(byte2 & B00010000);
  message.ide=(byte2 & B00001000);

  if(message.ide) {
    message.id = (byte1>>3);
    message.id = (message.id<<8) | ((byte1<<5) | ((byte2>>5)<<2) | (byte2 & B00000011));
    message.id = (message.id<<8) | byte3;
    message.id = (message.id<<8) | byte4;
  } else {
    message.id = ((byte1>>5)<<8) | ((byte1<<3) | (byte2>>5));
  }

  message.rtr=(byte5 & B01000000);
  message.dlc = (byte5 & B00001111);  // Number of data bytes
  for(int i=0;i<message.dlc;i++) {
    message.data[i] = SPI.transfer(0x00);
  }
  digitalWrite(_CS,HIGH);

  return message;
}

void MCP2515::Write(byte address, byte data) {
  digitalWrite(_CS,LOW);
  SPI.transfer(CAN_WRITE);
  SPI.transfer(address);
  SPI.transfer(data);
  digitalWrite(_CS,HIGH);
}

void MCP2515::Write(byte address, byte data[], byte bytes) {
  // allows for sequential writing of registers starting at address - see data sheet
  byte i;
  digitalWrite(_CS,LOW);
  SPI.transfer(CAN_WRITE);
  SPI.transfer(address);
  for(i=0;i<bytes;i++) {
    SPI.transfer(data[i]);
  }
  digitalWrite(_CS,HIGH);
}

void MCP2515::SendBuffer(byte buffers) {
  // buffers should be any combination of TXB0, TXB1, TXB2 ORed together, or TXB_ALL
  digitalWrite(_CS,LOW);
  SPI.transfer(CAN_RTS | buffers);
  digitalWrite(_CS,HIGH);
}

void MCP2515::LoadBuffer(byte buffer, Frame message) {
 
  // buffer should be one of TXB0, TXB1 or TXB2
  if(buffer==TXB0) buffer = 0;

  byte byte1=0; // TXBnSIDH
  byte byte2=0; // TXBnSIDL
  byte byte3=0; // TXBnEID8
  byte byte4=0; // TXBnEID0
  byte byte5=0; // TXBnDLC

  if(message.ide) {
    byte1 = byte((message.id<<3)>>24); // 8 MSBits of SID
	byte2 = byte((message.id<<11)>>24) & B11100000; // 3 LSBits of SID
	byte2 = byte2 | byte((message.id<<14)>>30); // 2 MSBits of EID
	byte2 = byte2 | B00001000; // EXIDE
    byte3 = byte((message.id<<16)>>24); // EID Bits 15-8
    byte4 = byte((message.id<<24)>>24); // EID Bits 7-0
  } else {
    byte1 = byte((message.id<<21)>>24); // 8 MSBits of SID
	byte2 = byte((message.id<<29)>>24) & B11100000; // 3 LSBits of SID
    byte3 = 0; // TXBnEID8
    byte4 = 0; // TXBnEID0
  }
  byte5 = message.dlc;
  if(message.rtr) {
    byte5 = byte5 | B01000000;
  }
  
  digitalWrite(_CS,LOW);
  SPI.transfer(CAN_LOAD_BUFFER | buffer);  
  SPI.transfer(byte1);
  SPI.transfer(byte2);
  SPI.transfer(byte3);
  SPI.transfer(byte4);
  SPI.transfer(byte5);
 
  for(int i=0;i<message.dlc;i++) {
    SPI.transfer(message.data[i]);
  }
  digitalWrite(_CS,HIGH);
}

byte MCP2515::Status() {
  digitalWrite(_CS,LOW);
  SPI.transfer(CAN_STATUS);
  byte data = SPI.transfer(0x00);
  digitalWrite(_CS,HIGH);
  return data;
  /*
  bit 7 - CANINTF.TX2IF
  bit 6 - TXB2CNTRL.TXREQ
  bit 5 - CANINTF.TX1IF
  bit 4 - TXB1CNTRL.TXREQ
  bit 3 - CANINTF.TX0IF
  bit 2 - TXB0CNTRL.TXREQ
  bit 1 - CANINTFL.RX1IF
  bit 0 - CANINTF.RX0IF
  */
}

byte MCP2515::RXStatus() {
  digitalWrite(_CS,LOW);
  SPI.transfer(CAN_RX_STATUS);
  byte data = SPI.transfer(0x00);
  digitalWrite(_CS,HIGH);
  return data;
  /*
  bit 7 - CANINTF.RX1IF
  bit 6 - CANINTF.RX0IF
  bit 5 - 
  bit 4 - RXBnSIDL.EIDE
  bit 3 - RXBnDLC.RTR
  bit 2 | 1 | 0 | Filter Match
  ------|---|---|-------------
      0 | 0 | 0 | RXF0
	  0 | 0 | 1 | RXF1
	  0 | 1 | 0 | RXF2
	  0 | 1 | 1 | RXF3
	  1 | 0 | 0 | RXF4
	  1 | 0 | 1 | RXF5
	  1 | 1 | 0 | RXF0 (rollover to RXB1)
	  1 | 1 | 1 | RXF1 (rollover to RXB1)
  */
}

void MCP2515::BitModify(byte address, byte mask, byte data) {
  // see data sheet for explanation
  digitalWrite(_CS,LOW);
  SPI.transfer(CAN_BIT_MODIFY);
  SPI.transfer(address);
  SPI.transfer(mask);
  SPI.transfer(data);
  digitalWrite(_CS,HIGH);
}

bool MCP2515::Interrupt() {
  return (digitalRead(_INT)==LOW);
}

bool MCP2515::Mode(byte mode) {
  /*
  mode can be one of the following:
  MODE_CONFIG
  MODE_LISTEN
  MODE_LOOPBACK
  MODE_SLEEP
  MODE_NORMAL
  */
  BitModify(CANCTRL, B11100000, mode);
  delay(10); // allow for any transmissions to complete
  byte data = Read(CANSTAT); // check mode has been set
  return ((data & mode)==mode);
}

/*
mask = either MASK0 or MASK1
MaskValue is either an 11 or 29 bit mask value to set
ext is true if the mask is supposed to be extended (29 bit)
*/
void MCP2515::SetRXMask(byte mask, long MaskValue, bool ext) {
	byte temp_buff[4];
	
	Mode(MODE_CONFIG); //have to be in config mode to change mask
	
	if (ext) { //fill out all 29 bits
		temp_buff[0] = byte((MaskValue << 3) >> 24);
		temp_buff[1] = byte((MaskValue << 11) >> 24) & B11100000;
		temp_buff[1] |= byte((MaskValue << 14) >> 30);
		temp_buff[2] = byte((MaskValue << 16)>>24);
		temp_buff[3] = byte((MaskValue << 24)>>24);
	}
	else { //make sure to set mask as 11 bit standard mask
		temp_buff[0] = byte((MaskValue << 21)>>24);
		temp_buff[1] = byte((MaskValue << 29) >> 24) & B11100000;
		temp_buff[2] = 0;
		temp_buff[3] = 0;
	}
	
	Write(mask, temp_buff, 4); //send the four byte mask out to the proper address
	
	Mode(MODE_NORMAL); //Maybe a good idea to figure out what old mode was and set back to that...
}

/*
filter = FILTER0, FILTER1, FILTER2, FILTER3, FILTER4, FILTER5 (pick one)
FilterValue = 11 or 29 bit filter to use
ext is true if this filter should apply to extended frames or false if it should apply to standard frames.
Do note that, while this function looks a lot like the mask setting function is is NOT identical
It might be able to be though... The setting of EXIDE would probably just be ignored by the mask
*/
void MCP2515::SetRXFilter(byte filter, long FilterValue, bool ext) {
	byte temp_buff[4];
	
	Mode(MODE_CONFIG); //have to be in config mode to change mask
	
	if (ext) { //fill out all 29 bits
		temp_buff[0] = byte((FilterValue << 3) >> 24);
		temp_buff[1] = byte((FilterValue << 11) >> 24) & B11100000;
		temp_buff[1] |= byte((FilterValue << 14) >> 30);
		temp_buff[1] |= B00001000; //set EXIDE
		temp_buff[2] = byte((FilterValue << 16)>>24);
		temp_buff[3] = byte((FilterValue << 24)>>24);
	}
	else { //make sure to set mask as 11 bit standard mask
		temp_buff[0] = byte((FilterValue << 21)>>24);
		temp_buff[1] = byte((FilterValue << 29) >> 24) & B11100000;
		temp_buff[2] = 0;
		temp_buff[3] = 0;
	}
	
	Write(filter, temp_buff, 4); //send the four byte mask out to the proper address
	
	Mode(MODE_NORMAL); //Maybe a good idea to figure out what old mode was and set back to that...	
}

//Places the given frame into the receive queue
void MCP2515::EnqueueRX(Frame& newFrame) {
	byte counter;
	rx_frames[rx_frame_write_pos].id = newFrame.id;
	rx_frames[rx_frame_write_pos].srr = newFrame.srr;
	rx_frames[rx_frame_write_pos].rtr = newFrame.rtr;
	rx_frames[rx_frame_write_pos].ide = newFrame.ide;
	rx_frames[rx_frame_write_pos].dlc = newFrame.dlc;
	for (counter = 0; counter < 8; counter++) rx_frames[rx_frame_write_pos].data[counter] = newFrame.data[counter];
	rx_frame_write_pos = (rx_frame_write_pos + 1) % 8;
}

//Places the given frame into the transmit queue
void MCP2515::EnqueueTX(Frame& newFrame) {
	byte counter;
	tx_frames[tx_frame_write_pos].id = newFrame.id;
	tx_frames[tx_frame_write_pos].srr = newFrame.srr;
	tx_frames[tx_frame_write_pos].rtr = newFrame.rtr;
	tx_frames[tx_frame_write_pos].ide = newFrame.ide;
	tx_frames[tx_frame_write_pos].dlc = newFrame.dlc;
	for (counter = 0; counter < 8; counter++) tx_frames[tx_frame_write_pos].data[counter] = newFrame.data[counter];
	tx_frame_write_pos = (tx_frame_write_pos + 1) % 8;
}

bool MCP2515::GetRXFrame(Frame &frame) {
	byte counter;
	if (rx_frame_read_pos != rx_frame_write_pos) {
		frame.id = rx_frames[rx_frame_read_pos].id;
		frame.srr = rx_frames[rx_frame_read_pos].srr;
		frame.rtr = rx_frames[rx_frame_read_pos].rtr;
		frame.ide = rx_frames[rx_frame_read_pos].ide;
		frame.dlc = rx_frames[rx_frame_read_pos].dlc;
		for (counter = 0; counter < 8; counter++) frame.data[counter] = rx_frames[rx_frame_read_pos].data[counter];
		rx_frame_read_pos = (rx_frame_read_pos + 1) % 8;
		return true;
	}
	else return false;
}

void MCP2515::intHandler(void) {
    Frame message;
    // determine which interrupt flags have been set
    byte interruptFlags = Read(CANINTF);
    
    if(interruptFlags & RX0IF) {
      // read from RX buffer 0
	message = ReadBuffer(RXB0);
     	EnqueueRX(message);
    }
    if(interruptFlags & RX1IF) {
      // read from RX buffer 1
      message = ReadBuffer(RXB1);
      EnqueueRX(message);
    }
    if(interruptFlags & TX0IF) {
      // TX buffer 0 sent
    }
    if(interruptFlags & TX1IF) {
      // TX buffer 1 sent
    }
    if(interruptFlags & TX2IF) {
      // TX buffer 2 sent
    }
    if(interruptFlags & ERRIF) {
      // error handling code
    }
    if(interruptFlags & MERRF) {
      // error handling code
      // if TXBnCTRL.TXERR set then transmission error
      // if message is lost TXBnCTRL.MLOA will be set
    }
}

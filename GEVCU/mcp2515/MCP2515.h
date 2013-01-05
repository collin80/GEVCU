/*
  MCP2515.h - Library for Microchip MCP2515 CAN Controller
  
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

#ifndef MCP2515_h
#define MCP2515_h

#include "Arduino.h"
#include "MCP2515_defs.h"

class MCP2515
{
  public:
	// Constructor defining which pins to use for CS, RESET and INT
    MCP2515(byte CS_Pin, byte RESET_Pin, byte INT_Pin);
	
	// Overloaded initialization function
	int Init(int baud, byte freq);
	int Init(int baud, byte freq, byte sjw);
	
	// Basic MCP2515 SPI Command Set
    void Reset();
    byte Read(byte address);
    void Read(byte address, byte data[], byte bytes);
	Frame ReadBuffer(byte buffer);
	void Write(byte address, byte data);
	void Write(byte address, byte data[], byte bytes);
	void LoadBuffer(byte buffer, Frame message);
	void SendBuffer(byte buffers);
	byte Status();
	byte RXStatus();
	void BitModify(byte address, byte mask, byte data);

	// Extra functions
	bool Interrupt(); // Expose state of INT pin
    void Reset(byte hardReset); // Reset using RESET pin
	// NOTE!  When using hardware reset on some boards this might also
	//        reset the Arduino if the MCP2515 RESET pin has been tied
	//        to the Arduino's reset.  Use SPI software Reset() instead!
	bool Mode(byte mode); // Returns TRUE if mode change successful
	void EnqueueRX(Frame& newFrame);
	void EnqueueTX(Frame& newFrame);
	bool GetRXFrame(Frame &frame);
	void SetRXFilter(byte filter, long FilterValue, bool ext);
	void SetRXMask(byte mask, long MaskValue, bool ext);
	void InitFilters(bool permissive);
	void intHandler();
	
  private:
	bool _init(int baud, byte freq, byte sjw, bool autoBaud);
    // Pin variables
	byte _CS;
	byte _RESET;
	byte _INT;
    // Definitions for software buffers
	volatile Frame rx_frames[8];
	volatile Frame tx_frames[8];
	volatile byte rx_frame_read_pos, rx_frame_write_pos;
  	volatile byte tx_frame_read_pos, tx_frame_write_pos;
};

#endif

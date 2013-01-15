/*
 * dmoc.c
 *
 * Interface to the DMOC - Handles sending of commands and reception of status frames
 *
 * Created: 1/13/2013 6:10:00 PM
 *  Author: Collin
 */ 

	void DMOC::DMOC() {
		step = STEP.RPM;
		selectedGear = GEARS.NEUTRAL;	
	}
	
	void DMOC::handleFrame(Frame& frame) {
		
	}
	
	/*Do note that the DMOC expects all three command frames and it expect them to happen at least twice a second. So, probably it'd be ok to essentially
	  rotate through all of them, one per tick. That gives us a time frame of 30ms for each command frame. That should be plenty fast.
	*/
	void DMOC::handleTick() {
		switch (step) {
		case STEP.RPM:
			step = STEP.TORQUE;
			break;
		case STEP.TORQUE:
			step = STEP.WATTS;
			break;
		case STEP.WATTS:
			step = STEP.RPM;
			break;
			
		}
		
	}
	
	void DMOC::setThrottle(int throt) {
		requestedThrottle = throt;	
	}
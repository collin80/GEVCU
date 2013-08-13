/*
Previously hard coded parameters were strewn all over creation. Move them
here and then get to fixing it so that nothing is hard coded anymore.
*/

/* if this is defined then the below hard coded values will be used
   in place of trying to read values from EEPROM. This is probably
   what you want during testing.
*/
#define USE_HARD_CODED

#define ThrottleRegenValue		0		//where does Regen stop (1/10 of percent)
#define	ThrottleFwdValue		175		//where does forward motion start
#define	ThrottleMapValue		665		//Where is the 1/2 way point for throttle
#define ThrottleMaxRegenValue	00		//how many percent of full regen to do with accel pedal
#define ThrottleMaxErrValue		75		//tenths of percentage allowable deviation between pedals
#define Throttle1MinValue		180		//Value ADC reads when pedal is up
#define Throttle1MaxValue		930		//Value ADC reads when pedal fully depressed
#define Throttle2MinValue		360		//Value ADC reads when pedal is up
#define Throttle2MaxValue		1900	//Value ADC reads when pedal fully depressed
	
#define	MaxTorqueValue	2000; //in tenths of a Nm
#define	MaxRPMValue		6000; //DMOC will ignore this but we can use it ourselves for limiting

#define MaxRegenWatts	20000 //in actual watts, there is no scale here
#define MaxAccelWatts	150000

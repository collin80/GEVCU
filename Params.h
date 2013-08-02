/*
Previously hard coded parameters were strewn all over creation. Move them
here and then get to fixing it so that nothing is hard coded anymore.
*/

#define ThrottleRegenValue		0    //where does Regen stop (1/10 of percent)
#define	ThrottleFwdValue		175  //where does forward motion start
#define	ThrottleMapValue		665  //Where is the 1/2 way point for throttle
#define ThrottleMaxRegenValue	00   //how many percent of full regen to do with accel pedal
#define ThrottleMaxErrValue		75   //tenths of percentage allowable deviation between pedals

#define	MaxTorqueValue	500; //in tenths so 50Nm max torque. This is plenty for testing
#define	MaxRPMValue		6000; //also plenty for a bench test

#define MaxRegenWatts	20000
#define MaxAccelWatts	150000

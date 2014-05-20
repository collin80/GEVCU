var FieldClass = {
	off		: "annunciatorOff",
	warn	: "annunciatorWarn",
	error	: "annunciatorError",
	ok		: "annunciatorOk"
};

/* These BitField definitions must be kept in synch with Status.cpp */

var Warning = { /* Status::Bitfield1 */
    warning                            : 1 << 0,  // 0x00000001
    driverShutdownPathActive           : 1 << 1,  // 0x00000002
    externalShutdownPath1Off           : 1 << 2,  // 0x00000004
    externalShutdownPath2Off           : 1 << 3,  // 0x00000008
    oscillationLimitControllerActive   : 1 << 4,  // 0x00000010
    speedSensorSignal                  : 1 << 5,  // 0x00000020
    maximumModulationLimiter           : 1 << 6,  // 0x00000040
    temperatureSensor                  : 1 << 7,  // 0x00000080
    limitationTorque                   : 1 << 12, // 0x00001000
    limitationMaxTorque                : 1 << 13, // 0x00002000
    limitationSpeed                    : 1 << 14, // 0x00004000
    limitationControllerTemperature    : 1 << 15, // 0x00008000
    limitationMotorTemperature         : 1 << 16, // 0x00010000
    limitationSlewRate                 : 1 << 17, // 0x00020000
    limitationMotorModel               : 1 << 18, // 0x00040000
    limitationMechanicalPower          : 1 << 19, // 0x00080000
    limitationAcVoltage                : 1 << 20, // 0x00100000
    limitationAcCurrent                : 1 << 21, // 0x00200000
    limitationDcVoltage                : 1 << 22, // 0x00400000
    limitationDcCurrent                : 1 << 23, // 0x00800000
};

var Error = { /* Status::Bitfield2 */
    error                              : 1 << 0,  // 0x00000001
    speedSensor                        : 1 << 1,  // 0x00000002
    speedSensorSupply                  : 1 << 2,  // 0x00000004
    canLimitMessageInvalid             : 1 << 3,  // 0x00000008
    canControlMessageInvalid           : 1 << 4,  // 0x00000010
    canLimitMessageLost                : 1 << 5,  // 0x00000020
    overvoltageInternalSupply          : 1 << 6,  // 0x00000040
    voltageMeasurement                 : 1 << 7,  // 0x00000080
    shortCircuit                       : 1 << 8,  // 0x00000100
    canControlMessageLost              : 1 << 9,  // 0x00000200
    canControl2MessageLost             : 1 << 10, // 0x00000400
    overtempController                 : 1 << 11, // 0x00000800
    overtempMotor                      : 1 << 12, // 0x00001000
    overspeed                          : 1 << 13, // 0x00002000
    hvUndervoltage                     : 1 << 14, // 0x00004000
    hvOvervoltage                      : 1 << 15, // 0x00008000
    hvOvercurrent                      : 1 << 16, // 0x00010000
    acOvercurrent                      : 1 << 17, // 0x00020000
    initalisation                      : 1 << 18, // 0x00040000
    analogInput                        : 1 << 19, // 0x00080000
    unexpectedShutdown                 : 1 << 20, // 0x00100000
    powerMismatch                      : 1 << 21, // 0x00200000
    motorEeprom                        : 1 << 22, // 0x00400000
    storage                            : 1 << 23, // 0x00800000
    enableSignalLost                   : 1 << 24, // 0x01000000
    canCommunicationStartup            : 1 << 25, // 0x02000000
    internalSupply                     : 1 << 26, // 0x04000000
    osTrap                             : 1 << 31, // 0x80000000
};

var Status = { /* Status::Bitfield3 */
    ready                              : 1 << 0,  // 0x00000001
    running                            : 1 << 1,  // 0x00000002
    preChargeRelay                     : 1 << 2,  // 0x00000004
    secondaryContactorRelay            : 1 << 3,  // 0x00000008
    mainContactorRelay                 : 1 << 4,  // 0x00000010
    enableOut                          : 1 << 5,  // 0x00000020
    coolingRelay                       : 1 << 6,  // 0x00000040
    brakeLight                         : 1 << 7,  // 0x00000080
    reverseLight                       : 1 << 8,  // 0x00000100
    enableIn                           : 1 << 9,  // 0x00000200
    
    digitalOutput0                     : 1 << 20, // 0x00100000
    digitalOutput1                     : 1 << 21, // 0x00200000
    digitalOutput2                     : 1 << 22, // 0x00400000
    digitalOutput3                     : 1 << 23, // 0x00800000
    digitalOutput4                     : 1 << 24, // 0x01000000
    digitalOutput5                     : 1 << 25, // 0x02000000
    digitalOutput6                     : 1 << 26, // 0x04000000
    digitalOutput7                     : 1 << 27, // 0x08000000
    digitalInput0                      : 1 << 28, // 0x10000000
    digitalInput1                      : 1 << 29, // 0x20000000
    digitalInput2                      : 1 << 30, // 0x40000000
    digitalInput3                      : 1 << 31, // 0x80000000
};


function updateAnnunciatorFields(name, bitfield) {
	switch (name) {
	case 'bitfield1':
		updateField("warning", FieldClass.warn, bitfield & Warning.warning);
		updateField("driverShutdownPathActive", FieldClass.warn, bitfield & Warning.driverShutdownPathActive);
		updateField("externalShutdownPath1Off", FieldClass.warn, bitfield & Warning.externalShutdownPath1Off);
		updateField("externalShutdownPath2Off", FieldClass.warn, bitfield & Warning.externalShutdownPath2Off);
		updateField("oscillationLimitControllerActive", FieldClass.warn, bitfield & Warning.oscillationLimitControllerActive);
		updateField("speedSensorSignal", FieldClass.warn, bitfield & Warning.speedSensorSignal);
		updateField("maximumModulationLimiter", FieldClass.warn, bitfield & Warning.maximumModulationLimiter);
		updateField("temperatureSensor", FieldClass.warn, bitfield & Warning.temperatureSensor);
		updateField("limitationTorque", FieldClass.warn, bitfield & Warning.limitationTorque);
		updateField("limitationMaxTorque", FieldClass.warn, bitfield & Warning.limitationMaxTorque);
		updateField("limitationSpeed", FieldClass.warn, bitfield & Warning.limitationSpeed);
		updateField("limitationControllerTemperature", FieldClass.warn, bitfield & Warning.limitationControllerTemperature);
		updateField("limitationMotorTemperature", FieldClass.warn, bitfield & Warning.limitationMotorTemperature);
		updateField("limitationSlewRate", FieldClass.warn, bitfield & Warning.limitationSlewRate);
		updateField("limitationMotorModel", FieldClass.warn, bitfield & Warning.limitationMotorModel);
		updateField("limitationMechanicalPower", FieldClass.warn, bitfield & Warning.limitationMechanicalPower);
		updateField("limitationAcVoltage", FieldClass.warn, bitfield & Warning.limitationAcVoltage);
		updateField("limitationAcCurrent", FieldClass.warn, bitfield & Warning.limitationAcCurrent);
		updateField("limitationDcVoltage", FieldClass.warn, bitfield & Warning.limitationDcVoltage);
		updateField("limitationDcCurrent", FieldClass.warn, bitfield & Warning.limitationDcCurrent);
		break;
	case 'bitfield2':
		updateField("error", FieldClass.error, bitfield & Error.error);
		updateField("speedSensor", FieldClass.error, bitfield & Error.speedSensor);
		updateField("speedSensorSupply", FieldClass.error, bitfield & Error.speedSensorSupply);
		updateField("canLimitMessageInvalid", FieldClass.error, bitfield & Error.canLimitMessageInvalid);
		updateField("canControlMessageInvalid", FieldClass.error, bitfield & Error.canControlMessageInvalid);
		updateField("canLimitMessageLost", FieldClass.error, bitfield & Error.canLimitMessageLost);
		updateField("overvoltageInternalSupply", FieldClass.error, bitfield & Error.overvoltageInternalSupply);
		updateField("voltageMeasurement", FieldClass.error, bitfield & Error.voltageMeasurement);
		updateField("shortCircuit", FieldClass.error, bitfield & Error.shortCircuit);
		updateField("canControlMessageLost", FieldClass.error, bitfield & Error.canControlMessageLost);
		updateField("canControl2MessageLost", FieldClass.error, bitfield & Error.canControl2MessageLost);
		updateField("overtempController", FieldClass.error, bitfield & Error.overtempController);
		updateField("overtempMotor", FieldClass.error, bitfield & Error.overtempMotor);
		updateField("overspeed", FieldClass.error, bitfield & Error.overspeed);
		updateField("hvUndervoltage", FieldClass.error, bitfield & Error.hvUndervoltage);
		updateField("hvOvervoltage", FieldClass.error, bitfield & Error.hvOvervoltage);
		updateField("hvOvercurrent", FieldClass.error, bitfield & Error.hvOvercurrent);
		updateField("acOvercurrent", FieldClass.error, bitfield & Error.acOvercurrent);
		updateField("initalisation", FieldClass.error, bitfield & Error.initalisation);
		updateField("analogInput", FieldClass.error, bitfield & Error.analogInput);
		updateField("unexpectedShutdown", FieldClass.error, bitfield & Error.unexpectedShutdown);
		updateField("powerMismatch", FieldClass.error, bitfield & Error.powerMismatch);
		updateField("motorEeprom", FieldClass.error, bitfield & Error.motorEeprom);
		updateField("storage", FieldClass.error, bitfield & Error.storage);
		updateField("enableSignalLost", FieldClass.error, bitfield & Error.enableSignalLost);
		updateField("canCommunicationStartup", FieldClass.error, bitfield & Error.canCommunicationStartup);
		updateField("internalSupply", FieldClass.error, bitfield & Error.internalSupply);
		updateField("osTrap", FieldClass.error, bitfield & Error.osTrap);
		break;
	case 'bitfield3':
		updateField("ready", FieldClass.ok, bitfield & Status.ready);
		updateField("running", FieldClass.ok, bitfield & Status.running);
		updateField("enableIn", FieldClass.ok, bitfield & Status.enableIn);
		updateField("preChargeRelay", FieldClass.ok, bitfield & Status.preChargeRelay);
		updateField("mainContactorRelay", FieldClass.ok, bitfield & Status.mainContactorRelay);
		updateField("secondaryContactorRelay", FieldClass.ok, bitfield & Status.secondaryContactorRelay);
		updateField("enableOut", FieldClass.ok, bitfield & Status.enableOut);
		updateField("coolingRelay", FieldClass.ok, bitfield & Status.coolingRelay);
		updateField("brakeLight", FieldClass.ok, bitfield & Status.brakeLight);
		updateField("reverseLight", FieldClass.ok, bitfield & Status.reverseLight);
		updateField("digitalOutput0", FieldClass.ok, bitfield & Status.digitalOutput0);
		updateField("digitalOutput1", FieldClass.ok, bitfield & Status.digitalOutput1);
		updateField("digitalOutput2", FieldClass.ok, bitfield & Status.digitalOutput2);
		updateField("digitalOutput3", FieldClass.ok, bitfield & Status.digitalOutput3);
		updateField("digitalOutput4", FieldClass.ok, bitfield & Status.digitalOutput4);
		updateField("digitalOutput5", FieldClass.ok, bitfield & Status.digitalOutput5);
		updateField("digitalOutput6", FieldClass.ok, bitfield & Status.digitalOutput6);
		updateField("digitalOutput7", FieldClass.ok, bitfield & Status.digitalOutput7);
		updateField("digitalInput0", FieldClass.ok, bitfield & Status.digitalInput0);
		updateField("digitalInput1", FieldClass.ok, bitfield & Status.digitalInput1);
		updateField("digitalInput2", FieldClass.ok, bitfield & Status.digitalInput2);
		updateField("digitalInput3", FieldClass.ok, bitfield & Status.digitalInput3);
		break;
	}
}

function updateField(id, fieldClass, flag) {
	var target = document.getElementById(id);
	if (target)
		target.className = (flag == 0 ? FieldClass.off : fieldClass);
}
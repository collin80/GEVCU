var FieldClass = {
	off		: "annunciatorOff",
	warn	: "annunciatorWarn",
	error	: "annunciatorError",
	ok		: "annunciatorOk"
};

var Status = { /* statusBitfield1 */
	out0		: 1 << 0,
	out1		: 1 << 1,
	out2		: 1 << 2,
	out3		: 1 << 3,
	out4		: 1 << 4,
	out5		: 1 << 5,
	out6		: 1 << 6,
	out7		: 1 << 7,
	torqueLimitation			: 1 << 8,
	errorFlag					: 1 << 9,
	warningFlag					: 1 << 10,
	slewRateLimitation			: 1 << 12,
	motorTemperatureLimitation	: 1 << 13,
	stateRunning				: 1 << 14,
	stateReady					: 1 << 15
};

var Warning = { /* statusBitfield2 */
	systemCheckActive					: 1 << 0,
	externalShutdownPathAw2Off			: 1 << 1,
	externalShutdownPathAw1Off			: 1 << 2,
	oscillationLimitControllerActive	: 1 << 3,
	driverShutdownPathActive			: 1 << 10,
	powerMismatchDetected				: 1 << 11,
	speedSensorSignal					: 1 << 12,
	hvUndervoltage						: 1 << 13,
	maximumModulationLimiter			: 1 << 14,
	temperatureSensor					: 1 << 15,
	res1								: 1 << 16,
	res2								: 1 << 17,
	res3								: 1 << 18,
	res4								: 1 << 19
};

var Error = { /* statusBitfield3 */
	speedSensorSupply			: 1 << 0,
	speedSensor					: 1 << 1,
	canLimitMessageInvalid		: 1 << 2,
	canControlMessageInvalid	: 1 << 3,
	canLimitMessageLost			: 1 << 4,
	overvoltageSkyConverter		: 1 << 5,
	voltageMeasurement			: 1 << 6,
	shortCircuit				: 1 << 7,
	canControlMessageLost		: 1 << 8,
	overtemp					: 1 << 9,
	overtempMotor				: 1 << 10,
	overspeed					: 1 << 11,
	undervoltage				: 1 << 12,
	overvoltage					: 1 << 13,
	overcurrent					: 1 << 14,
	initalisation				: 1 << 15,
	analogInput					: 1 << 16,
	driverShutdown				: 1 << 22,
	powerMismatch				: 1 << 23,
	canControl2MessageLost		: 1 << 24,
	motorEeprom					: 1 << 25,
	storage						: 1 << 26,
	enablePinSignalLost			: 1 << 27,
	canCommunicationStartup		: 1 << 28,
	internalSupply				: 1 << 29,
	acOvercurrent				: 1 << 30,
	osTrap						: 1 << 31
};

function updateAnnunciatorFields(name, bitfield) {
	switch (name) {
	case 'bitfield1':
		updateField("out0", FieldClass.ok, bitfield & Status.out0);
		updateField("out1", FieldClass.ok, bitfield & Status.out1);
		updateField("out2", FieldClass.ok, bitfield & Status.out2);
		updateField("out3", FieldClass.ok, bitfield & Status.out3);
		updateField("out4", FieldClass.ok, bitfield & Status.out4);
		updateField("out5", FieldClass.ok, bitfield & Status.out5);
		updateField("out6", FieldClass.ok, bitfield & Status.out6);
		updateField("out7", FieldClass.ok, bitfield & Status.out7);
		updateField("torqueLimitation", FieldClass.warn, bitfield & Status.torqueLimitation);
		updateField("errorFlag", FieldClass.error, bitfield & Status.errorFlag);
		updateField("warningFlag", FieldClass.warn, bitfield & Status.warningFlag);
		updateField("slewRateLimitation", FieldClass.warn, bitfield & Status.slewRateLimitation);
		updateField("motorTemperatureLimitation", FieldClass.warn, bitfield & Status.motorTemperatureLimitation);
		updateField("stateRunning", FieldClass.ok, bitfield & Status.stateRunning);
		updateField("stateReady", FieldClass.ok, bitfield & Status.stateReady);
		break;
	case 'bitfield2':
		updateField("systemCheckActive", FieldClass.warn, bitfield & Warning.systemCheckActive);
		updateField("externalShutdownPathAw2Off", FieldClass.warn, bitfield & Warning.externalShutdownPathAw2Off);
		updateField("externalShutdownPathAw1Off", FieldClass.warn, bitfield & Warning.externalShutdownPathAw1Off);
		updateField("oscillationLimitControllerActive", FieldClass.ok, bitfield & Warning.oscillationLimitControllerActive);
		updateField("driverShutdownPathActive", FieldClass.warn, bitfield & Warning.driverShutdownPathActive);
		updateField("powerMismatchDetected", FieldClass.warn, bitfield & Warning.powerMismatchDetected);
		updateField("speedSensorSignal", FieldClass.warn, bitfield & Warning.speedSensorSignal);
		updateField("hvUndervoltage", FieldClass.warn, bitfield & Warning.hvUndervoltage);
		updateField("maximumModulationLimiter", FieldClass.warn, bitfield & Warning.maximumModulationLimiter);
		updateField("temperatureSensor", FieldClass.warn, bitfield & Warning.temperatureSensor);
		updateField("res1", FieldClass.ok, bitfield & Warning.res1);
		updateField("res2", FieldClass.ok, bitfield & Warning.res2);
		updateField("res3", FieldClass.ok, bitfield & Warning.res3);
		updateField("res4", FieldClass.ok, bitfield & Warning.res4);
		break;
	case 'bitfield3':
		updateField("speedSensorSupply", FieldClass.error, bitfield & Error.speedSensorSupply);
		updateField("speedSensor", FieldClass.error, bitfield & Error.speedSensor);
		updateField("canLimitMessageInvalid", FieldClass.error, bitfield & Error.canLimitMessageInvalid);
		updateField("canControlMessageInvalid", FieldClass.error, bitfield & Error.canControlMessageInvalid);
		updateField("canLimitMessageLost", FieldClass.error, bitfield & Error.canLimitMessageLost);
		updateField("overvoltageSkyConverter", FieldClass.error, bitfield & Error.overvoltageSkyConverter);
		updateField("voltageMeasurement", FieldClass.error, bitfield & Error.voltageMeasurement);
		updateField("shortCircuit", FieldClass.error, bitfield & Error.shortCircuit);
		updateField("canControlMessageLost", FieldClass.error, bitfield & Error.canControlMessageLost);
		updateField("overtemp", FieldClass.error, bitfield & Error.overtemp);
		updateField("overtempMotor", FieldClass.error, bitfield & Error.overtempMotor);
		updateField("overspeed", FieldClass.error, bitfield & Error.overspeed);
		updateField("undervoltage", FieldClass.error, bitfield & Error.undervoltage);
		updateField("overvoltage", FieldClass.error, bitfield & Error.overvoltage);
		updateField("overcurrent", FieldClass.error, bitfield & Error.overcurrent);
		updateField("initalisation", FieldClass.error, bitfield & Error.initalisation);
		updateField("analogInput", FieldClass.error, bitfield & Error.analogInput);
		updateField("driverShutdown", FieldClass.error, bitfield & Error.driverShutdown);
		updateField("powerMismatch", FieldClass.error, bitfield & Error.powerMismatch);
		updateField("canControl2MessageLost", FieldClass.error, bitfield & Error.canControl2MessageLost);
		updateField("motorEeprom", FieldClass.error, bitfield & Error.motorEeprom);
		updateField("storage", FieldClass.error, bitfield & Error.storage);
		updateField("enablePinSignalLost", FieldClass.error, bitfield & Error.enablePinSignalLost);
		updateField("canCommunicationStartup", FieldClass.error, bitfield & Error.canCommunicationStartup);
		updateField("internalSupply", FieldClass.error, bitfield & Error.internalSupply);
		updateField("acOvercurrent", FieldClass.error, bitfield & Error.acOvercurrent);
		updateField("osTrap", FieldClass.error, bitfield & Error.osTrap);
		break;
	case 'bitfield4':
		break;
	}
}

function updateField(id, fieldClass, flag) {
	var target = document.getElementById(id);
	if (target)
		target.className = (flag == 0 ? FieldClass.off : fieldClass);
}
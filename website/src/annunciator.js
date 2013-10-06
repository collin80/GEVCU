var Status = { /* statusBitfield1 */
	motorModelLimitation : 1 << 0,
	mechanicalPowerLimitation : 1 << 1,
	maxTorqueLimitation : 1 << 2,
	acCurrentLimitation : 1 << 3,
	temperatureLimitation : 1 << 4,
	speedLimitation : 1 << 5,
	voltageLimitation : 1 << 6,
	currentLimitation : 1 << 7,
	torqueLimitation : 1 << 8,
	errorFlag : 1 << 9,
	warningFlag : 1 << 10,
	slewRateLimitation : 1 << 12,
	motorTemperatureLimitation : 1 << 13,
	stateRunning : 1 << 14,
	stateReady : 1 << 15
};

var Warning = { /* statusBitfield2 */
	systemCheckActive : 1 << 0,
	externalShutdownPathAw2Off : 1 << 1,
	externalShutdownPathAw1Off : 1 << 2,
	oscillationLimitControllerActive : 1 << 3,
	driverShutdownPathActive : 1 << 10,
	powerMismatchDetected : 1 << 11,
	speedSensorSignal : 1 << 12,
	hvUndervoltage : 1 << 13,
	maximumModulationLimiter : 1 << 14,
	temperatureSensor : 1 << 15,
};

var Error = { /* statusBitfield3 */
	speedSensorSupply : 1 << 0,
	speedSensor : 1 << 1,
	canLimitMessageInvalid : 1 << 2,
	canControlMessageInvalid : 1 << 3,
	canLimitMessageLost : 1 << 4,
	overvoltageSkyConverter : 1 << 5,
	voltageMeasurement : 1 << 6,
	shortCircuit : 1 << 7,
	canControlMessageLost : 1 << 8,
	overtemp : 1 << 9,
	overtempMotor : 1 << 10,
	overspeed : 1 << 11,
	undervoltage : 1 << 12,
	overvoltage : 1 << 13,
	overcurrent : 1 << 14,
	initalisation : 1 << 15,
	analogInput : 1 << 16,
	driverShutdown : 1 << 22,
	powerMismatch : 1 << 23,
	canControl2MessageLost : 1 << 24,
	motorEeprom : 1 << 25,
	storage : 1 << 26,
	enablePinSignalLost : 1 << 27,
	canCommunicationStartup : 1 << 28,
	internalSupply : 1 << 29,
	acOvercurrent : 1 << 30,
	osTrap : 1 << 31
};

var FieldType = {
	off: "annunciatorOff",
	warn: "annunciatorWarn",
	error: "annunciatorError",
	ok: "annunciatorOk"
};

function updateAnnunciatorFields(name, bitfieldxxxx) {
	var bitfield = Math.round(Math.random() * 0x10000000);
	switch (name) {
	case 'bitfield1':
		updateField("motorModelLimitation", FieldType.warn, bitfield & Status.motorModelLimitation);
		updateField("mechanicalPowerLimitation", FieldType.warn, bitfield & Status.mechanicalPowerLimitation);
		updateField("maxTorqueLimitation", FieldType.warn, bitfield & Status.maxTorqueLimitation);
		updateField("acCurrentLimitation", FieldType.warn, bitfield & Status.acCurrentLimitation);
		updateField("temperatureLimitation", FieldType.warn, bitfield & Status.temperatureLimitation);
		updateField("speedLimitation", FieldType.warn, bitfield & Status.speedLimitation);
		updateField("voltageLimitation", FieldType.warn, bitfield & Status.voltageLimitation);
		updateField("currentLimitation", FieldType.warn, bitfield & Status.currentLimitation);
		updateField("torqueLimitation", FieldType.warn, bitfield & Status.torqueLimitation);
		updateField("errorFlag", FieldType.error, bitfield & Status.errorFlag);
		updateField("warningFlag", FieldType.warn, bitfield & Status.warningFlag);
		updateField("slewRateLimitation", FieldType.warn, bitfield & Status.slewRateLimitation);
		updateField("motorTemperatureLimitation", FieldType.warn, bitfield & Status.motorTemperatureLimitation);
		updateField("stateRunning", FieldType.ok, bitfield & Status.stateRunning);
		updateField("stateReady", FieldType.ok, bitfield & Status.stateReady);
		break;
	case 'bitfield2':
		updateField("systemCheckActive", FieldType.warn, bitfield & Warning.systemCheckActive);
		updateField("externalShutdownPathAw2Off", FieldType.warn, bitfield & Warning.externalShutdownPathAw2Off);
		updateField("externalShutdownPathAw1Off", FieldType.warn, bitfield & Warning.externalShutdownPathAw1Off);
		updateField("oscillationLimitControllerActive", FieldType.warn, bitfield & Warning.oscillationLimitControllerActive);
		updateField("driverShutdownPathActive", FieldType.warn, bitfield & Warning.driverShutdownPathActive);
		updateField("powerMismatchDetected", FieldType.warn, bitfield & Warning.powerMismatchDetected);
		updateField("speedSensorSignal", FieldType.warn, bitfield & Warning.speedSensorSignal);
		updateField("hvUndervoltage", FieldType.warn, bitfield & Warning.hvUndervoltage);
		updateField("maximumModulationLimiter", FieldType.warn, bitfield & Warning.maximumModulationLimiter);
		updateField("temperatureSensor", FieldType.warn, bitfield & Warning.temperatureSensor);
		break;
	case 'bitfield3':
		updateField("speedSensorSupply", FieldType.error, bitfield & Error.speedSensorSupply);
		updateField("speedSensor", FieldType.error, bitfield & Error.speedSensor);
		updateField("canLimitMessageInvalid", FieldType.error, bitfield & Error.canLimitMessageInvalid);
		updateField("canControlMessageInvalid", FieldType.error, bitfield & Error.canControlMessageInvalid);
		updateField("canLimitMessageLost", FieldType.error, bitfield & Error.canLimitMessageLost);
		updateField("overvoltageSkyConverter", FieldType.error, bitfield & Error.overvoltageSkyConverter);
		updateField("voltageMeasurement", FieldType.error, bitfield & Error.voltageMeasurement);
		updateField("shortCircuit", FieldType.error, bitfield & Error.shortCircuit);
		updateField("canControlMessageLost", FieldType.error, bitfield & Error.canControlMessageLost);
		updateField("overtemp", FieldType.error, bitfield & Error.overtemp);
		updateField("overtempMotor", FieldType.error, bitfield & Error.overtempMotor);
		updateField("overspeed", FieldType.error, bitfield & Error.overspeed);
		updateField("undervoltage", FieldType.error, bitfield & Error.undervoltage);
		updateField("overvoltage", FieldType.error, bitfield & Error.overvoltage);
		updateField("overcurrent", FieldType.error, bitfield & Error.overcurrent);
		updateField("initalisation", FieldType.error, bitfield & Error.initalisation);
		updateField("analogInput", FieldType.error, bitfield & Error.analogInput);
		updateField("driverShutdown", FieldType.error, bitfield & Error.driverShutdown);
		updateField("powerMismatch", FieldType.error, bitfield & Error.powerMismatch);
		updateField("canControl2MessageLost", FieldType.error, bitfield & Error.canControl2MessageLost);
		updateField("motorEeprom", FieldType.error, bitfield & Error.motorEeprom);
		updateField("storage", FieldType.error, bitfield & Error.storage);
		updateField("enablePinSignalLost", FieldType.error, bitfield & Error.enablePinSignalLost);
		updateField("canCommunicationStartup", FieldType.error, bitfield & Error.canCommunicationStartup);
		updateField("internalSupply", FieldType.error, bitfield & Error.internalSupply);
		updateField("acOvercurrent", FieldType.error, bitfield & Error.acOvercurrent);
		updateField("osTrap", FieldType.error, bitfield & Error.osTrap);
		break;
	case 'bitfield4':
		break;
	}
}

function updateField(id, fieldType, flag) {
	var target = document.getElementById(id);
	if (target)
		target.className = (flag == 0 ? FieldType.off : fieldType);
}
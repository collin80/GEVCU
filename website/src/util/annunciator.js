var FieldClass = {
	off		: "annunciatorOff",
	warn	: "annunciatorWarn",
	error	: "annunciatorError",
	ok		: "annunciatorOk"
};

/* These BitField definitions must be kept in synch with Status.cpp */

var Motor = { /* Status::BitfieldMotor */
    // warnings
    warning                            : 1 << 0,  // 0x00000001
    oscillationLimiter                 : 1 << 1,  // 0x00000002
    maximumModulationLimiter           : 1 << 2,  // 0x00000004

    // errors
    overtempController                 : 1 << 8,  // 0x00000100
    overtempMotor                      : 1 << 9,  // 0x00000200
    overspeed                          : 1 << 10, // 0x00000400
    hvUndervoltage                     : 1 << 11, // 0x00000800
    hvOvervoltage                      : 1 << 12, // 0x00001000
    hvOvercurrent                      : 1 << 13, // 0x00002000
    acOvercurrent                      : 1 << 14, // 0x00004000

    // limitations
    limitationTorque                   : 1 << 16, // 0x00010000
    limitationMaxTorque                : 1 << 17, // 0x00020000
    limitationSpeed                    : 1 << 18, // 0x00040000
    limitationControllerTemperature    : 1 << 19, // 0x00080000
    limitationMotorTemperature         : 1 << 20, // 0x00100000
    limitationSlewRate                 : 1 << 21, // 0x00200000
    limitationMotorModel               : 1 << 22, // 0x00400000
    limitationMechanicalPower          : 1 << 23, // 0x00800000
    limitationAcVoltage                : 1 << 24, // 0x01000000
    limitationAcCurrent                : 1 << 25, // 0x02000000
    limitationDcVoltage                : 1 << 26, // 0x04000000
    limitationDcCurrent                : 1 << 27, // 0x08000000
};

var BMS = { /* Status::BitfieldBms */
    // status
    bmsRelayDischarge                  : 1 << 0,  // 0x00000001
    bmsRelayCharge                     : 1 << 1,  // 0x00000002
    bmsChagerSafety                    : 1 << 2,  // 0x00000004
    bmsDtcPresent                      : 1 << 3,  // 0x00000008

    bmsVoltageFailsafe                 : 1 << 8,  // 0x00000100
    bmsCurrentFailsafe                 : 1 << 9,  // 0x00000200
    bmsDepleted                        : 1 << 10, // 0x00000400
    bmsBalancingActive                 : 1 << 11, // 0x00000800
    bmsDtcWeakCellFault                : 1 << 12, // 0x00001000
    bmsDtcLowCellVolage                : 1 << 13, // 0x00002000
    bmsDtcHVIsolationFault             : 1 << 14, // 0x00004000
    bmsDtcVoltageRedundancyFault       : 1 << 15, // 0x00008000

    // limitations
    bmsDclLowSoc                       : 1 << 16, // 0x00010000
    bmsDclHighCellResistance           : 1 << 17, // 0x00020000
    bmsDclTemperature                  : 1 << 18, // 0x00040000
    bmsDclLowCellVoltage               : 1 << 19, // 0x00080000
    bmsDclLowPackVoltage               : 1 << 20, // 0x00100000
    bmsDclCclVoltageFailsafe           : 1 << 21, // 0x00200000
    bmsDclCclCommunication             : 1 << 22, // 0x00400000
    bmsCclHighSoc                      : 1 << 23, // 0x00800000
    bmsCclHighCellResistance           : 1 << 24, // 0x01000000
    bmsCclTemperature                  : 1 << 25, // 0x02000000
    bmsCclHighCellVoltage              : 1 << 26, // 0x04000000
    bmsCclHighPackVoltage              : 1 << 27, // 0x08000000
    bmsCclChargerLatch                 : 1 << 28, // 0x10000000
    bmsCclAlternate                    : 1 << 29, // 0x20000000
};

var IO = { /* Status::BitfieldIO */
    brakeHold                          : 1 << 0,  // 0x00000001
    preChargeRelay                     : 1 << 1,  // 0x00000002
    secondaryContactor                 : 1 << 2,  // 0x00000004
    mainContactor                      : 1 << 3,  // 0x00000008
    enableMotor                        : 1 << 4,  // 0x00000010
    coolingFan                         : 1 << 5,  // 0x00000020
    brakeLight                         : 1 << 6,  // 0x00000040
    reverseLight                       : 1 << 7,  // 0x00000080
    enableIn                           : 1 << 8,  // 0x00000100
    absActive                          : 1 << 9,  // 0x00000200
};

var foldTimeout = null;
var openedAutomatically = false;
var bitfieldMotor = 0;
var bitfieldBms = 0;
var bitfieldIO = 0;

function updateAnnunciatorFields(name, bitfield) {
	switch (name) {
	case 'bitfieldMotor':
	    // warnings
		updateField("warning"                       , FieldClass.warn, bitfield & Motor.warning);
		updateField("oscillationLimiter"            , FieldClass.ok, bitfield & Motor.oscillationLimiter);
		updateField("maximumModulationLimiter"      , FieldClass.warn, bitfield & Motor.maximumModulationLimiter);
	    // errors
		updateField("overtempController"            , FieldClass.error, bitfield & Motor.overtempController);
		updateField("overtempMotor"                 , FieldClass.error, bitfield & Motor.overtempMotor);
		updateField("overspeed"                     , FieldClass.error, bitfield & Motor.overspeed);
		updateField("hvUndervoltage"                , FieldClass.error, bitfield & Motor.hvUndervoltage);
		updateField("hvOvervoltage"                 , FieldClass.error, bitfield & Motor.hvOvervoltage);
		updateField("hvOvercurrent"                 , FieldClass.error, bitfield & Motor.hvOvercurrent);
		updateField("acOvercurrent"                 , FieldClass.error, bitfield & Motor.acOvercurrent);
	    // limitations
		updateField("limitationTorque"              , FieldClass.warn, bitfield & Motor.limitationTorque);
		updateField("limitationMaxTorque"           , FieldClass.warn, bitfield & Motor.limitationMaxTorque);
		updateField("limitationSpeed"               , FieldClass.warn, bitfield & Motor.limitationSpeed);
		updateField("limitationControllerTemperature", FieldClass.warn, bitfield & Motor.limitationControllerTemperature);
		updateField("limitationMotorTemperature"    , FieldClass.warn, bitfield & Motor.limitationMotorTemperature);
		updateField("limitationSlewRate"            , FieldClass.warn, bitfield & Motor.limitationSlewRate);
		updateField("limitationMotorModel"          , FieldClass.warn, bitfield & Motor.limitationMotorModel);
		updateField("limitationMechanicalPower"     , FieldClass.warn, bitfield & Motor.limitationMechanicalPower);
		updateField("limitationAcVoltage"           , FieldClass.warn, bitfield & Motor.limitationAcVoltage);
		updateField("limitationAcCurrent"           , FieldClass.warn, bitfield & Motor.limitationAcCurrent);
		updateField("limitationDcVoltage"           , FieldClass.warn, bitfield & Motor.limitationDcVoltage);
		updateField("limitationDcCurrent"           , FieldClass.warn, bitfield & Motor.limitationDcCurrent);

		bitfieldMotor = bitfield & ~~(Motor.oscillationLimiter | Motor.limitationTorque | Motor.limitationSpeed);
		break;
		
	case 'bitfieldBms':
		updateField("bmsRelayDischarge"             , FieldClass.ok, bitfield & BMS.bmsRelayDischarge);
		updateField("bmsRelayCharge"                , FieldClass.ok, bitfield & BMS.bmsRelayCharge);
		updateField("bmsChagerSafety"               , FieldClass.warn, bitfield & BMS.bmsChagerSafety);
		updateField("bmsDtcPresent"                 , FieldClass.error, bitfield & BMS.bmsDtcPresent);
		
		updateField("bmsVoltageFailsafe"            , FieldClass.error, bitfield & BMS.bmsVoltageFailsafe);
		updateField("bmsCurrentFailsafe"            , FieldClass.error, bitfield & BMS.bmsCurrentFailsafe);
		updateField("bmsDepleted"                   , FieldClass.error, bitfield & BMS.bmsDepleted);
		updateField("bmsBalancingActive"            , FieldClass.ok, bitfield & BMS.bmsBalancingActive);
		updateField("bmsDtcWeakCellFault"           , FieldClass.error, bitfield & BMS.bmsDtcWeakCellFault);
		updateField("bmsDtcLowCellVolage"           , FieldClass.error, bitfield & BMS.bmsDtcLowCellVolage);
		updateField("bmsDtcHVIsolationFault"        , FieldClass.error, bitfield & BMS.bmsDtcHVIsolationFault);
		updateField("bmsDtcVoltageRedundancyFault"  , FieldClass.error, bitfield & BMS.bmsDtcVoltageRedundancyFault);
	    
	    // limitations
		updateField("bmsDclLowSoc"                  , FieldClass.warn, bitfield & BMS.bmsDclLowSoc);
		updateField("bmsDclHighCellResistance"      , FieldClass.warn, bitfield & BMS.bmsDclHighCellResistance);
		updateField("bmsDclTemperature"             , FieldClass.warn, bitfield & BMS.bmsDclTemperature);
		updateField("bmsDclLowCellVoltage"          , FieldClass.warn, bitfield & BMS.bmsDclLowCellVoltage);
		updateField("bmsDclLowPackVoltage"          , FieldClass.warn, bitfield & BMS.bmsDclLowPackVoltage);
		updateField("bmsDclCclVoltageFailsafe"      , FieldClass.warn, bitfield & BMS.bmsDclCclVoltageFailsafe);
		updateField("bmsDclCclCommunication"        , FieldClass.warn, bitfield & BMS.bmsDclCclCommunication);
		updateField("bmsCclHighSoc"                 , FieldClass.warn, bitfield & BMS.bmsCclHighSoc);
		updateField("bmsCclHighCellResistance"      , FieldClass.warn, bitfield & BMS.bmsCclHighCellResistance);
		updateField("bmsCclTemperature"             , FieldClass.warn, bitfield & BMS.bmsCclTemperature);
		updateField("bmsCclHighCellVoltage"         , FieldClass.warn, bitfield & BMS.bmsCclHighCellVoltage);
		updateField("bmsCclHighPackVoltage"         , FieldClass.warn, bitfield & BMS.bmsCclHighPackVoltage);
		updateField("bmsCclChargerLatch"            , FieldClass.warn, bitfield & BMS.bmsCclChargerLatch);
		updateField("bmsCclAlternate"               , FieldClass.warn, bitfield & BMS.bmsCclAlternate);

		bitfieldBms = bitfield & ~~(BMS.bmsRelayDischarge | BMS.bmsRelayCharge | BMS.bmsChagerSafety);
		break;
	    
	case 'bitfieldIO':
		updateField("brakeHold"                     , FieldClass.ok, bitfield & IO.brakeHold);
		updateField("preChargeRelay"                , FieldClass.ok, bitfield & IO.preChargeRelay);
		updateField("secondaryContactor"            , FieldClass.ok, bitfield & IO.secondaryContactor);
		updateField("mainContactor"                 , FieldClass.ok, bitfield & IO.mainContactor);
		updateField("enableMotor"                   , FieldClass.ok, bitfield & IO.enableMotor);
		updateField("coolingFan"                    , FieldClass.ok, bitfield & IO.coolingFan);
		updateField("brakeLight"                    , FieldClass.ok, bitfield & IO.brakeLight);
		updateField("reverseLight"                  , FieldClass.ok, bitfield & IO.reverseLight);
		updateField("enableIn"                      , FieldClass.ok, bitfield & IO.enableIn);
		updateField("absActive"                     , FieldClass.ok, bitfield & IO.absActive);

		bitfieldIO = bitfield;
		break;
	}

	if (bitfieldMotor != 0 || bitfieldBms != 0) {
		if (foldTimeout) {
			clearTimeout(foldTimeout);
			foldTimeout = null;
		}
		if(!openedAutomatically) {
			foldAnnunciator(true);
			openedAutomatically = true;
		}
	} else if (openedAutomatically && foldTimeout == null) {
		// auto-fold after 10sec no error or warning
		foldTimeout = setTimeout(function () {
			foldAnnunciator(false);
			openedAutomatically = false;
			foldTimeout = null;
		}, 10000);
	}
}

function updateField(id, fieldClass, flag) {
	var target = nodecache[id];
	if (!target) {
		target = document.getElementById(id);
		nodecache[id] = target;
	}
	if (target)
		target.className = (flag == 0 ? FieldClass.off : fieldClass);
}

function foldAnnunciator(open) {
	var annunciators = document.getElementById('annunciators');
	if (annunciators) {
		if(open == undefined) {
			open = annunciators.getAttribute('class') != 'fly-in';
		}
		annunciators.setAttribute('class', (open ? 'fly-in' : 'fly-out'));
	}
}
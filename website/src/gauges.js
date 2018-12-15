var gaugeColors = {
	plate      : { start : '#001', end : '#253043' },
	majorTicks : '#f5f5f5',
	minorTicks : '#ddd',
	title      : '#fff',
	units      : '#ccc',
	numbers    : '#eee',
	needle     : { start : 'rgba(255, 100, 100, 1)', end : 'rgba(230, 100, 100, .9)' }
};

// calculate the min/max and ticks for a gauge
function calcRange(low, high, factor) {
	var j = 0;
	var ticks = new Array();
	var step = Math.ceil((high - low) / 50) * 5 * factor;
	var min = ((low == 0) ? 0 : (low - (low + 1) % step - step + 1));
	var max = high - (high - 1) % step + step - 1;
	for (i = min; i <= max; i += step) {
		ticks[j++] = i;
	}
	var range = {"min": min, "max": max, "ticks": ticks};
	return range;
}

function generateGauges(config) {
	var intervalBatteryVoltageLow = Math.round((config.batteryRangeLow[2] - config.batteryRangeLow[1]) / 2);
	var intervalBatteryVoltageHigh = Math.round((config.batteryRangeHigh[2] - config.batteryRangeHigh[1]) / 2);
	var rangeBatteryVoltage = calcRange(config.batteryRangeLow[0] - intervalBatteryVoltageLow, config.batteryRangeHigh[2] + intervalBatteryVoltageHigh, 3);
	var batteryVoltageHighlights = [
		{ from : rangeBatteryVoltage.min, to : config.batteryRangeLow[0], color : 'rgba(255, 0, 0, .75)' },
		{ from : config.batteryRangeLow[0], to : config.batteryRangeLow[1], color : 'rgba(255, 127, 0, .75)' },
		{ from : config.batteryRangeLow[1], to : config.batteryRangeLow[2], color : 'rgba(255, 220, 0, .75)' },
		{ from : config.batteryRangeLow[2], to : config.batteryRangeLow[2] + intervalBatteryVoltageLow, color : 'rgba(180, 255, 0, .75)' },
		{ from : config.batteryRangeLow[2] + intervalBatteryVoltageLow, to : config.batteryRangeHigh[0], color : 'rgba(0, 255,  0, .65)' },
		{ from : config.batteryRangeHigh[0], to : config.batteryRangeHigh[0] + intervalBatteryVoltageHigh, color : 'rgba(180, 255, 0, .75)' },
		{ from : config.batteryRangeHigh[0] + intervalBatteryVoltageHigh, to : config.batteryRangeHigh[1], color : 'rgba(255, 220, 0, .75)' },
		{ from : config.batteryRangeHigh[1], to : config.batteryRangeHigh[2], color : 'rgba(255, 127, 0, .75)' },
		{ from : config.batteryRangeHigh[2], to : rangeBatteryVoltage.max, color :  'rgba(255, 0, 0, .75)'}
	];

	var rangeCurrent = calcRange(config.currentRange[0], config.currentRange[1], 3);
	
	var intervalEnergy = Math.round((config.energyRange[1] - config.energyRange[0]) / 2);
	var rangeEnergy = calcRange(config.energyRange[0], config.energyRange[2], 2);

	var dcGauge = new Gauge({
		renderTo    : 'dcGauge',
		width       : 350,
		height      : 350,
		gap         : 15,
		colors      : gaugeColors,
		dials       : [
			{
				id          : 'dcVoltageDial',
				title       : "DC Voltage",
				units       : 'Vdc',
				minValue    : rangeBatteryVoltage.min,
				maxValue    : rangeBatteryVoltage.max,
				startValue  : (config.batteryRangeLow[2] + intervalBatteryVoltageLow + config.batteryRangeHigh[0]) / 2,
				valueFormat : { "int" : 3, "dec" : 1 },
				majorTicks  : rangeBatteryVoltage.ticks,
				minorTicks  : 5,
				highlights  : batteryVoltageHighlights,
				drawArc     : true,
				animation : {
					duration : 1000
				}

			},
			{
				id          : 'energyConsumptionDial',
				title       : "SOC",
				units       : '%',
				minValue    : rangeEnergy.min,
				maxValue    : rangeEnergy.max,
				startValue  : rangeEnergy.max,
				ccw         : false,
				valueFormat : { "int" : 2, "dec" : 1 },
				majorTicks  : rangeEnergy.ticks,
				minorTicks  : 5,
				highlights  : [
					{ from : rangeEnergy.min, to : config.energyRange[1] - intervalEnergy, color : 'rgba(255, 0,  0, .75)' },
					{ from : config.energyRange[1] - intervalEnergy, to : config.energyRange[1], color : 'rgba(255, 220,  0, .75)' },
					{ from : config.energyRange[1], to : config.energyRange[1] + intervalEnergy, color : 'rgba(180, 255,  0, .75)' },
					{ from : config.energyRange[1] + intervalEnergy, to : rangeEnergy.max, color : 'rgba(0, 255,  0, .65)' }
		   		],
				drawArc     : true,
				animation : {
					duration : 1000
				}
			},
			{
				id          : 'dcCurrentDial',
				title       : "DC Current",
				units       : 'Amps',
				minValue    : rangeCurrent.min,
				maxValue    : rangeCurrent.max,
				ccw         : true,
				valueFormat : { "int" : 3, "dec" : 1 },
				majorTicks  : rangeCurrent.ticks,
				minorTicks  : 5,
				drawLimits  : true,
				highlights  : [
		   			{ from : rangeCurrent.min, to : config.currentRange[0] * 0.5, color : 'rgba(255, 255, 0, .75)' },
					{ from : config.currentRange[0] * 0.5, to : 0, color : 'rgba(0, 255, 0, .65)' },
					{ from : 0, to : config.currentRange[1] * 0.5, color : 'rgba(0, 180,  255, .75)' },
					{ from : config.currentRange[1] * 0.5, to : rangeCurrent.max, color : 'rgba(180, 120, 255, .75)' }
				],
				drawArc     : true,
				animation : {
					duration : 1000
				}
			}
		]
	});

	var rangeRpm = calcRange(config.rpmRange[0], config.rpmRange[1] + 1000,2);
	var rangeTorque = calcRange(config.torqueRange[0], config.torqueRange[1],2);
	var motorGauge = new Gauge({
		renderTo    : 'motorGauge',
		width       : 450,
		height      : 450,
		colors      : gaugeColors,
		dials       : [
/*			{
				id          : 'speedActualDial',
				title       : "RPM",
				units       : '',
				minValue    : rangeRpm.min,
				maxValue    : rangeRpm.max,
				valueFormat : { "int" : 4, "dec" : 0 },
				majorTicks  : rangeRpm.ticks,
				minorTicks  : 5,
				highlights  : [
					{ from : rangeRpm.min, to : config.rpmRange[1] - 2000, color : 'rgba(0, 255,  0, .65)' },
					{ from : config.rpmRange[1] - 2000, to : config.rpmRange[1] - 1000, color : 'rgba(255, 255, 0, .75)' },
					{ from : config.rpmRange[1] - 1000, to : config.rpmRange[1], color : 'rgba(255, 127, 0, .75)' },
					{ from : config.rpmRange[1], to : rangeRpm.max, color : 'rgba(255, 0, 0, .75)' }
				],
				drawArc     : true,
				animation : {
					duration : 100
				}
			},
*/
			{
				id          : 'throttleDial',
				title       : "Throttle",
				units       : '',
				minValue    : -100,
				maxValue    : 100,
				valueFormat : { "int" : 3, "dec" : 0 },
				majorTicks  : [ -100, -75, -50, -25, 0, 25, 50, 75, 100 ],
				minorTicks  : 5,
				highlights  : [
					{ from : -100, to : -50, color : 'rgba(0, 255,  0, .65)' },
					{ from : -50, to : 0, color : 'rgba(255, 255, 0, .75)' },
					{ from : 0, to : 50, color : 'rgba(255, 127, 0, .75)' },
					{ from : 50, to : 100, color : 'rgba(255, 0, 0, .75)' }
				],
				drawArc     : true,
				animation : {
					duration : 100
				}
			},
			{
				id          : 'torqueActualDial',
				title       : "Torque",
				units       : 'Nm',
				minValue    : rangeTorque.min,
				maxValue    : rangeTorque.max,
				ccw         : true,
				valueFormat : { "int" : 3, "dec" : 0 },
				majorTicks  : rangeTorque.ticks,
				minorTicks  : 5,
				highlights  : [
					{ from : rangeTorque.min, to : 0, color : 'rgba(0, 255, 0, .65)' },
					{ from : 0, to : rangeTorque.max, color : 'rgba(0, 180,  255, .75)' }
				],
				drawArc     : true,
				animation : {
					duration : 100
				}
			}
		],
	});

	var intervalMotor = (config.motorTempRange[2] - config.motorTempRange[1]) / 2;
	var rangeMotor = calcRange(config.motorTempRange[0], config.motorTempRange[2] + intervalMotor,2);
	var intervalController = (config.controllerTempRange[2] - config.controllerTempRange[1]) / 2;
	var rangeController = calcRange(config.controllerTempRange[0], config.controllerTempRange[2] + intervalController, 2);
	var temperatureGauge = new Gauge({
		renderTo    : 'temperatureGauge',
		width       : 350,
		height      : 350,
		colors      : gaugeColors,
		dials       : [
			{
				id          : 'temperatureMotorDial',
				title       : "Motor",
				units       : '\u2103',
				minValue    : rangeMotor.min,
				maxValue    : rangeMotor.max,
				valueFormat : { "int" : 3, "dec" : 1 },
				majorTicks  : rangeMotor.ticks,
				minorTicks  : 5,
				highlights  : [
					{ from : rangeMotor.min, to : config.motorTempRange[1] - intervalMotor, color : 'rgba(0, 255,  0, .65)' },
					{ from : config.motorTempRange[1] - intervalMotor, to : config.motorTempRange[1], color : 'rgba(180, 255, 0, .75)' },
					{ from : config.motorTempRange[1], to : config.motorTempRange[2] - intervalMotor, color : 'rgba(255, 220, 0, .75)' },
					{ from : config.motorTempRange[2] - intervalMotor, to : config.motorTempRange[2], color : 'rgba(255, 127, 0, .75)' },
					{ from : config.motorTempRange[2], to : rangeMotor.max, color : 'rgba(255, 0, 0, .75)' }
				],
				drawArc     : true,
				animation : {
					duration : 1000
				}
			},
			{
				id          : 'temperatureControllerDial',
				title       : "Controller",
				units       : '\u2103',
				minValue    : rangeController.min,
				maxValue    : rangeController.max,
				ccw         : true,
				valueFormat : { "int" : 3, "dec" : 1 },
				majorTicks  : rangeController.ticks,
				minorTicks  : 5,
				highlights  : [
		   			{ from : rangeController.min, to : config.controllerTempRange[1] - intervalController, color : 'rgba(0, 255,  0, .65)' },
					{ from : config.controllerTempRange[1] - intervalController, to : config.controllerTempRange[1], color : 'rgba(180, 255, 0, .75)' },
					{ from : config.controllerTempRange[1], to : config.controllerTempRange[2] - intervalController, color : 'rgba(255, 220, 0, .75)' },
					{ from : config.controllerTempRange[2] - intervalController, to : config.controllerTempRange[2], color : 'rgba(255, 127, 0, .75)' },
					{ from : config.controllerTempRange[2], to : rangeController.max, color : 'rgba(255, 0, 0, .75)' }
				],
				drawArc     : true,
				animation : {
					duration : 1000
				}
			}

			],
	});
}

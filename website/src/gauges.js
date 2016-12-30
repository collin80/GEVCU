/**
 * Gauge initialization
 *
 */

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
	var rangeThrottle = calcRange(-100,100,2.5);
	
	var j = 0;
	var ticksEnergy = new Array();
	var intervalEnergy = Math.round((config.energyRange[2] - config.energyRange[1]) / 2);
	for (i = config.energyRange[0] ; i <= (config.energyRange[2] + 5); i += 5) {
		ticksEnergy[j++] = i;
	} 
	var rangeEnergy = calcRange(config.energyRange[0], config.energyRange[2], 3);
	var rangeMechanicalPower = calcRange(config.powerRange[0], config.powerRange[1], 3);

	var powerGauge = new Gauge({
		renderTo    : 'powerGauge',
		width       : 280,
		height      : 280,
		gap         : 15,
		colors      : gaugeColors,
		values      : [
			{
				id          : 'throttleGaugeValue',
				title       : "Throttle",
				units       : '%',
				minValue    : rangeThrottle.min,
				maxValue    : rangeThrottle.max,
				valueFormat : { "int" : 3, "dec" : 1 },
				majorTicks  : rangeThrottle.ticks,
				minorTicks  : 2,
				highlights  : [
		   			{ from : rangeThrottle.min,   to : 0, color : 'rgba(0, 255, 0, .65)' },
					{ from : 0,   to : rangeThrottle.max, color : 'rgba(0, 180, 255, .75)' }
		   		]
			},
			{
				id          : 'energyConsumptionGaugeValue',
				title       : "Energy",
				units       : 'kWh',
				minValue    : rangeEnergy.min,
				maxValue    : rangeEnergy.max,
				ccw         : true,
				valueFormat : { "int" : 2, "dec" : 1 },
				majorTicks  : ticksEnergy,
				minorTicks  : 5,
				highlights  : [
					{ from : rangeEnergy.min, to : config.energyRange[1] - intervalEnergy, color : 'rgba(0, 255,  0, .65)' },
					{ from : config.energyRange[1] - intervalEnergy, to : config.energyRange[1], color : 'rgba(180, 255,  0, .75)' },
					{ from : config.energyRange[1], to : config.energyRange[2] - intervalEnergy, color : 'rgba(255, 220,  0, .75)' },
					{ from : config.energyRange[2] - intervalEnergy, to : config.energyRange[2], color : 'rgba(255, 127,  0, .75)' },
					{ from : config.energyRange[2], to : rangeEnergy.max, color : 'rgba(255, 0,  0, .75)' }
		   		]
			},
			{
				id          : 'mechanicalPowerGaugeValue',
				title       : "Power",
				units       : 'kW',
				minValue    : rangeMechanicalPower.min,
				maxValue    : rangeMechanicalPower.max,
				valueFormat : { "int" : 3, "dec" : 0 },
				majorTicks  : rangeMechanicalPower.ticks,
				minorTicks  : 5,
				highlights  : [
		  			{ from : rangeMechanicalPower.min, to : config.powerRange[0] * .9, color : 'rgba(255, 255, 0, .75)' },
					{ from : config.powerRange[0] *.9, to : 0, color : 'rgba(0, 255, 0, .65)' },
					{ from : 0, to : config.powerRange[1] * .9, color : 'rgba(0, 180, 255, .75)' },
					{ from : config.powerRange[1] * .9, to : rangeMechanicalPower.max, color : 'rgba(180, 180, 255, .75)' }
		   		]
			}
		],
	});

	var rangeRpm = calcRange(config.rpmRange[0], config.rpmRange[1] + 1000,2);
	var rangeTorque = calcRange(config.torqueRange[0], config.torqueRange[1],2);
	var motorGauge = new Gauge({
		renderTo    : 'motorGauge',
		width       : 350,
		height      : 350,
		colors      : gaugeColors,
		values      : [
			{
				id          : 'speedActualGaugeValue',
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
				]
			},
			{
				id          : 'torqueActualGaugeValue',
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
				]
			}
		],
	});

	var intervalMotor = (config.motorTempRange[2] - config.motorTempRange[1]) / 2;
	var rangeMotor = calcRange(config.motorTempRange[0], config.motorTempRange[2] + intervalMotor,2);
	var intervalController = (config.controllerTempRange[2] - config.controllerTempRange[1]) / 2;
	var rangeController = calcRange(config.controllerTempRange[0], config.controllerTempRange[2] + intervalController, 2);
	var temperatureGauge = new Gauge({
		renderTo    : 'temperatureGauge',
		width       : 200,
		height      : 200,
		colors      : gaugeColors,
		values      : [
			{
				id          : 'temperatureMotorGaugeValue',
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
				]
			},
			{
				id          : 'temperatureControllerGaugeValue',
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
				]
			}

		],
	});

	var intervalBatteryVoltageLow = Math.round((config.batteryRangeLow[2] - config.batteryRangeLow[1]) / 2);
	var intervalBatteryVoltageHigh = Math.round((config.batteryRangeHigh[2] - config.batteryRangeHigh[1]) / 2);
	var rangeBatteryVoltage = calcRange(config.batteryRangeLow[0] - intervalBatteryVoltageLow, config.batteryRangeHigh[2] + intervalBatteryVoltageHigh, 2);
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
	var rangeCurrent = calcRange(config.currentRange[0], config.currentRange[1], 2);
	var dcGauge = new Gauge({
		renderTo    : 'dcGauge',
		width       : 200,
		height      : 200,
		colors      : gaugeColors,
		values      : [
			{
				id          : 'dcVoltageGaugeValue',
				title       : "DC Voltage",
				units       : 'Vdc',
				minValue    : rangeBatteryVoltage.min,
				maxValue    : rangeBatteryVoltage.max,
				startValue  : (config.batteryRangeLow[2] + intervalBatteryVoltageLow + config.batteryRangeHigh[0]) / 2,
				valueFormat : { "int" : 3, "dec" : 1 },
				majorTicks  : rangeBatteryVoltage.ticks,
				minorTicks  : 5,
				highlights  : batteryVoltageHighlights
			},
			{
				id          : 'dcCurrentGaugeValue',
				title       : "DC Current",
				units       : 'Amps',
				minValue    : rangeCurrent.min,
				maxValue    : rangeCurrent.max,
				ccw         : true,
				valueFormat : { "int" : 3, "dec" : 1 },
				majorTicks  : rangeCurrent.ticks,
				minorTicks  : 5,
				highlights  : [
		   			{ from : rangeCurrent.min, to : config.currentRange[0] * .9, color : 'rgba(255, 255, 0, .75)' },
					{ from : config.currentRange[0] *.9, to : 0, color : 'rgba(0, 255, 0, .65)' },
					{ from : 0, to : config.currentRange[1] * .9, color : 'rgba(0, 180,  255, .75)' },
					{ from : config.currentRange[1] * .9, to : rangeCurrent.max, color : 'rgba(180, 180, 255, .75)' }
				]
			}
		],
	});


	
	var rangeChgACV = calcRange(config.chargerInputVoltageRange[0], config.chargerInputVoltageRange[2], 4);
	var rangeChgACA = calcRange(config.chargerInputCurrentRange[0], config.chargerInputCurrentRange[1], 4);
	var rangeChgDCA = calcRange(config.chargerBatteryCurrentRange[0], config.chargerBatteryCurrentRange[1], 4);
	var testGauge = new Gauge({
		renderTo    : 'testGauge',
		width       : 280,
		height      : 280,
		gap         : 10,
		drawHighlights: true,
		colors      : gaugeColors,
		values      : [
			{
				id          : 'chargerInputVoltageGaugeValue',
				title       : "Voltage In",
				units       : 'V',
				minValue    : rangeChgACV.min,
				maxValue    : rangeChgACV.max,
				startValue  : config.chargerInputVoltageRange[1],
				valueFormat : { "int" : 3, "dec" : 1 },
				majorTicks  : rangeChgACV.ticks,
				minorTicks  : 2,
				highlights  : [
				   	{ from : rangeChgACV.min, to : config.chargerInputVoltageRange[0], color : 'rgba(255, 0,  0, .75)' },
				   	{ from : config.chargerInputVoltageRange[0], to : config.chargerInputVoltageRange[1] - 5, color : 'rgba(255, 255,  0, .75)' },
					{ from : config.chargerInputVoltageRange[1] - 5, to : config.chargerInputVoltageRange[1] + 5, color : 'rgba(0, 255,  0, .65)' },
					{ from : config.chargerInputVoltageRange[1] + 5, to : config.chargerInputVoltageRange[2], color : 'rgba(255, 255,  0, .75)' },
					{ from : config.chargerInputVoltageRange[2], to : rangeChgACV.max, color : 'rgba(255, 0, 0, .75)' }
		   		]
			},
			{
				id          : 'chargerInputCurrentGaugeValue',
				title       : "Current In",
				units       : 'A',
				minValue    : rangeChgACA.min,
				maxValue    : rangeChgACA.max,
				valueFormat : { "int" : 2, "dec" : 1 },
				majorTicks  : rangeChgACA.ticks,
				minorTicks  : 5,
				highlights  : [
					{ from : 0, to : config.chargerInputCurrentRange[1] * .9, color : 'rgba(0, 255,  0, .65)' },
					{ from : config.chargerInputCurrentRange[1] * .9, to : rangeChgACA.max, color : 'rgba(255, 255, 0, .75)' }
		   		]
			},
			{
				id          : 'chargerBatteryVoltageGaugeValue',
				title       : "Voltage Out",
				units       : 'V',
				minValue    : rangeBatteryVoltage.min,
				maxValue    : rangeBatteryVoltage.max,
				startValue  : (config.batteryRangeLow[2] + intervalBatteryVoltageLow + config.batteryRangeHigh[0]) / 2,
				ccw         : true,
				valueFormat : { "int" : 3, "dec" : 1 },
				majorTicks  : rangeBatteryVoltage.ticks,
				minorTicks  : 5,
				highlights  : batteryVoltageHighlights
			},
			{
				id          : 'chargerBatteryCurrentGaugeValue',
				title       : "Current Out",
				units       : 'A',
				minValue    : rangeChgDCA.min,
				maxValue    : rangeChgDCA.max,
				valueFormat : { "int" : 2, "dec" : 1 },
				majorTicks  : rangeChgDCA.ticks,
				minorTicks  : 5,
				highlights  : [
					{ from : 0, to : config.chargerBatteryCurrentRange[1] * .9, color : 'rgba(0, 255,  0, .65)' },
					{ from : config.chargerBatteryCurrentRange[1] * .9, to : rangeChgDCA.max, color : 'rgba(255, 255, 0, .75)' }
		   		]
			}
		],
	});
/*

	interval = (config.chargerTempRange[2] - config.chargerTempRange[1]) / 2;
	range = calcRange(config.chargerTempRange[0], config.chargerTempRange[2] + interval);
	var chargerTemperatureGauge = new Gauge({
		renderTo    : 'chargerTemperatureGauge',
		width       : 200,
		height      : 200,
		units       : '\u2103',
		title       : "Temperature",
		colors      : gaugeColors,
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat : { "int" : 3, "dec" : 1 },
		highlights  : [
   			{ from : range.min, to : config.chargerTempRange[1] - interval, color : 'rgba(0, 255,  0, .65)' },
			{ from : config.chargerTempRange[1] - interval, to : config.chargerTempRange[1], color : 'rgba(180, 255, 0, .75)' },
			{ from : config.chargerTempRange[1], to : config.chargerTempRange[2] - interval, color : 'rgba(255, 220, 0, .75)' },
			{ from : config.chargerTempRange[2] - interval, to : config.chargerTempRange[2], color : 'rgba(255, 127, 0, .75)' },
			{ from : config.chargerTempRange[2], to : range.max, color : 'rgba(255, 0, 0, .75)' }
		]
	});

	var range = calcRange(config.dcDcLvVoltageRange[0], config.dcDcLvVoltageRange[1]);
	var dcDcLvVoltageGauge = new Gauge({
		renderTo    : 'dcDcLvVoltageGauge',
		width       : 150,
		height      : 150,
		units       : 'Vdc',
		title       : "Output",
		colors      : gaugeColors,
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat : { "int" : 2, "dec" : 1 },
		highlights  : [
			{ from : range.min, to : config.dcDcLvVoltageRange[1] * .9, color : 'rgba(0, 255,  0, .65)' },
			{ from : config.dcDcLvVoltageRange[1] * .9, to : range.max, color : 'rgba(255, 255, 0, .75)' }
		]
	});

	var range = calcRange(config.dcDcHvCurrentRange[0], config.dcDcHvCurrentRange[1]);
	var dcDcHvCurrentGauge = new Gauge({
		renderTo    : 'dcDcHvCurrentGauge',
		width       : 150,
		height      : 150,
		units       : 'Amps',
		title       : "Input",
		colors      : gaugeColors,
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat : { "int" : 2, "dec" : 1 },
		highlights  : [
			{ from : 0, to : config.dcDcHvCurrentRange[1] * .9, color : 'rgba(0, 255,  0, .65)' },
			{ from : config.dcDcHvCurrentRange[1] * .9, to : range.max, color : 'rgba(255, 255, 0, .75)' }
		]
	});

	var range = calcRange(config.dcDcLvCurrentRange[0], config.dcDcLvCurrentRange[1]);
	var dcDcLvCurrentGauge = new Gauge({
		renderTo    : 'dcDcLvCurrentGauge',
		width       : 150,
		height      : 150,
		units       : 'Amps',
		title       : "Output",
		colors      : gaugeColors,
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat : { "int" : 3, "dec" : 0 },
		highlights  : [
			{ from : 0, to : config.dcDcLvCurrentRange[1] * .9, color : 'rgba(0, 255,  0, .65)' },
			{ from : config.dcDcLvCurrentRange[1] * .9, to : range.max, color : 'rgba(255, 255, 0, .75)' }
		]
	});

	interval = (config.dcDcTempRange[2] - config.dcDcTempRange[1]) / 2;
	range = calcRange(config.dcDcTempRange[0], config.dcDcTempRange[2] + interval);
	var dcDcTemperatureGauge = new Gauge({
		renderTo    : 'dcDcTemperatureGauge',
		width       : 150,
		height      : 150,
		units       : '\u2103',
		title       : "Temperature",
		colors      : gaugeColors,
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat : { "int" : 3, "dec" : 1 },
		highlights  : [
   			{ from : range.min, to : config.dcDcTempRange[1] - interval, color : 'rgba(0, 255,  0, .65)' },
			{ from : config.dcDcTempRange[1] - interval, to : config.dcDcTempRange[1], color : 'rgba(180, 255, 0, .75)' },
			{ from : config.dcDcTempRange[1], to : config.dcDcTempRange[2] - interval, color : 'rgba(255, 220, 0, .75)' },
			{ from : config.dcDcTempRange[2] - interval, to : config.dcDcTempRange[2], color : 'rgba(255, 127, 0, .75)' },
			{ from : config.dcDcTempRange[2], to : range.max, color : 'rgba(255, 0, 0, .75)' }
		]
	});
	
	var dcDcHvVoltageGauge = new Gauge({
		renderTo    : 'dcDcHvVoltageGauge',
		width       : 150,
		height      : 150,
		units       : 'Vdc',
		title       : "Battery",
		colors      : gaugeColors,
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat : { "int" : 3, "dec" : 1 },
		highlights  : batteryHighlights
	});
*/
}

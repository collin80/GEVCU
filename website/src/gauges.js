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
	var rangeThrottle = calcRange(-100,100,1);
	var powerGauge = new Gauge({
		renderTo    : 'powerGauge',
		width       : 280,
		height      : 280,
		glow        : true,
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
				strokeTicks : false,
				highlights  : [
		   			{ from : -100,   to : 0, color : 'rgba(0, 255, 0, .65)' },
					{ from : 0,   to : 100, color : 'rgba(0, 180, 255, .75)' }
		   		]
			}
		],
	});
	powerGauge.draw();

	var rangeRpm = calcRange(config.rpmRange[0], config.rpmRange[1] + 1000,2);
	var rangeTorque = calcRange(config.torqueRange[0], config.torqueRange[1],2);
	var motorGauge = new Gauge({
		renderTo    : 'motorGauge',
		width       : 350,
		height      : 350,
		glow        : true,
		colors      : gaugeColors,
		values      : [
			{
				id          : 'speedActualGaugeValue',
				title       : "RPM",
				units       : '',
				minValue    : rangeRpm.min,
				maxValue    : rangeRpm.max,
				direction   : 'cw',
				valueFormat : { "int" : 4, "dec" : 0 },
				majorTicks  : rangeRpm.ticks,
				minorTicks  : 5,
				strokeTicks : false,
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
				direction   : 'ccw',
				valueFormat : { "int" : 3, "dec" : 1 },
				majorTicks  : rangeTorque.ticks,
				minorTicks  : 5,
				strokeTicks : false,
				highlights  : [
					{ from : rangeTorque.min, to : 0, color : 'rgba(0, 255, 0, .65)' },
					{ from : 0, to : rangeTorque.max, color : 'rgba(0, 180,  255, .75)' }
				]
			}
		],
	});
	motorGauge.draw();

	var intervalMotor = (config.motorTempRange[2] - config.motorTempRange[1]) / 2;
	var rangeMotor = calcRange(config.motorTempRange[0], config.motorTempRange[2] + intervalMotor,2);
	var intervalController = (config.controllerTempRange[2] - config.controllerTempRange[1]) / 2;
	var rangeController = calcRange(config.controllerTempRange[0], config.controllerTempRange[2] + intervalController, 2);
	var temperatureGauge = new Gauge({
		renderTo    : 'temperatureGauge',
		width       : 200,
		height      : 200,
		glow        : true,
		colors      : gaugeColors,
		values      : [
			{
				id          : 'temperatureMotorGaugeValue',
				title       : "Motor",
				units       : '\u2103',
				minValue    : rangeMotor.min,
				maxValue    : rangeMotor.max,
				direction   : 'cw',
				valueFormat : { "int" : 3, "dec" : 1 },
				majorTicks  : rangeMotor.ticks,
				minorTicks  : 5,
				strokeTicks : false,
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
				direction   : 'ccw',
				valueFormat : { "int" : 3, "dec" : 1 },
				majorTicks  : rangeController.ticks,
				minorTicks  : 5,
				strokeTicks : false,
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
	temperatureGauge.draw();

	var intervalLow = Math.round((config.batteryRangeLow[2] - config.batteryRangeLow[1]) / 2);
	var intervalHigh = Math.round((config.batteryRangeHigh[2] - config.batteryRangeHigh[1]) / 2);
	var rangeVoltage = calcRange(config.batteryRangeLow[0] - intervalLow, config.batteryRangeHigh[2] + intervalHigh, 2);
	var voltageHighlights = [
		{ from : rangeVoltage.min, to : config.batteryRangeLow[0], color : 'rgba(255, 0, 0, .75)' },
		{ from : config.batteryRangeLow[0], to : config.batteryRangeLow[1], color : 'rgba(255, 127, 0, .75)' },
		{ from : config.batteryRangeLow[1], to : config.batteryRangeLow[2], color : 'rgba(255, 220, 0, .75)' },
		{ from : config.batteryRangeLow[2], to : config.batteryRangeLow[2] + intervalLow, color : 'rgba(180, 255, 0, .75)' },
		{ from : config.batteryRangeLow[2] + intervalLow, to : config.batteryRangeHigh[0], color : 'rgba(0, 255,  0, .65)' },
		{ from : config.batteryRangeHigh[0], to : config.batteryRangeHigh[0] + intervalHigh, color : 'rgba(180, 255, 0, .75)' },
		{ from : config.batteryRangeHigh[0] + intervalHigh, to : config.batteryRangeHigh[1], color : 'rgba(255, 220, 0, .75)' },
		{ from : config.batteryRangeHigh[1], to : config.batteryRangeHigh[2], color : 'rgba(255, 127, 0, .75)' },
		{ from : config.batteryRangeHigh[2], to : rangeVoltage.max, color :  'rgba(255, 0, 0, .75)'}
	];
	var rangeCurrent = calcRange(config.currentRange[0], config.currentRange[1], 2);
	var dcGauge = new Gauge({
		renderTo    : 'dcGauge',
		width       : 200,
		height      : 200,
		glow        : true,
		colors      : gaugeColors,
		values      : [
			{
				id          : 'dcVoltageGaugeValue',
				title       : "DC Voltage",
				units       : 'Vdc',
				minValue    : rangeVoltage.min,
				maxValue    : rangeVoltage.max,
				direction   : 'cw',
				valueFormat : { "int" : 3, "dec" : 1 },
				majorTicks  : rangeVoltage.ticks,
				minorTicks  : 5,
				strokeTicks : false,
				highlights  : voltageHighlights
			},
			{
				id          : 'dcCurrentGaugeValue',
				title       : "DC Current",
				units       : 'Amps',
				minValue    : rangeCurrent.min,
				maxValue    : rangeCurrent.max,
				direction   : 'ccw',
				valueFormat : { "int" : 3, "dec" : 1 },
				majorTicks  : rangeCurrent.ticks,
				minorTicks  : 5,
				strokeTicks : false,
				highlights  : [
		   			{ from : rangeCurrent.min, to : config.currentRange[0] * .9, color : 'rgba(255, 255, 0, .75)' },
					{ from : config.currentRange[0] *.9, to : 0, color : 'rgba(0, 255, 0, .65)' },
					{ from : 0, to : config.currentRange[1] * .9, color : 'rgba(0, 180,  255, .75)' },
					{ from : config.currentRange[1] * .9, to : rangeCurrent.max, color : 'rgba(180, 180, 255, .75)' }
				]
			}
		],
	});
	dcGauge.draw();

/*
	j = 0;
	ticks = new Array();
	interval = Math.round((config.energyRange[2] - config.energyRange[1]) / 2);
	for (i = config.energyRange[0] ; i <= (config.energyRange[2] + 5); i += 5) {
		ticks[j++] = i;
	} 
	
	var range = calcRange(config.energyRange[0], config.energyRange[2]);
	var energyConsumptionGauge = new Gauge({
		renderTo    : 'energyConsumptionGauge',
		width       : 200,
		height      : 200,
		glow        : true,
		units       : 'kWh',
		title       : "Energy",
		colors      : gaugeColors,
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat : { "int" : 2, "dec" : 1 },
		highlights  : [
			{ from : range.min, to : config.energyRange[1] - interval, color : 'rgba(0, 255,  0, .65)' },
			{ from : config.energyRange[1] - interval, to : config.energyRange[1], color : 'rgba(180, 255,  0, .75)' },
			{ from : config.energyRange[1], to : config.energyRange[2] - interval, color : 'rgba(255, 220,  0, .75)' },
			{ from : config.energyRange[2] - interval, to : config.energyRange[2], color : 'rgba(255, 127,  0, .75)' },
			{ from : config.energyRange[2], to : range.max, color : 'rgba(255, 0,  0, .75)' }
		]
	});
	energyConsumptionGauge.draw();
	nodecache["energyConsumptionGauge"] = energyConsumptionGauge;

	var range = calcRange(config.powerRange[0], config.powerRange[1]);
	var mechanicalPowerGauge = new Gauge({
		renderTo    : 'mechanicalPowerGauge',
		width       : 200,
		height      : 200,
		glow        : true,
		units       : 'kW',
		title       : "Power",
		colors      : gaugeColors,
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat : { "int" : 3, "dec" : 1 },
		highlights  : [
  			{ from : range.min, to : config.powerRange[0] * .9, color : 'rgba(255, 255, 0, .75)' },
			{ from : config.powerRange[0] *.9, to : 0, color : 'rgba(0, 255, 0, .65)' },
			{ from : 0, to : config.powerRange[1] * .9, color : 'rgba(0, 180, 255, .75)' },
			{ from : config.powerRange[1] * .9, to : range.max, color : 'rgba(180, 180, 255, .75)' }
		]
	});
	mechanicalPowerGauge.draw();
	nodecache["mechanicalPowerGauge"] = mechanicalPowerGauge;
/*	
	var range = calcRange(config.chargerInputVoltageRange[0], config.chargerInputVoltageRange[2]);
	var chargerInputVoltageGauge = new Gauge({
		renderTo    : 'chargerInputVoltageGauge',
		width       : 200,
		height      : 200,
		glow        : true,
		units       : 'Vac',
		title       : "Mains",
		colors      : gaugeColors,
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat : { "int" : 2, "dec" : 1 },
		highlights  : [
		   	{ from : range.min, to : config.chargerInputVoltageRange[1] - 10, color : 'rgba(255, 255,  0, .75)' },
			{ from : config.chargerInputVoltageRange[1] - 10, to : config.chargerInputVoltageRange[1] + 10, color : 'rgba(0, 255,  0, .65)' },
			{ from : config.chargerInputVoltageRange[1] + 10, to : range.max, color : 'rgba(255, 255, 0, .75)' }
		]
	});
	chargerInputVoltageGauge.draw();
	nodecache["chargerInputVoltageGauge"] = chargerInputVoltageGauge;

	var range = calcRange(config.chargerInputCurrentRange[0], config.chargerInputCurrentRange[1]);
	var chargerInputCurrentGauge = new Gauge({
		renderTo    : 'chargerInputCurrentGauge',
		width       : 200,
		height      : 200,
		glow        : true,
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
			{ from : 0, to : config.chargerInputCurrentRange[1] * .9, color : 'rgba(0, 255,  0, .65)' },
			{ from : config.chargerInputCurrentRange[1] * .9, to : range.max, color : 'rgba(255, 255, 0, .75)' }
		]
	});
	chargerInputCurrentGauge.draw();
	nodecache["chargerInputCurrentGauge"] = chargerInputCurrentGauge;

	var range = calcRange(config.chargerBatteryCurrentRange[0], config.chargerBatteryCurrentRange[1]);
	var chargerBatteryCurrentGauge = new Gauge({
		renderTo    : 'chargerBatteryCurrentGauge',
		width       : 200,
		height      : 200,
		glow        : true,
		units       : 'Amps',
		title       : "Battery",
		colors      : gaugeColors,
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat : { "int" : 2, "dec" : 1 },
		highlights  : [
			{ from : 0, to : config.chargerBatteryCurrentRange[1] * .9, color : 'rgba(0, 255,  0, .65)' },
			{ from : config.chargerBatteryCurrentRange[1] * .9, to : range.max, color : 'rgba(255, 255, 0, .75)' }
		]
	});
	chargerBatteryCurrentGauge.draw();
	nodecache["chargerBatteryCurrentGauge"] = chargerBatteryCurrentGauge;

	interval = (config.chargerTempRange[2] - config.chargerTempRange[1]) / 2;
	range = calcRange(config.chargerTempRange[0], config.chargerTempRange[2] + interval);
	var chargerTemperatureGauge = new Gauge({
		renderTo    : 'chargerTemperatureGauge',
		width       : 200,
		height      : 200,
		glow        : true,
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
	chargerTemperatureGauge.draw();
	nodecache["chargerTemperatureGauge"] = chargerTemperatureGauge;

	var range = calcRange(config.dcDcLvVoltageRange[0], config.dcDcLvVoltageRange[1]);
	var dcDcLvVoltageGauge = new Gauge({
		renderTo    : 'dcDcLvVoltageGauge',
		width       : 150,
		height      : 150,
		glow        : true,
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
	dcDcLvVoltageGauge.draw();
	nodecache["dcDcLvVoltageGauge"] = dcDcLvVoltageGauge;

	var range = calcRange(config.dcDcHvCurrentRange[0], config.dcDcHvCurrentRange[1]);
	var dcDcHvCurrentGauge = new Gauge({
		renderTo    : 'dcDcHvCurrentGauge',
		width       : 150,
		height      : 150,
		glow        : true,
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
	dcDcHvCurrentGauge.draw();
	nodecache["dcDcHvCurrentGauge"] = dcDcHvCurrentGauge;

	var range = calcRange(config.dcDcLvCurrentRange[0], config.dcDcLvCurrentRange[1]);
	var dcDcLvCurrentGauge = new Gauge({
		renderTo    : 'dcDcLvCurrentGauge',
		width       : 150,
		height      : 150,
		glow        : true,
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
	dcDcLvCurrentGauge.draw();
	nodecache["dcDcLvCurrentGauge"] = dcDcLvCurrentGauge;

	interval = (config.dcDcTempRange[2] - config.dcDcTempRange[1]) / 2;
	range = calcRange(config.dcDcTempRange[0], config.dcDcTempRange[2] + interval);
	var dcDcTemperatureGauge = new Gauge({
		renderTo    : 'dcDcTemperatureGauge',
		width       : 150,
		height      : 150,
		glow        : true,
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
	dcDcTemperatureGauge.draw();
	nodecache["dcDcTemperatureGauge"] = dcDcTemperatureGauge;
	
	var chargerBatteryVoltageGauge = new Gauge({
		renderTo    : 'chargerBatteryVoltageGauge',
		width       : 200,
		height      : 200,
		glow        : true,
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
	chargerBatteryVoltageGauge.draw();
	nodecache["chargerBatteryVoltageGauge"] = chargerBatteryVoltageGauge;

	var dcDcHvVoltageGauge = new Gauge({
		renderTo    : 'dcDcHvVoltageGauge',
		width       : 150,
		height      : 150,
		glow        : true,
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
	dcDcHvVoltageGauge.draw();
	nodecache["dcDcHvVoltageGauge"] = dcDcHvVoltageGauge;
*/
}

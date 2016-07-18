/**
 * Gauge initialization
 *
 */

var gaugeColors = {
	plate      : '#222',
	majorTicks : '#f5f5f5',
	minorTicks : '#ddd',
	title      : '#fff',
	units      : '#ccc',
	numbers    : '#eee',
	needle     : { start : 'rgba(240, 128, 128, 1)', end : 'rgba(255, 160, 122, .9)' }
};

// calculate the min/max and ticks for a gauge
function calcRange(low, high) {
	var j = 0;
	var ticks = new Array();
	var step = Math.ceil((high - low) / 50) * 5;
	var min = ((low == 0) ? 0 : (low - (low + 1) % step - step + 1));
	var max = high - (high - 1) % step + step - 1;
	for (i = min; i <= max; i += step) {
		ticks[j++] = i;
	}
	var range = {"min": min, "max": max, "ticks": ticks};
	return range;
}

function generateGauges(config) {
	var range = calcRange(-100,100);	
	var throttleGauge = new Gauge({
		renderTo    : 'throttleGauge',
		width       : 230,
		height      : 230,
		glow        : true,
		units       : '%',
		title       : "Throttle",
		colors      : gaugeColors,
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 2,
		strokeTicks : false,
		valueFormat : { "int" : 3, "dec" : 1 },
		highlights  : [
		   			{ from : -100,   to : 0, color : 'rgba(0, 255, 0, .75)' },
					{ from : 0,   to : 100, color : 'rgba(0, 180, 255, .75)' }
		]
	});
	throttleGauge.draw();
	nodecache["throttleGauge"] = throttleGauge;

	range = calcRange(config.torqueRange[0], config.torqueRange[1]);
	var torqueActualGauge = new Gauge({
		renderTo    : 'torqueActualGauge',
		width       : 280,
		height      : 280,
		glow        : true,
		units       : 'Nm',
		title       : "Torque",
		colors      : gaugeColors,
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat : { "int" : 3, "dec" : 1 },
		highlights  : [
			{ from : range.min, to : 0, color : 'rgba(0, 255, 0, .75)' },
			{ from : 0, to : range.max, color : 'rgba(0, 180,  255, .75)' }
		]
	});
	torqueActualGauge.draw();
	nodecache["torqueActualGauge"] = torqueActualGauge;

	range = calcRange(config.rpmRange[0], config.rpmRange[1] + 1000);
	var speedActualGauge = new Gauge({
		renderTo    : 'speedActualGauge',
		width       : 280,
		height      : 280,
		glow        : true,
		units       : 'x1000',
		title       : "RPM",
		colors      : gaugeColors,
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat : { "int" : 4, "dec" : 0 },
		highlights  : [
			{ from : range.min, to : config.rpmRange[1] - 2000, color : 'rgba(0, 255,  0, .75)' },
			{ from : config.rpmRange[1] - 2000, to : config.rpmRange[1] - 1000, color : 'rgba(255, 255, 0, .75)' },
			{ from : config.rpmRange[1] - 1000, to : config.rpmRange[1], color : 'rgba(255, 127, 0, .75)' },
			{ from : config.rpmRange[1], to : range.max, color : 'rgba(255, 0, 0, .75)' }
		]
	});
	speedActualGauge.draw();
	nodecache["speedActualGauge"] = speedActualGauge;

	var interval = (config.motorTempRange[2] - config.motorTempRange[1]) / 2;
	range = calcRange(config.motorTempRange[0], config.motorTempRange[2] + interval);
	var temperatureMotorGauge = new Gauge({
		renderTo    : 'temperatureMotorGauge',
		width       : 200,
		height      : 200,
		glow        : true,
		units       : '\u2103',
		title       : "Motor",
		colors      : gaugeColors,
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat : { "int" : 3, "dec" : 1 },
		highlights  : [
			{ from : range.min, to : config.motorTempRange[1] - interval, color : 'rgba(0, 255,  0, .75)' },
			{ from : config.motorTempRange[1] - interval, to : config.motorTempRange[1], color : 'rgba(180, 255, 0, .75)' },
			{ from : config.motorTempRange[1], to : config.motorTempRange[2] - interval, color : 'rgba(255, 220, 0, .75)' },
			{ from : config.motorTempRange[2] - interval, to : config.motorTempRange[2], color : 'rgba(255, 127, 0, .75)' },
			{ from : config.motorTempRange[2], to : range.max, color : 'rgba(255, 0, 0, .75)' }
		]
	});
	temperatureMotorGauge.draw();
	nodecache["temperatureMotorGauge"] = temperatureMotorGauge;
	
	interval = (config.controllerTempRange[2] - config.controllerTempRange[1]) / 2;
	range = calcRange(config.controllerTempRange[0], config.controllerTempRange[2] + interval);
	var temperatureControllerGauge = new Gauge({
		renderTo    : 'temperatureControllerGauge',
		width       : 200,
		height      : 200,
		glow        : true,
		units       : '\u2103',
		title       : "Controller",
		colors      : gaugeColors,
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat : { "int" : 3, "dec" : 1 },
		highlights  : [
   			{ from : range.min, to : config.controllerTempRange[1] - interval, color : 'rgba(0, 255,  0, .75)' },
			{ from : config.controllerTempRange[1] - interval, to : config.controllerTempRange[1], color : 'rgba(180, 255, 0, .75)' },
			{ from : config.controllerTempRange[1], to : config.controllerTempRange[2] - interval, color : 'rgba(255, 220, 0, .75)' },
			{ from : config.controllerTempRange[2] - interval, to : config.controllerTempRange[2], color : 'rgba(255, 127, 0, .75)' },
			{ from : config.controllerTempRange[2], to : range.max, color : 'rgba(255, 0, 0, .75)' }
		]
	});
	temperatureControllerGauge.draw();
	nodecache["temperatureControllerGauge"] = temperatureControllerGauge;

	var intervalLow = Math.round((config.batteryRangeLow[2] - config.batteryRangeLow[1]) / 2);
	var intervalHigh = Math.round((config.batteryRangeHigh[2] - config.batteryRangeHigh[1]) / 2);
	range = calcRange(config.batteryRangeLow[0] - intervalLow, config.batteryRangeHigh[2] + intervalHigh);
	var batteryHighlights = [
	             			{ from : range.min, to : config.batteryRangeLow[0], color : 'rgba(255, 0, 0, .75)' },
	            			{ from : config.batteryRangeLow[0], to : config.batteryRangeLow[1], color : 'rgba(255, 127, 0, .75)' },
	            			{ from : config.batteryRangeLow[1], to : config.batteryRangeLow[2], color : 'rgba(255, 220, 0, .75)' },
	            			{ from : config.batteryRangeLow[2], to : config.batteryRangeLow[2] + intervalLow, color : 'rgba(180, 255, 0, .75)' },
	            			{ from : config.batteryRangeLow[2] + intervalLow, to : config.batteryRangeHigh[0], color : 'rgba(0, 255,  0, .75)' },
	            			{ from : config.batteryRangeHigh[0], to : config.batteryRangeHigh[0] + intervalHigh, color : 'rgba(180, 255, 0, .75)' },
	            			{ from : config.batteryRangeHigh[0] + intervalHigh, to : config.batteryRangeHigh[1], color : 'rgba(255, 220, 0, .75)' },
	            			{ from : config.batteryRangeHigh[1], to : config.batteryRangeHigh[2], color : 'rgba(255, 127, 0, .75)' },
	            			{ from : config.batteryRangeHigh[2], to : range.max, color :  'rgba(255, 0, 0, .75)'}
	            		];
	var dcVoltageGauge = new Gauge({
		renderTo    : 'dcVoltageGauge',
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
	dcVoltageGauge.draw();
	nodecache["dcVoltageGauge"] = dcVoltageGauge;

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

	var range = calcRange(config.currentRange[0], config.currentRange[1]);
	var dcCurrentGauge = new Gauge({
		renderTo    : 'dcCurrentGauge',
		width       : 230,
		height      : 230,
		glow        : true,
		units       : 'Amps',
		title       : "DC Current",
		colors      : gaugeColors,
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat : { "int" : 3, "dec" : 1 },
		highlights  : [
   			{ from : range.min, to : config.currentRange[0] * .9, color : 'rgba(255, 255, 0, .75)' },
			{ from : config.currentRange[0] *.9, to : 0, color : 'rgba(0, 255, 0, .75)' },
			{ from : 0, to : config.currentRange[1] * .9, color : 'rgba(0, 180,  255, .75)' },
			{ from : config.currentRange[1] * .9, to : range.max, color : 'rgba(180, 180, 255, .75)' }
		]
	});
	dcCurrentGauge.draw();
	nodecache["dcCurrentGauge"] = dcCurrentGauge;

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
			{ from : range.min, to : config.energyRange[1] - interval, color : 'rgba(0, 255,  0, .75)' },
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
			{ from : config.powerRange[0] *.9, to : 0, color : 'rgba(0, 255, 0, .75)' },
			{ from : 0, to : config.powerRange[1] * .9, color : 'rgba(0, 180, 255, .75)' },
			{ from : config.powerRange[1] * .9, to : range.max, color : 'rgba(180, 180, 255, .75)' }
		]
	});
	mechanicalPowerGauge.draw();
	nodecache["mechanicalPowerGauge"] = mechanicalPowerGauge;
	
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
			{ from : config.chargerInputVoltageRange[1] - 10, to : config.chargerInputVoltageRange[1] + 10, color : 'rgba(0, 255,  0, .75)' },
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
			{ from : 0, to : config.chargerInputCurrentRange[1] * .9, color : 'rgba(0, 255,  0, .75)' },
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
			{ from : 0, to : config.chargerBatteryCurrentRange[1] * .9, color : 'rgba(0, 255,  0, .75)' },
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
   			{ from : range.min, to : config.chargerTempRange[1] - interval, color : 'rgba(0, 255,  0, .75)' },
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
			{ from : range.min, to : config.dcDcLvVoltageRange[1] * .9, color : 'rgba(0, 255,  0, .75)' },
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
			{ from : 0, to : config.dcDcHvCurrentRange[1] * .9, color : 'rgba(0, 255,  0, .75)' },
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
			{ from : 0, to : config.dcDcLvCurrentRange[1] * .9, color : 'rgba(0, 255,  0, .75)' },
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
   			{ from : range.min, to : config.dcDcTempRange[1] - interval, color : 'rgba(0, 255,  0, .75)' },
			{ from : config.dcDcTempRange[1] - interval, to : config.dcDcTempRange[1], color : 'rgba(180, 255, 0, .75)' },
			{ from : config.dcDcTempRange[1], to : config.dcDcTempRange[2] - interval, color : 'rgba(255, 220, 0, .75)' },
			{ from : config.dcDcTempRange[2] - interval, to : config.dcDcTempRange[2], color : 'rgba(255, 127, 0, .75)' },
			{ from : config.dcDcTempRange[2], to : range.max, color : 'rgba(255, 0, 0, .75)' }
		]
	});
	dcDcTemperatureGauge.draw();
	nodecache["dcDcTemperatureGauge"] = dcDcTemperatureGauge;
}
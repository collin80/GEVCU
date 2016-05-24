/**
 * Gauge initialization
 *
 */

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
		width       : 250,
		height      : 250,
		glow        : true,
		units       : '%',
		title       : "Throttle",
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 2,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 1 },
		highlights  : [
		   			{ from : -100,   to : 0, color : 'rgba(0, 180, 255, .75)' },
					{ from : 0,   to : 100, color : 'rgba(0, 255, 0, .75)' }
		],
		colors      : {
			plate      : '#222',
			majorTicks : '#f5f5f5',
			minorTicks : '#ddd',
			title      : '#fff',
			units      : '#ccc',
			numbers    : '#eee',
			needle     : { start : 'rgba(240, 128, 128, 1)', end : 'rgba(255, 160, 122, .9)' }
		}
	});
	throttleGauge.draw();
	nodecache["throttleGauge"] = throttleGauge;

	range = calcRange(config.torqueRange[0], config.torqueRange[1]);
	var torqueActualGauge = new Gauge({
		renderTo    : 'torqueActualGauge',
		width       : 300,
		height      : 300,
		glow        : true,
		units       : 'Nm',
		title       : "Torque",
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 1 },
		highlights  : [
			{ from : range.min, to : 0, color : 'rgba(0, 180, 255, .75)' },
			{ from : 0, to : range.max, color : 'rgba(0, 255,  0, .75)' }
		],
		colors      : {
			plate      : '#222',
			majorTicks : '#f5f5f5',
			minorTicks : '#ddd',
			title      : '#fff',
			units      : '#ccc',
			numbers    : '#eee',
			needle     : { start : 'rgba(240, 128, 128, 1)', end : 'rgba(255, 160, 122, .9)' }
		}
	});
	torqueActualGauge.draw();
	nodecache["torqueActualGauge"] = torqueActualGauge;

	range = calcRange(config.rpmRange[0], config.rpmRange[1] + 1000);
	var speedActualGauge = new Gauge({
		renderTo    : 'speedActualGauge',
		width       : 300,
		height      : 300,
		glow        : true,
		units       : 'x1000',
		title       : "RPM",
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat      : { "int" : 4, "dec" : 0 },
		highlights  : [
			{ from : range.min, to : config.rpmRange[1] - 2000, color : 'rgba(0, 255,  0, .75)' },
			{ from : config.rpmRange[1] - 2000, to : config.rpmRange[1] - 1000, color : 'rgba(255, 255, 0, .75)' },
			{ from : config.rpmRange[1] - 1000, to : config.rpmRange[1], color : 'rgba(255, 127, 0, .75)' },
			{ from : config.rpmRange[1], to : range.max, color : 'rgba(255, 0, 0, .75)' }
		],
		colors      : {
			plate      : '#222',
			majorTicks : '#f5f5f5',
			minorTicks : '#ddd',
			title      : '#fff',
			units      : '#ccc',
			numbers    : '#eee',
			needle     : { start : 'rgba(240, 128, 128, 1)', end : 'rgba(255, 160, 122, .9)' }
		}
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
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 1 },
		highlights  : [
			{ from : range.min, to : config.motorTempRange[1] - interval, color : 'rgba(0, 255,  0, .75)' },
			{ from : config.motorTempRange[1] - interval, to : config.motorTempRange[1], color : 'rgba(180, 255, 0, .75)' },
			{ from : config.motorTempRange[1], to : config.motorTempRange[2] - interval, color : 'rgba(255, 220, 0, .75)' },
			{ from : config.motorTempRange[2] - interval, to : config.motorTempRange[2], color : 'rgba(255, 127, 0, .75)' },
			{ from : config.motorTempRange[2], to : range.max, color : 'rgba(255, 0, 0, .75)' }
		],
		colors      : {
			plate      : '#222',
			majorTicks : '#f5f5f5',
			minorTicks : '#ddd',
			title      : '#fff',
			units      : '#ccc',
			numbers    : '#eee',
			needle     : { start : 'rgba(240, 128, 128, 1)', end : 'rgba(255, 160, 122, .9)' }
		}
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
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 1 },
		highlights  : [
   			{ from : range.min, to : config.controllerTempRange[1] - interval, color : 'rgba(0, 255,  0, .75)' },
			{ from : config.controllerTempRange[1] - interval, to : config.controllerTempRange[1], color : 'rgba(180, 255, 0, .75)' },
			{ from : config.controllerTempRange[1], to : config.controllerTempRange[2] - interval, color : 'rgba(255, 220, 0, .75)' },
			{ from : config.controllerTempRange[2] - interval, to : config.controllerTempRange[2], color : 'rgba(255, 127, 0, .75)' },
			{ from : config.controllerTempRange[2], to : range.max, color : 'rgba(255, 0, 0, .75)' }
		],
		colors      : {
			plate      : '#222',
			majorTicks : '#f5f5f5',
			minorTicks : '#ddd',
			title      : '#fff',
			units      : '#ccc',
			numbers    : '#eee',
			needle     : { start : 'rgba(240, 128, 128, 1)', end : 'rgba(255, 160, 122, .9)' }
		}
	});
	temperatureControllerGauge.draw();
	nodecache["temperatureControllerGauge"] = temperatureControllerGauge;

	var intervalLow = Math.round((config.batteryRangeLow[2] - config.batteryRangeLow[1]) / 2);
	var intervalHigh = Math.round((config.batteryRangeHigh[2] - config.batteryRangeHigh[1]) / 2);
	range = calcRange(config.batteryRangeLow[0] - intervalLow, config.batteryRangeHigh[2] + intervalHigh);
	var dcVoltageGauge = new Gauge({
		renderTo    : 'dcVoltageGauge',
		width       : 200,
		height      : 200,
		glow        : true,
		units       : 'Vdc',
		title       : "Battery",
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 1 },
		highlights  : [
			{ from : range.min, to : config.batteryRangeLow[0], color : 'rgba(255, 0, 0, .75)' },
			{ from : config.batteryRangeLow[0], to : config.batteryRangeLow[1], color : 'rgba(255, 127, 0, .75)' },
			{ from : config.batteryRangeLow[1], to : config.batteryRangeLow[2], color : 'rgba(255, 220, 0, .75)' },
			{ from : config.batteryRangeLow[2], to : config.batteryRangeLow[2] + intervalLow, color : 'rgba(180, 255, 0, .75)' },
			{ from : config.batteryRangeLow[2] + intervalLow, to : config.batteryRangeHigh[0], color : 'rgba(0, 255,  0, .75)' },
			{ from : config.batteryRangeHigh[0], to : config.batteryRangeHigh[0] + intervalHigh, color : 'rgba(180, 255, 0, .75)' },
			{ from : config.batteryRangeHigh[0] + intervalHigh, to : config.batteryRangeHigh[1], color : 'rgba(255, 220, 0, .75)' },
			{ from : config.batteryRangeHigh[1], to : config.batteryRangeHigh[2], color : 'rgba(255, 127, 0, .75)' },
			{ from : config.batteryRangeHigh[2], to : range.max, color :  'rgba(255, 0, 0, .75)'}
		],
		colors      : {
			plate      : '#222',
			majorTicks : '#f5f5f5',
			minorTicks : '#ddd',
			title      : '#fff',
			units      : '#ccc',
			numbers    : '#eee',
			needle     : { start : 'rgba(240, 128, 128, 1)', end : 'rgba(255, 160, 122, .9)' }
		}
	});
	dcVoltageGauge.draw();
	nodecache["dcVoltageGauge"] = dcVoltageGauge;

	var range = calcRange(config.currentRange[0], config.currentRange[1]);
	var dcCurrentGauge = new Gauge({
		renderTo    : 'dcCurrentGauge',
		width       : 250,
		height      : 250,
		glow        : true,
		units       : 'Amps',
		title       : "DC Current",
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 1 },
		highlights  : [
   			{ from : range.min, to : config.currentRange[0] * .9, color : 'rgba(180, 180, 255, .75)' },
			{ from : config.currentRange[0] *.9, to : 0, color : 'rgba(0, 180, 255, .75)' },
			{ from : 0, to : config.currentRange[1] * .9, color : 'rgba(0, 255,  0, .75)' },
			{ from : config.currentRange[1] * .9, to : range.max, color : 'rgba(255, 255, 0, .75)' }
		],
		colors      : {
			plate      : '#222',
			majorTicks : '#f5f5f5',
			minorTicks : '#ddd',
			title      : '#fff',
			units      : '#ccc',
			numbers    : '#eee',
			needle     : { start : 'rgba(240, 128, 128, 1)', end : 'rgba(255, 160, 122, .9)' }
		}
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
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat      : { "int" : 2, "dec" : 1 },
		highlights  : [
			{ from : range.min, to : config.energyRange[1] - interval, color : 'rgba(0, 255,  0, .75)' },
			{ from : config.energyRange[1] - interval, to : config.energyRange[1], color : 'rgba(180, 255,  0, .75)' },
			{ from : config.energyRange[1], to : config.energyRange[2] - interval, color : 'rgba(255, 220,  0, .75)' },
			{ from : config.energyRange[2] - interval, to : config.energyRange[2], color : 'rgba(255, 127,  0, .75)' },
			{ from : config.energyRange[2], to : range.max, color : 'rgba(255, 0,  0, .75)' }
		],
		colors      : {
			plate      : '#222',
			majorTicks : '#f5f5f5',
			minorTicks : '#ddd',
			title      : '#fff',
			units      : '#ccc',
			numbers    : '#eee',
			needle     : { start : 'rgba(240, 128, 128, 1)', end : 'rgba(255, 160, 122, .9)' }
		}
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
		minValue    : range.min,
		maxValue    : range.max,
		majorTicks  : range.ticks,
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat : { "int" : 3, "dec" : 1 },
		highlights  : [
  			{ from : range.min, to : config.powerRange[0] * .9, color : 'rgba(180, 180, 255, .75)' },
			{ from : config.powerRange[0] *.9, to : 0, color : 'rgba(0, 180, 255, .75)' },
			{ from : 0, to : config.powerRange[1] * .9, color : 'rgba(0, 255,  0, .75)' },
			{ from : config.powerRange[1] * .9, to : range.max, color : 'rgba(255, 255, 0, .75)' }
		],
		colors      : {
			plate      : '#222',
			majorTicks : '#f5f5f5',
			minorTicks : '#ddd',
			title      : '#fff',
			units      : '#ccc',
			numbers    : '#eee',
			needle     : { start : 'rgba(240, 128, 128, 1)', end : 'rgba(255, 160, 122, .9)' }
		}
	});
	mechanicalPowerGauge.draw();
	nodecache["mechanicalPowerGauge"] = mechanicalPowerGauge;
}
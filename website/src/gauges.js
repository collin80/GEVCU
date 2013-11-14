/**
 * Gauge initialization
 *
 */

function updateThrottleGaugeHighlights(throttleRegenMin, throttleFwd) {

	Gauge.Collection.get('throttleGauge').updateConfig({
		highlights  : [
			{ from : 0,   to : throttleRegenMin, color : 'rgba(255, 0, 0, .75)' },
			{ from : throttleRegenMin, to : throttleFwd, color : 'rgba(255, 255, 0, .75)' },
			{ from : throttleFwd, to : 100, color : 'rgba(0, 255,  0, .75)' }
		]
	});
}

function refreshGaugeValue(setting, value) {
	var id = setting + 'Gauge';
	var gauge = document.getElementById(id);
	if ( gauge ) {
		if ( !value ) {
			var div = document.getElementById(setting);
			if ( div ) {
				value = div.innerHTML;
			}
		}
		if ( value ) {
			Gauge.Collection.get(id).setValue(value);
		}
	}
}

function generateGauges() {
	var throttleGauge = new Gauge({
		renderTo    : 'throttleGauge',
		width       : 300,
		height      : 300,
		glow        : true,
		units       : 'Percent',
		title       : "Throttle",
		minValue    : 0,
		maxValue    : 100,
		majorTicks  : ['0','10','20','30','40','50','60','70','80','90','100'],
		minorTicks  : 2,
		strokeTicks : false,
	
		highlights  : [
			{ from : 0,   to : 15, color : 'rgba(255, 0, 0, .75)' },
			{ from : 15, to : 30, color : 'rgba(255, 255, 0, .75)' },
			{ from : 30, to : 100, color : 'rgba(0, 255,  0, .75)' }
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

	// testing
	/*
	throttleGauge.onready = function() {
		setInterval( function() {
			//throttleGauge.setValue( Math.random() * 100);
			Gauge.Collection.get('throttleGauge').setValue( Math.random() * 100);
		}, 1000);
	};
	*/
	throttleGauge.draw();

	var torqueGauge = new Gauge({
		renderTo    : 'torqueActualGauge',
		width       : 250,
		height      : 250,
		glow        : true,
		units       : 'Nm',
		title       : "Torque",
		minValue    : -400,
		maxValue    : 400,
		majorTicks  : ['-400','-300','-200','-100','0','100','200','300','400'],
		minorTicks  : 2,
		strokeTicks : false,
	
		highlights  : [
			{ from : -400,   to : 0, color : 'rgba(255, 0, 0, .75)' },
			{ from : 0, to : 400, color : 'rgba(0, 255,  0, .75)' }
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

	// testing
	/*
	torqueGauge.onready = function() {
		setInterval( function() {
			torqueGauge.setValue( Math.random() * 400);
		}, 1000);
	};
	*/
	
	torqueGauge.draw();

	var rpmGauge = new Gauge({
		renderTo    : 'speedActualGauge',
		width       : 250,
		height      : 250,
		glow        : true,
		units       : 'x1000',
		title       : "RPM",
		minValue    : 0,
		maxValue    : 10000,
		majorTicks  : ['0','1','2','3','4','5','6','7','8','9','10'],
		minorTicks  : 2,
		strokeTicks : false,
		valueFormat      : { "int" : 4, "dec" : 0 },
		highlights  : [
			{ from : 5000, to : 7000, color : 'rgba(255, 255, 0, .75)' },
			{ from : 7000, to : 10000, color : 'rgba(255, 0, 0, .75)' }
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

	// testing
	/*
	rpmGauge.onready = function() {
		setInterval( function() {
			rpmGauge.setValue( Math.random() * 10000);
		}, 1000);
	};
	*/
	
	rpmGauge.draw();

	var temperatureMotorGauge = new Gauge({
		renderTo    : 'temperatureMotorGauge',
		width       : 175,
		height      : 175,
		glow        : true,
		units       : 'degrees F',
		title       : "Motor Temp",
		minValue    : 0,
		maxValue    : 250,
		majorTicks  : ['0','25','50','75','100','125','150','175','200','225','250'],
		minorTicks  : 2,
		strokeTicks : false,
		valueFormat      : { "int" : 4, "dec" : 0 },
		highlights  : [
			{ from : 0,   to : 180, color : 'rgba(0, 255,  0, .75)' },
			{ from : 180, to : 220, color : 'rgba(255, 255, 0, .75)' },
			{ from : 220, to : 250, color : 'rgba(255, 0, 0, .75)' }
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

	// testing
	/*
	temperatureMotorGauge.onready = function() {
		setInterval( function() {
			temperatureMotorGauge.setValue( Math.random() * 10000);
		}, 1000);
	};
	*/

	temperatureMotorGauge.draw();
	
	var temperatureInverterGauge = new Gauge({
		renderTo    : 'temperatureInverterGauge',
		width       : 175,
		height      : 175,
		glow        : true,
		units       : 'degrees F',
		title       : "Inverter Temp",
		minValue    : 0,
		maxValue    : 250,
		majorTicks  : ['0','25','50','75','100','125','150','175','200','225','250'],
		minorTicks  : 2,
		strokeTicks : false,
		valueFormat      : { "int" : 4, "dec" : 0 },
		highlights  : [
			{ from : 0,   to : 180, color : 'rgba(0, 255,  0, .75)' },
			{ from : 180, to : 220, color : 'rgba(255, 255, 0, .75)' },
			{ from : 220, to : 250, color : 'rgba(255, 0, 0, .75)' }
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

	// testing
	/*
	temperatureInverterGauge.onready = function() {
		setInterval( function() {
			temperatureInverterGauge.setValue( Math.random() * 10000);
		}, 1000);
	};
	*/

	temperatureInverterGauge.draw();
	
	var dcVoltageGauge = new Gauge({
		renderTo    : 'dcVoltageGauge',
		width       : 175,
		height      : 175,
		glow        : true,
		units       : 'Volts',
		title       : "DC Voltage",
		minValue    : 0,
		maxValue    : 450,
		majorTicks  : ['0','50','100','150','200','250','300','350','400','450'],
		minorTicks  : 2,
		strokeTicks : false,
		valueFormat      : { "int" : 4, "dec" : 0 },
		highlights  : [
			{ from : 0,   to : 280, color : 'rgba(255, 0, 0, .75)' },
			{ from : 280, to : 300, color : 'rgba(255, 255, 0, .75)' },
			{ from : 300, to : 450, color : 'rgba(0, 255,  0, .75)' }
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

	// testing
	/*
	dcVoltageGauge.onready = function() {
		setInterval( function() {
			dcVoltageGauge.setValue( Math.random() * 10000);
		}, 1000);
	};
	*/

	dcVoltageGauge.draw();
	
	var dcCurrentGauge = new Gauge({
		renderTo    : 'dcCurrentGauge',
		width       : 175,
		height      : 175,
		glow        : true,
		units       : 'Amps',
		title       : "DC Current",
		minValue    : 0,
		maxValue    : 450,
		majorTicks  : ['0','50','100','150','200','250','300','350','400','450'],
		minorTicks  : 2,
		strokeTicks : false,
		valueFormat      : { "int" : 4, "dec" : 0 },
		highlights  : [
			{ from : 0,   to : 280, color : 'rgba(0, 255,  0, .75)' },
			{ from : 280, to : 300, color : 'rgba(255, 255, 0, .75)' },
			{ from : 300, to : 450, color : 'rgba(255, 0, 0, .75)' }
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

	// testing
	/*
	dcCurrentGauge.onready = function() {
		setInterval( function() {
			dcCurrentGauge.setValue( Math.random() * 10000);
		}, 1000);
	};
	*/

	dcCurrentGauge.draw();
	
	var acCurrentGauge = new Gauge({
		renderTo    : 'acCurrentGauge',
		width       : 175,
		height      : 175,
		glow        : true,
		units       : 'Amps',
		title       : "AC Current",
		minValue    : 0,
		maxValue    : 450,
		majorTicks  : ['0','50','100','150','200','250','300','350','400','450'],
		minorTicks  : 2,
		strokeTicks : false,
		valueFormat      : { "int" : 4, "dec" : 0 },
		highlights  : [
			{ from : 0,   to : 280, color : 'rgba(0, 255,  0, .75)' },
			{ from : 280, to : 300, color : 'rgba(255, 255, 0, .75)' },
			{ from : 300, to : 450, color : 'rgba(255, 0, 0, .75)' }
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

	// testing
	/*
	acCurrentGauge.onready = function() {
		setInterval( function() {
			acCurrentGauge.setValue( Math.random() * 10000);
		}, 1000);
	};
	*/

	acCurrentGauge.draw();
	
	var mechanicalPowerGauge = new Gauge({
		renderTo    : 'mechanicalPowerGauge',
		width       : 175,
		height      : 175,
		glow        : true,
		units       : 'Kw',
		title       : "Power",
		minValue    : 0,
		maxValue    : 450,
		majorTicks  : ['0','25','50','75','100','125','150'],
		minorTicks  : 2,
		strokeTicks : false,
		valueFormat      : { "int" : 4, "dec" : 0 },
		highlights  : [
			{ from : 0, to : 450, color : 'rgba(0, 255,  0, .75)' }
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

	// testing
	/*
	mechanicalPowerGauge.onready = function() {
		setInterval( function() {
			mechanicalPowerGauge.setValue( Math.random() * 10000);
		}, 1000);
	};
	*/

	mechanicalPowerGauge.draw();
}
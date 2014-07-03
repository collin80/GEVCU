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
		width       : 200,
		height      : 200,
		glow        : true,
		units       : 'Percent',
		title       : "Throttle",
		minValue    : 0,
		maxValue    : 100,
		majorTicks  : ['0','10','20','30','40','50','60','70','80','90','100'],
		minorTicks  : 2,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 1 },

	
		highlights  : [
				{ from : 0, to : 100, color : 'rgba(0, 255,  0, .75)' }
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
		width       : 300,
		height      : 300,
		glow        : true,
		units       : 'Nm',
		title       : "Torque",
		minValue    : -100,
		maxValue    : 300,
		majorTicks  : ['-100','-50','0','50','100','150','200','250','300'],
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 0 },

	
		highlights  : [
			{ from : -100,   to : 0, color : 'rgba(255, 0, 0, .75)' },
			{ from : 0, to : 300, color : 'rgba(0, 255,  0, .75)' }
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
		width       : 200,
		height      : 200,
		glow        : true,
		units       : 'x1000',
		title       : "RPM",
		minValue    : 0,
		maxValue    : 10000,
		majorTicks  : ['0','1','2','3','4','5','6','7','8','9','10'],
		minorTicks  : 2,
		strokeTicks : false,
		valueFormat      : { "int" : 5, "dec" : 0 },
		highlights  : [
			{ from : 0, to : 6000, color : 'rgba(0, 255,  0, .75)' },
			{ from : 6000, to : 7000, color : 'rgba(255, 255, 0, .75)' },
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
		width       : 200,
		height      : 200,
		glow        : true,
		units       : 'degrees C',
		title       : "Motor Temp",
		minValue    : 0,
		maxValue    : 250,
		majorTicks  : ['0','25','50','75','100','125','150','175','200','225','250'],
		minorTicks  : 2,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 0 },
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
		width       : 200,
		height      : 200,
		glow        : true,
		units       : 'degrees C',
		title       : "Inverter Temp",
		minValue    : 0,
		maxValue    : 120,
		majorTicks  : ['0','10','20','30','40','50','60','70','80','90','100','110','120'],
		minorTicks  : 2,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 0 },
		highlights  : [
			{ from : 0,   to : 70, color : 'rgba(0, 255,  0, .75)' },
			{ from : 70, to : 80, color : 'rgba(255, 255, 0, .75)' },
			{ from : 80, to : 120, color : 'rgba(255, 0, 0, .75)' }
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
		width       : 200,
		height      : 200,
		glow        : true,
		units       : 'Volts',
		title       : "DC Voltage",
		minValue    : 0,
		maxValue    : 450,
		majorTicks  : ['0','50','100','150','200','250','300','350','400','450'],
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 0 },
		highlights  : [
			{ from : 0,   to : 220, color : 'rgba(255, 255, 0, .75)' },
			{ from : 220, to : 400, color : 'rgba(0, 255,  0, .75)' },
			{ from : 400, to : 450, color :  'rgba(255, 0, 0, .75)'}
		
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
		width       : 300,
		height      : 300,
		glow        : true,
		units       : 'Amps',
		title       : "DC Current",
		minValue    : -100,
		maxValue    : 450,
		majorTicks  : ['-100','-50','0','50','100','150','200','250','300','350','400','450'],
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 0 },
		highlights  : [
		{ from : -100, to : 0, color : 'rgba(255, 0, 0, .75)' },
			{ from : 0,   to :350, color : 'rgba(0, 255,  0, .75)' },
			{ from : 350, to : 400, color : 'rgba(255, 255, 0, .75)' },
			{ from : 400, to : 450, color : 'rgba(255, 0, 0, .75)' }
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
	
	var kiloWattHoursGauge = new Gauge({
		renderTo    : 'kiloWattHoursGauge',
		width       : 200,
		height      : 200,
		glow        : true,
		units       : 'kWh',
		title       : "kiloWatt Hours",
		minValue    : 0,
		maxValue    : 30,
		majorTicks  : ['0','5','10','15','20','25','30'],
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat      : { "int" : 2, "dec" : 1 },
		highlights  : [
			{ from : 0,   to : 30, color : 'rgba(0, 255,  0, .75)' }
			
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
	kiloWattHoursGauge.onready = function() {
		setInterval( function() {
			kiloWattHoursGauge.setValue( Math.random() * 10000);
		}, 1000);
	};
	*/

	kiloWattHoursGauge.draw();
	
	var mechanicalPowerGauge = new Gauge({
		renderTo    : 'mechanicalPowerGauge',
		width       : 200,
		height      : 200,
		glow        : true,
		units       : 'Kw',
		title       : "Power",
		minValue    : 0,
		maxValue    : 150,
		majorTicks  : ['0','25','50','75','100','125','150'],
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 0 },
		highlights  : [
			{ from : 0, to : 150, color : 'rgba(0, 255,  0, .75)' }
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
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
function updateTemperatureInverterGaugeHighlights(coolOff, coolOn) {

	Gauge.Collection.get('temperatureInverterGauge').updateConfig({
		highlights  : [
			{ from : 0,   to : coolOff, color : 'rgba(255, 0, 0, .75)' },
			{ from : coolOff, to : coolOn, color : 'rgba(255, 255, 0, .75)' },
			{ from : coolOn, to : 120, color : 'rgba(0, 255,  0, .75)' }
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
		width       : 250,
		height      : 250,
		glow        : true,
		units       : 'Percent',
		title       : "Throttle",
		minValue    : -60,
		maxValue    : 100,
		majorTicks  : ['-60','-40','-20','0','20','40','60','80','100'],
		minorTicks  : 2,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 1 },

	
		highlights  : [
			{ from : -60,   to : 0, color : 'rgba(255, 0, 0, .75)' },
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
		minValue    : -150,
		maxValue    : 300,
		majorTicks  : ['-150','-100','-50','0','50','100','150','200','250','300'],
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 0 },

	
		highlights  : [
			{ from : -150,   to : 0, color : 'rgba(255, 0, 0, .75)' },
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
		units       : 'C',
		title       : "Motor",
		minValue    : 0,
		maxValue    : 200,
		majorTicks  : ['0','25','50','75','100','125','150','175','200'],
		minorTicks  : 2,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 0 },
		highlights  : [
			{ from : 0,   to : 150, color : 'rgba(0, 255,  0, .75)' },
			{ from : 150, to : 175, color : 'rgba(255, 255, 0, .75)' },
			{ from : 175, to : 200, color : 'rgba(255, 0, 0, .75)' }
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
		units       : 'C',
		title       : "Inverter",
		minValue    : 0,
		maxValue    : 70,
		majorTicks  : ['0','10','20','30','40','50','60','70'],
		minorTicks  : 2,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 0 },
		highlights  : [
			{ from : 0,   to : 40, color : 'rgba(0, 255,  0, .75)' },
			{ from : 40, to : 70, color : 'rgba(255, 0, 0, .75)' }

			
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
		units       : 'Vdc',
		title       : "Battery",
		minValue    : 100,
		maxValue    : 450,
		majorTicks  : ['100','150','200','250','300','350','400','450'],
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 0 },
		highlights  : [
			{ from : 100,   to : 200, color : 'rgba(255, 255, 0, .75)' },
			{ from : 200, to : 400, color : 'rgba(0, 255,  0, .75)' },
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
		minValue    : -150,
		maxValue    : 400,
		majorTicks  : ['-150','-100','-50','0','50','100','150','200','250','300','350','400'],
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 0 },
		highlights  : [
		{ from : -150, to : 0, color : 'rgba(255, 0, 0, .75)' },
			{ from : 0,   to :350, color : 'rgba(0, 255,  0, .75)' },
			{ from : 350, to : 400, color : 'rgba(255, 255, 0, .75)' }
			
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
		title       : "Energy",
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
		units       : 'kW',
		title       : "Power",
		minValue    : -25,
		maxValue    : 150,
		majorTicks  : ['-25','0','25','50','75','100','125','150'],
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 0 },
		highlights  : [
			{ from : -25, to : 0, color : 'rgba(255, 0, 0, .75)' },
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
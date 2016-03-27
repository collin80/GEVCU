/**
 * Gauge initialization
 *
 */

function updateThrottleGaugeHighlights(positionRegenMinimum, positionForwardStart) {

	Gauge.Collection.get('throttleGauge').updateConfig({
		highlights  : [
			{ from : 0,   to : positionRegenMinimum, color : 'rgba(255, 0, 0, .75)' },
			{ from : positionRegenMinimum, to : positionForwardStart, color : 'rgba(255, 255, 0, .75)' },
			{ from : positionForwardStart, to : 100, color : 'rgba(0, 255,  0, .75)' }
		]
	});
}
function updateTemperatureControllerGaugeHighlights(coolingTempOff, coolingTempOn) {

	Gauge.Collection.get('temperatureControllerGauge').updateConfig({
		highlights  : [
			{ from : 0,   to : coolingTempOff, color : 'rgba(255, 0, 0, .75)' },
			{ from : coolingTempOff, to : coolingTempOn, color : 'rgba(255, 255, 0, .75)' },
			{ from : coolingTempOn, to : 120, color : 'rgba(0, 255,  0, .75)' }
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
		units       : '%',
		title       : "Throttle",
		minValue    : -100,
		maxValue    : 100,
		majorTicks  : ['-100','-80','-60','-40','-20','0','20','40','60','80','100'],
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

	var torqueGauge = new Gauge({
		renderTo    : 'torqueActualGauge',
		width       : 300,
		height      : 300,
		glow        : true,
		units       : 'Nm',
		title       : "Torque",
		minValue    : -200,
		maxValue    : 300,
		majorTicks  : ['-200','-150','-100','-50','0','50','100','150','200','250','300'],
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 1 },

	
		highlights  : [
			{ from : -200,   to : 0, color : 'rgba(0, 180, 255, .75)' },
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
	torqueGauge.draw();

	var rpmGauge = new Gauge({
		renderTo    : 'speedActualGauge',
		width       : 300,
		height      : 300,
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
			{ from : 0, to : 6000, color : 'rgba(0, 255,  0, .75)' },
			{ from : 6000, to : 7000, color : 'rgba(180, 255, 0, .75)' },
			{ from : 7000, to : 8000, color : 'rgba(255, 255, 0, .75)' },
			{ from : 8000, to : 9000, color : 'rgba(255, 127, 0, .75)' },
			{ from : 9000, to : 10000, color : 'rgba(255, 0, 0, .75)' }
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
	rpmGauge.draw();

	var temperatureMotorGauge = new Gauge({
		renderTo    : 'temperatureMotorGauge',
		width       : 200,
		height      : 200,
		glow        : true,
		units       : 'C',
		title       : "Motor",
		minValue    : 0,
		maxValue    : 160,
		majorTicks  : ['0','20','40','60','80','100','120','140','160'],
		minorTicks  : 2,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 1 },
		highlights  : [
			{ from : 0,   to : 80, color : 'rgba(0, 255,  0, .75)' },
			{ from : 80,  to : 100, color : 'rgba(180, 255, 0, .75)' },
			{ from : 100, to : 120, color : 'rgba(255, 255, 0, .75)' },
			{ from : 120, to : 140, color : 'rgba(255, 127, 0, .75)' },
			{ from : 140, to : 160, color : 'rgba(255, 0, 0, .75)' }
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
	
	var temperatureControllerGauge = new Gauge({
		renderTo    : 'temperatureControllerGauge',
		width       : 200,
		height      : 200,
		glow        : true,
		units       : 'C',
		title       : "Controller",
		minValue    : 0,
		maxValue    : 100,
		majorTicks  : ['0','10','20','30','40','50','60','70','80','90','100'],
		minorTicks  : 2,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 1 },
		highlights  : [
			{ from : 0,   to : 60, color : 'rgba(0, 255,  0, .75)' },
			{ from : 60, to : 70, color : 'rgba(180, 255, 0, .75)' },
			{ from : 70, to : 80, color : 'rgba(255, 255, 0, .75)' },
			{ from : 80, to : 90, color : 'rgba(255, 127, 0, .75)' },
			{ from : 90, to : 100, color : 'rgba(255, 0, 0, .75)' }
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
	
	var dcVoltageGauge = new Gauge({
		renderTo    : 'dcVoltageGauge',
		width       : 200,
		height      : 200,
		glow        : true,
		units       : 'Vdc',
		title       : "Battery",
		minValue    : 275,
		maxValue    : 450,
		majorTicks  : ['275','300','325','350','375','400','425','450'],
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 1 },
		highlights  : [
			{ from : 275,   to : 300, color : 'rgba(255, 0, 0, .75)' },
			{ from : 300,   to : 320, color : 'rgba(255, 127, 0, .75)' },
			{ from : 320, to : 350, color : 'rgba(255, 255, 0, .75)' },
			{ from : 350, to : 370, color : 'rgba(180, 255, 0, .75)' },
			{ from : 370, to : 410, color : 'rgba(0, 255,  0, .75)' },
			{ from : 410, to : 415, color : 'rgba(180, 255, 0, .75)' },
			{ from : 415, to : 420, color : 'rgba(255, 255, 0, .75)' },
			{ from : 420, to : 430, color : 'rgba(255, 127, 0, .75)' },
			{ from : 430, to : 450, color :  'rgba(255, 0, 0, .75)'}
		
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
	
	var dcCurrentGauge = new Gauge({
		renderTo    : 'dcCurrentGauge',
		width       : 250,
		height      : 250,
		glow        : true,
		units       : 'Amps',
		title       : "DC Current",
		minValue    : -200,
		maxValue    : 400,
		majorTicks  : ['-200','-150','-100','-50','0','50','100','150','200','250','300','350','400'],
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat      : { "int" : 3, "dec" : 1 },
		highlights  : [
		{ from : -200, to : 0, color : 'rgba(0, 180, 255, .75)' },
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
	dcCurrentGauge.draw();
	
	var kiloWattHoursGauge = new Gauge({
		renderTo    : 'kiloWattHoursGauge',
		width       : 200,
		height      : 200,
		glow        : true,
		units       : 'kWh',
		title       : "Energy",
		minValue    : 0,
		maxValue    : 40,
		majorTicks  : ['0','5','10','15','20','25','30','35','40'],
		minorTicks  : 5,
		strokeTicks : false,
		valueFormat      : { "int" : 2, "dec" : 1 },
		highlights  : [
			{ from : 0,   to : 40, color : 'rgba(0, 255,  0, .75)' }
			
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
		valueFormat      : { "int" : 3, "dec" : 1 },
		highlights  : [
			{ from : -25, to : 0, color : 'rgba(0, 180, 255, .75)' },
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
	mechanicalPowerGauge.draw();
}
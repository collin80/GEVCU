/**
 * Gauge initialization
 *
 */

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
		width       : 300,
		height      : 300,
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
		renderTo    : 'speedRequestedGauge',
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

}
onmessage = function(event) {
	handleMessage(event.data);
}

var 
	config,
	toValue,
	animateInterval
;

function handleMessage(data) {
	if(data.config) {
		processConfig(data.config);
		return;
	}

    postMessage({id : config.id, text: data});

    var value = constrain(data, config.minValue, config.maxValue);
	var from = config.animation ? toValue : value;
	toValue = value;

	config.animation && (!config.animation.threshold || Math.abs(from - value) > config.animation.threshold) ? animate(from, value) : sendUpdate(value);
}

function radians(degrees) {
	return degrees * Math.PI / 180;
}

function constrain(val, low, high) {
	return (val < low ? low : (val > high ? high : val));
}

function sendUpdate(value) {
	var 
		cfg = config,  // reduce global searches
		angle, arcStart, arcEnd
	;
	
	if (value < 0) {
		value = Math.abs(cfg.minValue - value);
	} else if (cfg.minValue > 0) {
		value -= cfg.minValue;
	} else {
		value = Math.abs(cfg.minValue) + value;
	}

   angle = cfg.offset + (cfg.ccw ? cfg.range - value : value) * cfg.angle / cfg.range;

	 if (cfg.ccw) {
		arcStart = angle;
		arcEnd = cfg.offset + (cfg.minValue < cfg.startValue ? cfg.range - cfg.startValue + cfg.minValue : cfg.range) * cfg.angle / cfg.range;
	} else {
		arcStart = cfg.offset + (cfg.minValue < cfg.startValue ? cfg.startValue - cfg.minValue : 0) * cfg.angle / cfg.range;
		arcEnd = angle;
	}

	if (cfg.minValue < cfg.startValue && (value + cfg.minValue) < cfg.startValue) {
		var temp = arcStart;
		arcStart = arcEnd;
		arcEnd = temp;
	}
   
    postMessage({id : cfg.id, arcStart : radians(90 + arcStart), arcEnd: radians(90 + arcEnd), angle: radians(angle)});
}

function _animate(opts) {
	var progress = (Date.now() - opts.start) / opts.duration;
	if (progress > 1) {
		progress = 1;
	}

	sendUpdate(opts.from + opts.range * opts.fn(progress));

    if (progress == 1) {
		clearInterval(animateInterval);
	}
}

function animate(from, to) {
	animateInterval && clearInterval(animateInterval); // stop previous animation
	animateInterval = setInterval(_animate, config.animation.delay, {
		start: Date.now(),
		from: from,
		range: to - from,
		fn: config.animation.fn,
		duration: config.animation.duration
	});
}

var animateFx = {
		linear : function(p) { return p; },
		quad   : function(p) { return Math.pow(p, 2); },
		quint  : function(p) { return Math.pow(p, 5); },
		cycle  : function(p) { return 1 - Math.sin(Math.acos(p)); },
		bounce : function(p) {
			return 1 - (function(p) {
				for(var a = 0, b = 1; 1; a += b, b /= 2) {
					if (p >= (7 - 4 * a) / 11) {
						return -Math.pow((11 - 6 * a - 11 * p) / 4, 2) + Math.pow(b, 2);
					}
				}
			})(1 - p);
		},
		elastic : function(p) {
			return 1 - (function(p) {
				var x = 1.5;
				return Math.pow(2, 10 * (p - 1)) * Math.cos(20 * Math.PI * x / 3 * p);
			})(1 - p);
		}
	};

function processConfig(config) {
	this.config = config;
	toValue = this.config.minValue;
	if (this.config.animation && this.config.animation.fn) {
		this.config.animation.fn = animateFx[this.config.animation.fn];
	}
}
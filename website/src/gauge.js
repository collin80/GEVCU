/**!
 * @license
 * Multi-Dial Gauge by Michael Neuweiler <michael_neuweiler@yahoo.com>
 * 
 * This code is subject to MIT license and is based on
 * HTML5 Canvas Gauge Copyright (c) 2012 Mykhailo Stadnyk <mikhus@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
 * Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * @authors: Mykhailo Stadnyk <mikhus@gmail.com>
 *           Chris Poile <poile@edwards.usask.ca>
 *           Luca Invernizzi <http://www.lucainvernizzi.net>
 *           Robert Blackburn <http://www.rwblackburn.com>
 *           Michael Neuweiler  <michael_neuweiler@yahoo.com>
 */

/**
 * @param {Object}
 *            config
 * @constructor
 */
var Gauge = function(config) {
	/**
	 * Default gauge configuration
	 */
	this.config = {
		renderTo    : null,
		width       : 200,
		height      : 200,
		start       : 15,
		angle       : 330,
		gap         : 30,
		glow        : true,
		colors      : {
			plate      : '#fff',
			majorTicks : '#444',
			minorTicks : '#666',
			title      : '#888',
			units      : '#888',
			numbers    : '#444',
			needle     : { start : 'rgba(240, 128, 128, 1)', end : 'rgba(255, 160, 122, .9)' }
		},
		values      : [
			{
				id          : false,
				title       : false,
				units       : false,
				minValue    : 0,
				maxValue    : 100,
				direction   : 'cw',
				majorTicks       : [],
				minorTicks       : 10,
				strokeTicks      : true,
				valueFormat      : { "int" : 3, "dec" : 2 },
		        majorTicksFormat : { "int" : 1, "dec" : 0 },
				highlights  : [
					{ from : 20, to : 60, color : '#eee' },
					{ from : 60, to : 80, color : '#ccc' },
					{ from : 80, to : 100, color : '#999' }]
			}
		],
		animation : {
			delay    : 0,
			duration : 200,
			fn       : 'cycle'
		}
	}

	var
		value     = [],
		values    = [],
		self      = this,
		fromValue = [],
		toValue   = [],
		imready   = false
	;

	/**
	 * ********* Public methods - to be called from the application **********
	 */
	
	/**
	 * Sets a new value to gauge and updates the gauge view
	 * 
	 * @param {number}
	 *            id - the number of the dial
	 * @param {number}
	 *            val - the new value to set to the gauge
	 * @return {Gauge} this - returns self
	 */
	this.setValue = function(id, val) {
		value[id] = val;
		val = constrain (val, values[id].minValue, values[id].maxValue);
		fromValue[id] = config.animation ? toValue[id] : val;
		toValue[id] = val;

		drawValue(ctxInfoValues, id);
		config.animation ? animate(id) : this.draw(id);

		return this;
	}
	
	/**
	 * Sets a new value to gauge and updates the gauge view without any
	 * animation (even if configured)
	 * 
	 * @param {number}
	 *            id - the number of the dial
	 * @param {number}
	 *            val - the new value to set to the gauge
	 * @return {Gauge} this - returns self
	 */
	this.setRawValue = function(id, val) {
		value[id] = val;
		fromValue[id] = constrain (val, values[id].minValue, values[id].maxValue);

		drawValue(ctxInfoValues, id);
		this.draw(id);

		return this;
	}

	/**
	 * Clears the value of the gauge
	 * 
	 * @param {number}
	 *            id - the number of the dial
	 * @return {Gauge}
	 */
	this.clear = function(id) {
		value[id] = fromValue[id] = toValue[id] = values[id].minValue;
		drawValue(ctxInfoValues, id);
		this.draw(id);
		return this;
	}

	/**
	 * Returns the current value been set to the gauge
	 * 
	 * @param {number}
	 *            id - the number of the dial
	 * @return {number} value - current gauge's value
	 */
	this.getValue = function(id) {
		return value[id];
	}

	/**
	 * Updates the gauge config
	 * 
	 * @param {Object}
	 *            config
	 * @return {Gauge}
	 */
	this.updateConfig = function(config) {
        applyRecursive(this.config, config);
        baseInit();
        this.draw();
        return this;
    }

	/**
	 * Ready event for the gauge. Use it whenever you initialize the gauge to be
	 * assured it was fully drawn before you start the update on it
	 * 
	 * @event {Function} onready
	 */
	this.onready = function() {}
	
	/**
	 * ******* Private methods - to be used by the gauges internally ********
	 */
		
	/**
	 * Recursively copy the values of one object to another.
	 * 
	 * @param dst destination object
	 * @param src source object
	 * @returns
	 */
	function applyRecursive(dst, src) {
		for (var i in src) {
			if (typeof src[i] == "object" && !(Object.prototype.toString.call(src[i]) === '[object Array]') && i != 'renderTo') {
				if (typeof dst[i] != "object") {
					dst[i] = {};
				}
				applyRecursive(dst[i], src[i]);
			} else {
				dst[i] = src[i];
			}
		}
	}

	applyRecursive(this.config, config);
	config = this.config;

	if (!config.renderTo) {
		throw Error("Canvas element was not specified when creating the Gauge object!");
	}

	var ctxDial = [], ctxInfo, ctxInfoValues,
		CW  = config.width,
		CH  = config.height,
		CX  = CW / 2,
		CY  = CH / 2,
		max = CX < CY ? CX : CY
	;
	
	function baseInit() {
		// important. first add all canvas before accessing one !!
		addCanvas(config.renderTo + "Plate");
		for (var i = 0; i < config.values.length; i++) {
			addCanvas(config.values[i].id);
		}
		addCanvas(config.renderTo + "Info");
		addCanvas(config.renderTo + "InfoValues");

		// now it's safe to look them up
		var ctxPlate = prepareCanvas(config.renderTo + "Plate", false);
		for (var i = 0; i < config.values.length; i++) {
			var id = config.values[i].id;
			ctxDial[id] = prepareCanvas(id, true);
			fromValue[id] = value[id] = toValue[id] = config.values[i].minValue;
			config.values[i].offset = config.start + (config.angle + config.gap) * i / config.values.length;
			config.values[i].angle = (config.angle - config.gap * (config.values.length - 1)) / config.values.length;
			values[config.values[i].id] = config.values[i];
		}
		ctxInfo = prepareCanvas(config.renderTo + "Info", true);
		ctxInfoValues = prepareCanvas(config.renderTo + "InfoValues", true);

		drawPlate(ctxPlate);
		drawHighlights(ctxPlate);
		drawMinorTicks(ctxPlate);
		drawMajorTicks(ctxPlate);
		drawNumbers(ctxPlate);
		
		drawInfoArea(ctxInfo);
		drawTitle(ctxInfo);
		drawUnits(ctxInfo);
	//	drawValueBox(ctxInfo);
	}

	// do basic initialization
	baseInit();
	
	/**
	 * Add a canvas with a specified id to the 'renderTo' container.
	 * 
	 * @param id
	 *            id for the canvas
	 */
	function addCanvas(id) {
		var container = config.renderTo.tagName ? config.renderTo : document.getElementById(config.renderTo);
		container.innerHTML += "<canvas id='" + id + "'></canvas>";
		Gauge.Collection[id] = self;
	}

	/**
	 * Resize, shift and translate a canvas / context.
	 * 
	 * @param id
	 *            for the canvas id
	 * @param relocate
	 *            if the canvas should by shifted to the left and upwards by its
	 *            size
	 * @returns the context of the canvas
	 */
	function prepareCanvas(id, relocate) {
		var canvas = document.getElementById(id);
		canvas.width  = config.width;
		canvas.height = config.height;
		
		if (relocate) {
			canvas.style.marginLeft -= config.width;
			canvas.style.marginTop -= config.height;
		}
		var context = canvas.getContext('2d');
		context.translate(CX, CY);// translate canvas to have 0,0 in center
		context.save();
		return context;
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
	}

	var animateInterval = [];

	function _animate(opts) {
		var start = new Date; 

		animateInterval[opts.id] = setInterval(function() {
			var
				timePassed = new Date - start,
				progress = timePassed / opts.duration;

			if (progress > 1) {
				progress = 1;
			}

			var animateFn = typeof opts.delta == "function" ? opts.delta : animateFx[opts.delta];
			var delta = animateFn(progress);
			opts.step(delta);

			if (progress == 1) {
				clearInterval(animateInterval[opts.id]);
			}
		}, opts.delay || 10);
	}

	function animate(id) {
		// stop previous animation
		animateInterval[id] && clearInterval(animateInterval[id]); 
		var
			path = (toValue[id] - fromValue[id]),
			from = fromValue[id],
			cfg  = config.animation
		;

		_animate({
			delay    : cfg.delay,
			duration : cfg.duration,
			delta    : cfg.fn,
			id       : id,
			step     : function(delta) { fromValue[id] = parseFloat(from) + path * delta; self.draw(id); }
		});
	}

	/**
	 * Draws the variable parts of the gauge.
	 * 
	 * @return {Gauge} this - returns the self Gauge object
	 */
	this.draw = function(id) {
		// draw all values if no id is passed
		if(typeof id == "undefined") {
			for (var i = 0; i < ctxDial.length; i++) {
				this.draw(ctxDial[i].id);
			}
			return;
		}

		drawNeedle(id);
		drawArc(id);

		if (!imready) {
			self.onready && self.onready();
			imready = true;
		}
		return this;
	}

	/**
	 * Transforms degrees to radians
	 */
	function radians(degrees) {
		return degrees * Math.PI / 180;
	}

	/**
	 * Linear gradient
	 */
	function lgrad(ctx, clrFrom, clrTo, len) {
		var grad = ctx.createLinearGradient(0, 0, 0, len);  
		grad.addColorStop(0, clrFrom);  
		grad.addColorStop(1, clrTo);
		return grad;
	}

	function drawPlate(ctx) {
		var
			r0 = max / 100 * 97,
			d0 = max -r0,
			r1 = max / 100 * 96,
			d1 = max - r1,
			r2 = max / 100 * 95,
			d2 = max - r2,
			r3 = max / 100 * 94;

		ctx.save();

		if (config.glow) {
			ctx.shadowBlur  = d0;
			ctx.shadowColor = 'rgba(0, 0, 0, 0.5)';
		}

		ctx.beginPath();
		ctx.arc(0, 0, r0, 0, Math.PI * 2, true);
		ctx.fillStyle = lgrad(ctx, '#ddd', '#aaa', r0);
		ctx.fill();

		ctx.restore();

		ctx.beginPath();
		ctx.arc(0, 0, r1, 0, Math.PI * 2, true);
		ctx.fillStyle = lgrad(ctx, '#fafafa', '#ccc', r1);
		ctx.fill();

		ctx.beginPath();
		ctx.arc(0, 0, r2, 0, Math.PI * 2, true);
		ctx.fillStyle = lgrad(ctx, '#eee', '#f0f0f0', r2);
		ctx.fill();
	
		ctx.beginPath();
		ctx.arc(0, 0, r3, 0, Math.PI * 2, true);

		var grd = ctx.createRadialGradient(0, 0, 50, 0, 0, r3);
		grd.addColorStop(0, config.colors.plate.start);
		grd.addColorStop(1, config.colors.plate.end);
		ctx.fillStyle = grd;
		ctx.fill();

		ctx.save();
	}

    /**
	 * Formats a number for display on the dial's plate using the
	 * majorTicksFormat config option.
	 * 
	 * @param {number}
	 *            num The number to format
	 * @returns {string} The formatted number
	 */
    function formatMajorTickNumber(num) {
        var r, isDec = false;

        // First, force the correct number of digits right of the decimal.
        if(config.majorTicksFormat.dec === 0) {
            r = Math.round(num).toString();
        } else {
            r = num.toFixed(config.majorTicksFormat.dec);
        }

        // Second, force the correct number of digits left of the decimal.
        if(config.majorTicksFormat["int"] > 1) {
            // Does this number have a decimal?
            isDec = (r.indexOf('.') > -1);

            // Is this number a negative number?
            if(r.indexOf('-') > -1) {
                return '-' + [
					config.majorTicksFormat["int"] + config.majorTicksFormat.dec + 2 + (isDec ? 1 : 0) - r.length
				].join('0') + r.replace('-', '');
            } else {
                return [
                	config.majorTicksFormat["int"] + config.majorTicksFormat.dec + 1 + (isDec ? 1 : 0) - r.length
				].join('0') + r;
            }
        } else {
            return r;
        }
    }

	// major ticks draw
	function drawMajorTicks(ctx) {
		var r = max / 100 * 88;

		ctx.lineWidth = 2;
		ctx.strokeStyle = config.colors.majorTicks;
		ctx.save();

		for (var v = 0; v < config.values.length; v++) {
	        if(config.values[v].majorTicks.length == 0) {
	            var numberOfDefaultTicks = 5;
	            var tickSize = (config.values[v].maxValue - config.values[v].minValue)/numberOfDefaultTicks;
	
	            for(var i = 0; i < numberOfDefaultTicks; i++) {
	                config.values[v].majorTicks.push(formatMajorTickNumber(config.values[v].minValue+(tickSize*i)));
	            }
	            config.values[v].majorTicks.push(formatMajorTickNumber(config.values[v].maxValue));
	        }
	
			var range = config.values[v].majorTicks.length - 1;
			for (var i = 0; i < config.values[v].majorTicks.length; ++i) {
				ctx.rotate(radians(config.values[v].offset + i * config.values[v].angle / range));

				ctx.beginPath();
				ctx.moveTo(0, r);
				ctx.lineTo(0, r - max / 100 * 15);
				ctx.stroke();

				ctx.restore();
				ctx.save();
			}
	
			if (config.values[v].strokeTicks) {
				ctx.rotate(radians(90));

				ctx.beginPath();
				ctx.arc(0, 0, r, radians(config.start), radians(config.start + config.angle), false);
				ctx.stroke();
				ctx.restore();
				ctx.save();
			}
		}
	}

	// minor ticks draw
	function drawMinorTicks(ctx) {
		var r = max / 100 * 88;

		ctx.lineWidth = 1;
		ctx.strokeStyle = config.colors.minorTicks;

		ctx.save();

		for (var v = 0; v < config.values.length; v++) {
			var range = config.values[v].minorTicks * (config.values[v].majorTicks.length - 1);	
			for (var i = 0; i < range; ++i) {
				ctx.rotate(radians(config.values[v].offset + i * config.values[v].angle / range));
	
				ctx.beginPath();
				ctx.moveTo(0, r);
				ctx.lineTo(0, r - max / 100 * 7.5);
				ctx.stroke();
	
				ctx.restore();
				ctx.save();
			}
		}
	}

	// tick numbers draw
	function drawNumbers(ctx) {
		var r = max / 100 * 60;

		for (var v = 0; v < config.values.length; v++) {
			var range = config.values[v].majorTicks.length - 1;
			for (var i = 0; i < config.values[v].majorTicks.length; ++i) {
				var 
					p = rpoint(r, radians(config.values[v].offset + i * config.values[v].angle / range)),
					idx = (config.values[v].direction == 'ccw' ? config.values[v].majorTicks.length - 1 - i : i)
				;
	
				ctx.font      = 20 * (max / 200) + "px Arial";
				ctx.fillStyle = config.colors.numbers;
				ctx.lineWidth = 0;
				ctx.textAlign = "center";
				ctx.fillText(config.values[v].majorTicks[idx], p.x, p.y + 3);
			}
		}
	}

	// title draw
	function drawTitle(ctx) {
		var 
			r = max * 0.45,
			l = config.values.length,
			fh = 24 * (max / 200) - (1.6 * (l - 1)),
			y;
		;
		for (var v = 0; v < l; v++) {
			if (!config.values[v].title) {
				continue;
			}
			y = r * (v - (l - 1) / 2) * (0.2 + 1 / l) - fh / 2;
			
			ctx.save();
			ctx.font = fh + "px Arial";
			ctx.fillStyle = config.colors.title;
			ctx.textAlign = "center";
			ctx.fillText(config.values[v].title, 0, y);
			ctx.restore();
		}
	}

	// units draw
	function drawUnits(ctx) {
//		for (var v = 0; v < config.values.length; v++) {
//			if (!config.values[v].units) {
//				return;
//			}
//	
//			ctx.save();
//			ctx.font = 22 * (max / 200) + "px Arial";
//			ctx.fillStyle = config.colors.units;
//			ctx.textAlign = "center";
//			ctx.fillText(config.values[v].units, 0, max / 3.25 * v);
//			ctx.restore();
//		}
	}

	function padValue(val, cint, cdec) {
		val = parseFloat(val);
		var n = (val < 0);
		val = Math.abs(val);

		if (cdec > 0) {
			val = val.toFixed(cdec).toString().split('.');
	
			for (var i = 0, s = cint - val[0].length; i < s; ++i) {
				val[0] = '0' + val[0];
			}

			val = (n ? '-' : '') + val[0] + '.' + val[1];
		} else {
			val = Math.round(val).toString();

			for (var i = 0, s = cint - val.length; i < s; ++i) {
				val = '0' + val;
			}

			val = (n ? '-' : '') + val
		}
		return val;
	}

	function rpoint(r, a) {
		var 
			x = 0, y = r,

			sin = Math.sin(a),
			cos = Math.cos(a),

			X = x * cos - y * sin,
			Y = x * sin + y * cos
		;

		return { x : X, y : Y };
	}

	// draws the highlight colors
	function drawHighlights(ctx) {
		ctx.save();

		var r1 = max / 100 * 84;
		var width = max / 100 * 9;

		for (var v = 0; v < config.values.length; v++) {
			for (var i = 0, s = config.values[v].highlights.length; i < s; i++) {
				var
					hlt = config.values[v].highlights[i],
					range = config.values[v].maxValue - config.values[v].minValue,
					sa, ea
				;
				if (config.values[v].direction == 'ccw') {
					sa = (config.values[v].maxValue - hlt.to) * config.values[v].angle / range;
					ea = (config.values[v].maxValue - hlt.from) * config.values[v].angle / range;
				} else {
					sa = (hlt.from - config.values[v].minValue) * config.values[v].angle / range;
					ea = (hlt.to - config.values[v].minValue) * config.values[v].angle / range;
				}

				ctx.beginPath();
				ctx.arc(0, 0, r1, radians(90 + config.values[v].offset + sa), radians(90 + config.values[v].offset + ea), false);
				ctx.lineWidth = width;
				ctx.strokeStyle = hlt.color;
				ctx.stroke();
			}
		}
	}

	// draw the gauge needle(s)
	function drawNeedle(id) {
		var
			ctx = ctxDial[id],
			rIn  = max / 100 * 94,
			rOut = max / 100 * 25,
			pad1 = max / 100 * 1,
			pad2 = max / 100 * 1,
			value = values[id],

			shad = function() {
				ctx.shadowOffsetX = 4;
				ctx.shadowOffsetY = 4;
				ctx.shadowBlur    = 10;
				ctx.shadowColor   = 'rgba(188, 143, 143, 0.45)';
			}
		;
		
		if(typeof ctx == "undefined") {
			console.log("ERROR: trying to access nonexisting canvas context #" + id);
			return;
		}

		// clear the canvas
		ctx.clearRect(-CX, -CY, CW, CH);
		ctx.save();

		shad();
		
		ctx.save();
		
		if (fromValue[id] < 0) {
			fromValue[id] = Math.abs(value.minValue - fromValue[id]);
		} else if (value.minValue > 0) {
			fromValue[id] -= value.minValue;
		} else {
			fromValue[id] = Math.abs(value.minValue) + fromValue[id];
		}
		var range = value.maxValue - value.minValue;
		ctx.rotate(radians(value.offset + (value.direction == 'ccw' ? range - fromValue[id] : fromValue[id]) * value.angle / range));

		ctx.beginPath();
		ctx.moveTo(-pad2, -rOut);
		ctx.lineTo(-pad1, 0);
		ctx.lineTo(-1, rIn);
		ctx.lineTo(1, rIn);
		ctx.lineTo(pad1, 0);
		ctx.lineTo(pad2, -rOut);
		ctx.closePath();

		ctx.fillStyle = lgrad(ctx,
			config.colors.needle.start,
			config.colors.needle.end,
			rIn - rOut
		);
		ctx.fill();
		ctx.restore();
	}

	function drawInfoArea(ctx) {
		var
			r1 = max / 100 * 45,
			r2 = max / 100 * 44;
		;

		ctx.beginPath();
		ctx.arc(0, 0, r1, 0, Math.PI * 2, true);
		ctx.fillStyle = lgrad(ctx, 'rgba(128, 128, 128, 1)', 'rgba(160, 160, 160, 1)', r1);
		ctx.fill();

		ctx.restore();

		ctx.beginPath();
		ctx.arc(0, 0, r2, 0, Math.PI * 2, true);
		ctx.fillStyle = lgrad(ctx, 'rgba(32, 32, 32, 0.9)', 'rgba(48, 48, 48, 0.9)', r2);
		ctx.fill();
	}
	
	function drawArc(id) {
		var
			ctx = ctxDial[id],
			rIn  = max / 100 * 91,
			width = max / 100 * 6,
			value = values[id],
			range = value.maxValue - value.minValue,
			as, ae
		;
		
		if (value.direction == 'ccw') {
			as = 90 + value.offset + (range - fromValue[id]) * value.angle / range;
			ae = 90 + value.offset - (value.minValue < 0 ? value.minValue : -range) * value.angle / range;
		} else {
			as = 90 + value.offset - (value.minValue < 0 ? value.minValue * value.angle / range : 0);
			ae = 90 + value.offset + fromValue[id] * value.angle / range;
		}
		
		if (value.minValue < 0 && fromValue[id] < Math.abs(value.minValue)) {
			var temp = as;
			as = ae;
			ae = temp;
		}
		
		
		ctx.shadowOffsetX = 2;
		ctx.shadowOffsetY = 2;
		ctx.shadowBlur    = 10;
		ctx.shadowColor   = 'rgba(143, 143, 188, 0.45)';

		ctx.beginPath();
		ctx.arc(0, 0, rIn, radians(as), radians(ae), false);
		ctx.lineWidth = width;

		
		var grad = ctx.createLinearGradient(0, -200, 0, 30);  
		grad.addColorStop(0, 'rgba(243, 22, 22, 0.9)');  
		grad.addColorStop(1, 'rgba(143, 163, 188, 0.75)');
		ctx.strokeStyle = grad;
		ctx.stroke();
	}
	
	function roundRect(ctx, x, y, w, h, r) {
		ctx.beginPath();

		ctx.moveTo(x + r, y);
		ctx.lineTo(x + w - r, y);

		ctx.quadraticCurveTo(x + w, y, x + w, y + r);
		ctx.lineTo(x + w, y + h - r);

		ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h);
		ctx.lineTo(x + r, y + h);

		ctx.quadraticCurveTo(x, y + h, x, y + h - r);
		ctx.lineTo(x, y + r);

		ctx.quadraticCurveTo(x, y, x + r, y);

		ctx.closePath();
	}

	// value box draw
	function drawValueBox(ctx) {
		ctx.save();
		
		for (var v = 0; v < config.values.length; v++) {
			ctx.font = (max / 5) + "px Led";
			var
				cdec = config.values[v].valueFormat['dec'],
				cint = config.values[v].valueFormat['int'],
				tw   = ctx.measureText('-' + padValue(0, cint, cdec)).width,
				y = max / 4 * v,
				x = 0,
				th = 0.12 * max
			;
	
			ctx.save();
	
			roundRect(ctx,
				-tw / 2 - 0.025 * max,
				y - th - 0.04 * max,
				tw + 0.05 * max,
				th + 0.07 * max,
				0.025 * max
			);
	
			var grd = ctx.createRadialGradient(
				x,
				y - 0.12 * max - 0.025 * max + (0.12 * max + 0.045 * max) / 2,
				max / 10,
				x,
				y - 0.12 * max - 0.025 * max + (0.12 * max + 0.045 * max) / 2,
				max / 5
			);
	
			grd.addColorStop(0, "#888");
		    grd.addColorStop(1, "#666");
	
			ctx.strokeStyle = grd;
			ctx.lineWidth = 0.05 * max;
			ctx.stroke();
	
			ctx.shadowBlur  = 0.012 * max;
			ctx.shadowColor = 'rgba(0, 0, 0, 1)';
	
			ctx.fillStyle = "#babab2";
			ctx.fill();
			
			ctx.restore();
		}
	}

	function drawValue(ctx, id) {
		ctx.save();
		
		var 
			r = max * 0.45,
			l = config.values.length,
			fh = 42 * (max / 200) - (1.6 * (l - 1))
		;

		for (var v = 0; v < config.values.length; v++) {
			if (config.values[v].id != id) 
				continue;
			
			ctx.font = fh + "px Led";
			var
				cdec = config.values[v].valueFormat['dec'],
				cint = config.values[v].valueFormat['int'],
				text = padValue(value[id], cint, cdec),
				y = r * (v - (l - 1) / 2) * (0.2 + 1 / l) + fh * 0.55;
				x = 0
			;
			ctx.clearRect(ctx.canvas.width / -2, y - max / 5, ctx.canvas.width, max / 4.5);
	
			ctx.save();
	
			ctx.shadowOffsetX = 0.004 * max;
			ctx.shadowOffsetY = 0.004 * max;
			ctx.shadowBlur    = 0.012 * max;
			ctx.shadowColor   = 'rgba(0, 0, 0, 0.3)';
	
//			ctx.fillStyle = "#444";
			ctx.fillStyle = "#eeb";
			ctx.textAlign = "center";
			ctx.fillText(text, -x, y);
	
			ctx.restore();
		}
	}

}

function constrain(val,low,high) {
	return (val < low ? low : (val > high ? high : val));
}

// initialize (load font by adding style to the head and temporarely adding a
// div which refers to the font
Gauge.initialized = false;
(function(){
	var
		d = document,
		h = d.getElementsByTagName('head')[0],
		url = 'fonts/digital-7-mono.ttf',
		text = "@font-face {" +
				"font-family: 'Led';" +
				"src: url('" + url + "');" +
				"}",
		ss,
		r = d.createElement('style')
	;

	r.type = 'text/css';

	try {
		r.appendChild(d.createTextNode(text));
	} catch (e) {
		r.cssText = text;
	}

	h.appendChild(r);

	ss = r.styleSheet ? r.styleSheet : (r.sheet || d.styleSheets[d.styleSheets.length - 1]);

	var iv = setInterval(function() {
		if (!d.body) {
			return;
		}

		clearInterval(iv);

		var dd = d.createElement('div');

		dd.style.fontFamily = 'Led';
		dd.style.position   = 'absolute';
		dd.style.height     = dd.style.width = 0;
		dd.style.overflow   = 'hidden';

		dd.innerHTML = '.';

		d.body.appendChild(dd);

		// no other way to handle font is rendered by a browser just give the
		// browser around 250ms to do that
		setTimeout(function() { Gauge.initialized = true; dd.parentNode.removeChild(dd); }, 250);
	}, 1);
})();

Gauge.Collection = [];
Gauge.Collection.get = function(id) {
	return this[id];
}

window['Gauge'] = Gauge;

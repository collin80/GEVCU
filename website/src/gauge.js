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
		drawArc     : true,
		drawHighlights: false,
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
				ccw         : false,
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
			delay    : 35,
			duration : 200,
			fn       : 'linear'
		}
	}

	var
		configValues = [],
		self      = this,
		imready   = false
	;

	/**
	 * ********* Public methods - to be called from the application **********
	 */
	
	/**
	 * Sets a new value to a dial and updates the view
	 * 
	 * @param {number}
	 *            id - the id of the value to update
	 * @param {number}
	 *            val - the new value to set
	 */
	this.setValue = function(id, val) {
		var value = configValues[id];

		value.value = val;
		val = constrain(val, value.minValue, value.maxValue);
		value.fromValue = config.animation ? value.toValue : val;
		value.toValue = val;

		drawValue(ctxInfoValues, value);
		config.animation ? animate(value) : draw(value);
	}
	
	/**
	 * Sets a new value to a dial and updates the view without any
	 * animation (even if configured)
	 * 
	 * @param {number}
	 *            id - the id of the value to update
	 * @param {number}
	 *            val - the new value to set
	 */
	this.setRawValue = function(id, val) {
		var value = configValues[id];

		value.value = val;
		value.fromValue = constrain(val, value.minValue, value.maxValue);

		drawValue(ctxInfoValues, value);
		draw(value);
	}

	/**
	 * Clears the value of a dial
	 * 
	 * @param {number}
	 *            id - the id of the value to update
	 */
	this.clear = function(id) {
		var value = configValues[id];

		value.value = value.fromValue = value.toValue = value.minValue;
		drawValue(ctxInfoValues, value);
		draw(value);
	}

	/**
	 * Returns the current value been set to a dial
	 * 
	 * @param {number}
	 *            id - the id of the value
	 * @return {number} value - current value
	 */
	this.getValue = function(id) {
		var value = configValues[id];
		return value.value;
	}

	/**
	 * Updates the gauge config
	 * 
	 * @param {Object}
	 *            config to set
	 */
	this.updateConfig = function(config) {
        applyRecursive(this.config, config);
        thisbaseInit();
		drawValue();
        draw();
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

	var ctxInfo, ctxInfoValues,
		CW  = config.width,
		CH  = config.height,
		CX  = CW / 2,
		CY  = CH / 2,
		max = CX < CY ? CX : CY,
		animateFx = {
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
	;

	function baseInit() {
		if(typeof config.animation.fn != "function") {
			config.animation.fn = animateFx[config.animation.fn];
		}
		
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
			var value = config.values[i];
			value.position = i;
			value.ctx = prepareCanvas(value.id, true);
			value.fromValue = value.value = value.toValue = value.minValue;
			value.offset = config.start + (config.angle + config.gap) * i / config.values.length;
			value.angle = (config.angle - config.gap * (config.values.length - 1)) / config.values.length;
			value.range = value.maxValue - value.minValue;
			if(typeof value.startValue == "undefined") {
				value.startValue = 0;
			}
			configValues[value.id] = value;
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
		
		draw();
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

	function _animate(opts) {
		var
			progress = (Date.now() - opts.start) / opts.cfg.duration,
			value = opts.value,
			cfg = opts.cfg
		;
		
		if (progress > 1) {
			progress = 1;
		}

		value.fromValue = opts.from + (value.toValue - opts.from) * cfg.fn(progress);
		draw(value);

		if (progress == 1) {
			clearInterval(value.animateInterval);
		}
	}

	function animate(value) {
		// stop previous animation
		value.animateInterval && clearInterval(value.animateInterval); 
		value.animateInterval = setInterval(_animate, config.animation.delay, {
			value: value,
			start: Date.now(),
			from: value.fromValue,
			cfg: config.animation
		});
	}

	/**
	 * Draws the variable parts of the gauge.
	 * 
	 * @return {Gauge} this - returns the self Gauge object
	 */
	function draw(value) {
		// draw all values if no id is passed
		if(typeof value == "undefined") {
			for (var i = 0; i < configValues.length; i++) {
				this.draw(configValues[i]);
			}
			return;
		}

		drawNeedle(value);
		drawArc(value);

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
					idx = (config.values[v].ccw ? config.values[v].majorTicks.length - 1 - i : i)
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
		if (!config.drawHighlights) {
			return;
		}

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
				if (config.values[v].ccw) {
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
	
	var
		dialRIn  = max / 100 * 94,
		dialROut = max / 100 * 25,
		dialPad1 = max / 100 * 1,
		dialPad2 = max / 100 * 1,
		dialShadowColor = '#000',
		dialFillStyle = false
	;

	// draw the gauge needle(s)
	function drawNeedle(value) {
		var
			ctx = value.ctx,
			from = value.fromValue
		;

		// clear the canvas
		ctx.clearRect(-CX, -CY, CW, CH);
		ctx.save();
		
		if (from < 0) {
			from = Math.abs(value.minValue - from);
		} else if (value.minValue > 0) {
			from -= value.minValue;
		} else {
			from = Math.abs(value.minValue) + from;
		}
		value.fromValue = from;
		ctx.rotate(radians(value.offset + (value.ccw ? value.range - from : from) * value.angle / value.range));

		if (config.glow) {
			ctx.shadowOffsetX = 2;
			ctx.shadowOffsetY = 2;
			ctx.shadowBlur    = 3;
			ctx.shadowColor   = dialShadowColor;
		}

		ctx.beginPath();
		ctx.moveTo(-dialPad2, -dialROut);
		ctx.lineTo(-dialPad1, 0);
		ctx.lineTo(-1, dialRIn);
		ctx.lineTo(1, dialRIn);
		ctx.lineTo(dialPad1, 0);
		ctx.lineTo(dialPad2, -dialROut);
		ctx.closePath();

		if (!dialFillStyle) {
			dialFillStyle = lgrad(ctx, config.colors.needle.start, config.colors.needle.end, dialRIn - dialROut);
		}
		ctx.fillStyle = dialFillStyle;
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
	
	var
		arcR  = max / 100 * 91,
		arcWidth = max / 100 * 7,
		arcShadowColor = 'rgba(200, 200, 235, 0.9)'
	;

	function drawArc(value) {
		if (!config.drawArc) {
			return;
		}
		
		var
			ctx = value.ctx,
			as, ae
		;
		
		if (value.ccw) {
			as = 90 + value.offset + (value.range - value.fromValue) * value.angle / value.range;
			ae = 90 + value.offset + (value.minValue < value.startValue ? value.range - value.startValue + value.minValue : value.range) * value.angle / value.range;
		} else {
			as = 90 + value.offset + (value.minValue < value.startValue ? value.startValue - value.minValue : 0) * value.angle / value.range;
			ae = 90 + value.offset + value.fromValue * value.angle / value.range;
		}
		
		if (value.minValue < value.startValue && (value.fromValue + value.minValue) < value.startValue) {
			var temp = as;
			as = ae;
			ae = temp;
		}

		if (config.glow) {
			ctx.shadowOffsetX = 2;
			ctx.shadowOffsetY = 2;
			ctx.shadowBlur    = 10;
			ctx.shadowColor   = arcShadowColor;
		}

		ctx.beginPath();
		ctx.arc(0, 0, arcR, radians(as), radians(ae), false);
		ctx.lineWidth = arcWidth;

		
		if (!value.arcStrokeStyle) {
			var
				range = value.majorTicks.length - 1,
				p1 = rpoint(arcR, radians(value.offset)),
				p2 = rpoint(arcR, radians(value.offset + value.angle))
			;
			value.arcStrokeStyle = ctx.createLinearGradient(p1.x, p1.y, p2.x, p2.y);
			for (var i = 0; i < value.highlights.length; i++) {
				var
					hlt = value.highlights[i],
					range = (value.minValue < 0 ? value.maxValue : value.maxValue - value.minValue)
				;
				value.arcStrokeStyle.addColorStop(constrain((value.ccw ? value.maxValue - (hlt.to + hlt.from)/2 : (hlt.to + hlt.from)/2 - value.minValue) / range, 0, 1), hlt.color);  
			}
		}
		ctx.strokeStyle = value.arcStrokeStyle;
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

	function drawValue(ctx, value) {
		var 
			r = max * 0.45,
			l = config.values.length,
			fh = 42 * (max / 200) - (1.6 * (l - 1))
		;

		ctx.save();
		ctx.font = fh + "px Led";
		var
			cdec = value.valueFormat['dec'],
			cint = value.valueFormat['int'],
			text = padValue(value.value, cint, cdec),
			y = r * (value.position - (l - 1) / 2) * (0.2 + 1 / l) + fh * 0.55;
			x = 0
		;
		ctx.clearRect(ctx.canvas.width / -2, y - max / 5, ctx.canvas.width, max / 4.5);

		ctx.save();

		ctx.shadowOffsetX = 0.004 * max;
		ctx.shadowOffsetY = 0.004 * max;
		ctx.shadowBlur    = 0.012 * max;
		ctx.shadowColor   = 'rgba(0, 0, 0, 0.3)';
	
//		ctx.fillStyle = "#444";
		ctx.fillStyle = "#eeb";
		ctx.textAlign = "center";
		ctx.fillText(text, -x, y);

		ctx.restore();
	}

	function constrain(val,low,high) {
		return (val < low ? low : (val > high ? high : val));
	}
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

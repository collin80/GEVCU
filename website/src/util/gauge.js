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
 * @constructor with default gauge configuration
 */
var Gauge = function(gaugeConfig) {
	var config = {
		renderTo    : null,
		width       : 200,
		height      : 200,
		start       : 15,
		angle       : 330,
		gap         : 30,
		glow        : true,
		drawHighlights: false,
		colors      : {
			plate      : '#fff',
			majorTicks : '#444',
			minorTicks : '#666',
			title      : '#888',
			units      : '#888',
			numbers    : '#444',
			needle     : { start : 'rgba(240, 128, 128, 1)', end : 'rgba(255, 160, 122, .9)' },
			limits     : { start : 'rgba(100, 150, 180, 1)', end : 'rgba(100, 150, 180, .9)' }
		},
		dials       : [
			{
				id         : false,
				title      : false,
				units      : false,
				minValue   : 0,
				maxValue   : 100,
				ccw        : false,
				drawArc    : true,
				drawLimits : true,
				limits     : {
					min: 0,
					max: 100
				},
				majorTicks  : [],
				minorTicks  : 10,
				strokeTicks : true,
				valueFormat : { "ints" : 3, "dec" : 2 },
		        majorTicksFormat : { "ints" : 1, "dec" : 0 },
				highlights  : [
					{ from : 20, to : 60, color : '#eee' },
					{ from : 60, to : 80, color : '#ccc' },
					{ from : 80, to : 100, color : '#999' }],
				animation : {
					delay    : 70,
					duration : 200,
					fn       : 'linear'
				}
			}
		],
	}

	/**
	 * Recursively copy the values from one config to another. (to not lose the default values)
	 */
	function copyConfig(dst, src) {
		for (var i in src) {
			if (typeof src[i] == "object" && !(Object.prototype.toString.call(src[i]) === '[object Array]') && i != 'renderTo') {
				if (typeof dst[i] != "object") {
					dst[i] = {};
				}
				copyConfig(dst[i], src[i]);
			} else {
				dst[i] = src[i];
			}
		}
	}

	copyConfig(config, gaugeConfig);
	if (!config.renderTo) {
		throw Error("Canvas element was not specified when creating the Gauge object!");
	}

	var ctxInfo, ctxInfoValues, ctxLimits,
		CW  = config.width,
		CH  = config.height,
		CX  = CW / 2,
		CY  = CH / 2,
		max = CX < CY ? CX : CY,
		dialRIn  = max / 100 * 94,
		dialROut = max / 100 * 25,
		dialPad1 = max / 100 * 1,
		dialPad2 = max / 100 * 1,
		dialShadowColor = '#000',
		dialFillStyle = false
	;

	function baseInit() {
		// important. first add all canvas before accessing one !!
		addCanvas(config.renderTo + "Plate");
		for (var i = 0; i < config.dials.length; i++) {
			addCanvas(config.dials[i].id);
		}
		addCanvas(config.renderTo + "Limits")
		addCanvas(config.renderTo + "Info");
		addCanvas(config.renderTo + "InfoValues");

		// now it's safe to look them up
		var ctxPlate = prepareCanvas(config.renderTo + "Plate", false);
		for (var i = 0; i < config.dials.length; i++) {
			var dialConfig=config.dials[i];
			var ctx = prepareCanvas(dialConfig.id, true);
			dialConfig.offset = config.start + (config.angle + config.gap) * i / config.dials.length;
			dialConfig.angle = (config.angle - config.gap * (config.dials.length - 1)) / config.dials.length;
			dialConfig.range = dialConfig.maxValue - dialConfig.minValue;
			dialConfig.colors = config.colors;
			if (dialConfig.animation) {
				dialConfig.animation.delay = dialConfig.animation.delay || 70; 
				dialConfig.animation.duration = dialConfig.animation.duration || 200;
				dialConfig.animation.fn = dialConfig.animation.fn || 'linear';
				dialConfig.animation.threshold = dialConfig.animation.threshold || (dialConfig.range / dialConfig.angle / 5);
				dialConfig.textPosition = i;
				dialConfig.glow = config.glow;
				dialConfig.startValue = dialConfig.startValue || 0; 
			}
			Gauge.dials[dialConfig.id.substring(0, dialConfig.id.length - 4)] = new Dial(ctx, dialConfig);
		}
		ctxLimits = prepareCanvas(config.renderTo + "Limits", true);
		ctxInfo = prepareCanvas(config.renderTo + "Info", true);
		ctxInfoValues = prepareCanvas(config.renderTo + "InfoValues", true);
		ctxInfoValues.shadowOffsetX = 0.004 * max;
		ctxInfoValues.shadowOffsetY = 0.004 * max;
		ctxInfoValues.shadowBlur    = 0.012 * max;
		ctxInfoValues.shadowColor   = 'rgba(0, 0, 0, 0.3)';
		ctxInfoValues.fillStyle = "#adf";
		ctxInfoValues.textAlign = "center";
		
		drawPlate(ctxPlate);
		drawHighlights(ctxPlate);
		drawMinorTicks(ctxPlate);
		drawMajorTicks(ctxPlate);
		drawNumbers(ctxPlate);
		
		drawInfoArea(ctxInfo);
		drawTitle(ctxInfo);
// drawUnits(ctxInfo);
// drawValueBox(ctxInfo);
	}

	baseInit();
	
	/**
	 * Add a canvas with a specified id to the 'renderTo' container.
	 */
	function addCanvas(canvasId) {
		var container = config.renderTo.tagName ? config.renderTo : document.getElementById(config.renderTo);
		container.innerHTML += "<canvas id='" + canvasId + "'></canvas>";
	}

	/**
	 * Resize, shift and translate a canvas / context.
	 */
	function prepareCanvas(canvasId, relocate) {
		var canvas = document.getElementById(canvasId);
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

	/**
	 * Transforms degrees to radians
	 */
	function radians(degrees) {
		return degrees * Math.PI / 180;
	}

	/**
	 * Linear gradient
	 */
	function linearGradient(ctx, clrFrom, clrTo, len) {
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
		ctx.fillStyle = linearGradient(ctx, '#ddd', '#aaa', r0);
		ctx.fill();

		ctx.restore();

		ctx.beginPath();
		ctx.arc(0, 0, r1, 0, Math.PI * 2, true);
		ctx.fillStyle = linearGradient(ctx, '#fafafa', '#ccc', r1);
		ctx.fill();

		ctx.beginPath();
		ctx.arc(0, 0, r2, 0, Math.PI * 2, true);
		ctx.fillStyle = linearGradient(ctx, '#eee', '#f0f0f0', r2);
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
	 * Formats a number for display on the dial's plate using the majorTicksFormat config option.
	 * 
	 * @param {number}
	 *            num The number to format
	 * @returns {string} The formatted number
	 */
    function formatMajorTickNumber(num, dialNumber) {
        var r, isDec = false;
        var ints = 3, dec = 0;
        
        if (config.dials[dialNumber] && config.dials[dialNumber].majorTicksFormat) {
        	ints = config.dials[dialNumber].majorTicksFormat.ints;
        	dec = config.dials[dialNumber].majorTicksFormat.dec;
        }

        // First, force the correct number of digits right of the decimal.
        if(dec === 0) {
            r = Math.round(num).toString();
        } else {
            r = num.toFixed(dec);
        }

        // Second, force the correct number of digits left of the decimal.
        if(ints > 1) {
            // Does this number have a decimal?
            isDec = (r.indexOf('.') > -1);

            // Is this number a negative number?
            if(r.indexOf('-') > -1) {
                return '-' + [
					ints + dec + 2 + (isDec ? 1 : 0) - r.length
				].join('0') + r.replace('-', '');
            } else {
                return [
                	ints + dec + 1 + (isDec ? 1 : 0) - r.length
				].join('0') + r;
            }
        } else {
            return r;
        }
    }

	/**
	 * draw major ticks
	 */
	function drawMajorTicks(ctx) {
		var r = max / 100 * 88;

		ctx.lineWidth = 2;
		ctx.strokeStyle = config.colors.majorTicks;
		ctx.save();

		for (var i = 0; i < config.dials.length; i++) {
			var dialConfig = config.dials[i];
	        if(dialConfig.majorTicks.length == 0) {
	            var numberOfDefaultTicks = 5;
	            var tickSize = (dialConfig.maxValue - dialConfig.minValue) / numberOfDefaultTicks;
	
	            for(var j = 0; j < numberOfDefaultTicks; j++) {
	            	dialConfig.majorTicks.push(formatMajorTickNumber(dialConfig.minValue + (tickSize * j), i));
	            }
	            dialConfig.majorTicks.push(formatMajorTickNumber(dialConfig.maxValue), i);
	        }
	
			var range = dialConfig.majorTicks.length - 1;
			for (var j = 0; j < dialConfig.majorTicks.length; j++) {
				var tick = dialConfig.majorTicks[j];
				ctx.rotate(radians(dialConfig.offset + j * dialConfig.angle / range));

				ctx.beginPath();
				ctx.moveTo(0, r);
				ctx.lineTo(0, r - max / 100 * 15);
				ctx.stroke();

				ctx.restore();
				ctx.save();
			}
	
			if (dialConfig.strokeTicks) {
				ctx.rotate(radians(90));

				ctx.beginPath();
				ctx.arc(0, 0, r, radians(config.start), radians(config.start + config.angle), false);
				ctx.stroke();
				ctx.restore();
				ctx.save();
			}
		}
	}

	/**
	 * draw minor ticks
	 */
	function drawMinorTicks(ctx) {
		var r = max / 100 * 88;

		ctx.lineWidth = 1;
		ctx.strokeStyle = config.colors.minorTicks;

		ctx.save();

		for (var i = 0; i < config.dials.length; i++) {
			var dialConfig = config.dials[i];
			var range = dialConfig.minorTicks * (dialConfig.majorTicks.length - 1);	
			for (var j = 0; j < range; ++j) {
				ctx.rotate(radians(dialConfig.offset + j * dialConfig.angle / range));
	
				ctx.beginPath();
				ctx.moveTo(0, r);
				ctx.lineTo(0, r - max / 100 * 7.5);
				ctx.stroke();
	
				ctx.restore();
				ctx.save();
			}
		}
	}

	/**
	 * draw the numbers of the plate
	 */
	function drawNumbers(ctx) {
		var r = max / 100 * 60;

		for (var i = 0; i < config.dials.length; i++) {
			var dialConfig = config.dials[i];
			var range = dialConfig.majorTicks.length - 1;
			for (var j = 0; j < dialConfig.majorTicks.length; ++j) {
				var 
					p = rpoint(r, radians(dialConfig.offset + j * dialConfig.angle / range)),
					idx = (dialConfig.ccw ? dialConfig.majorTicks.length - 1 - j : j)
				;
	
				ctx.font      = 20 * (max / 200) + "px Arial";
				ctx.fillStyle = config.colors.numbers;
				ctx.lineWidth = 0;
				ctx.textAlign = "center";
				ctx.fillText(dialConfig.majorTicks[idx], p.x, p.y + 3);
			}
		}
	}

	/**
	 * draw the dial titles
	 */
	function drawTitle(ctx) {
		var 
			r = max * 0.45,
			l = config.dials.length,
			fh = 24 * (max / 200) - (1.6 * (l - 1)),
			y;
		;
		for (var v = 0; v < l; v++) {
			if (!config.dials[v].title) {
				continue;
			}
			y = r * (v - (l - 1) / 2) * (0.2 + 1 / l) - fh / 2;
			
			ctx.save();
			ctx.font = fh + "px Arial";
			ctx.fillStyle = config.colors.title;
			ctx.textAlign = "center";
			ctx.fillText(config.dials[v].title, 0, y);
			ctx.restore();
		}
	}

	/**
	 * draw the units
	 */
	function drawUnits(ctx) {
		for (var v = 0; v < config.dials.length; v++) {
			if (!config.dials[v].units) {
				return;
			}
	
			ctx.save();
			ctx.font = 22 * (max / 200) + "px Arial";
			ctx.fillStyle = config.colors.units;
			ctx.textAlign = "center";
			ctx.fillText(config.dials[v].units, 0, max / 3.25 * v);
			ctx.restore();
		}
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

	/**
	 * draws the highlight colors
	 */
	function drawHighlights(ctx) {
		if (!config.drawHighlights) {
			return;
		}

		ctx.save();

		var r1 = max / 100 * 84;
		var width = max / 100 * 9;

		for (var i = 0; i < config.dials.length; i++) {
			var dialConfig = config.dials[i];
			for (var j = 0; j < dialConfig.highlights.length; j++) {
				var hlt = dialConfig.highlights[j];
				var
					range = dialConfig.maxValue - dialConfig.minValue,
					sa, ea
				;
				if (dialConfig.ccw) {
					sa = (dialConfig.maxValue - hlt.to) * dialConfig.angle / range;
					ea = (dialConfig.maxValue - hlt.from) * dialConfig.angle / range;
				} else {
					sa = (hlt.from - dialConfig.minValue) * dialConfig.angle / range;
					ea = (hlt.to - dialConfig.minValue) * dialConfig.angle / range;
				}

				ctx.beginPath();
				ctx.arc(0, 0, r1, radians(90 + dialConfig.offset + sa), radians(90 + dialConfig.offset + ea), false);
				ctx.lineWidth = width;
				ctx.strokeStyle = hlt.color;
				ctx.stroke();
			}
		}
	}
	
	function drawInfoArea(ctx) {
		var r1 = max / 100 * 45;
		var r2 = max / 100 * 44;

		ctx.beginPath();
		ctx.arc(0, 0, r1, 0, Math.PI * 2, true);
		ctx.fillStyle = linearGradient(ctx, 'rgba(128, 128, 128, 1)', 'rgba(160, 160, 160, 1)', r1);
		ctx.fill();

		ctx.restore();

		ctx.beginPath();
		ctx.arc(0, 0, r2, 0, Math.PI * 2, true);
		ctx.fillStyle = linearGradient(ctx, 'rgba(32, 32, 32, 0.9)', 'rgba(48, 48, 48, 0.9)', r2);
		ctx.fill();
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
		
		for (var v = 0; v < config.dials.length; v++) {
			ctx.font = (max / 5) + "px Led";
			var
				cdec = config.dials[v].valueFormat['dec'],
				cint = config.dials[v].valueFormat['ints'],
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

	function constrain(val,low,high) {
		return (val < low ? low : (val > high ? high : val));
	}

	function drawLimits() {
		ctxLimits.clearRect(-CX, -CY, CW, CH);
		
		for (var i = 0; i < config.dials.length; i++) {
			var dialConfig = config.dials[i];
			var cfg = Gauge.dials[dialConfig.id.substring(0, dialConfig.id.length - 4)].getConfig();
			if (cfg.limits) {
	 			if (cfg.limits.min || cfg.limits.min === 0) {
					drawLimitNeedle(cfg, cfg.limits.min);
				}
				if (cfg.limits.max || cfg.limits.max === 0) {
					drawLimitNeedle(cfg, cfg.limits.max);
				}
			}
		}
	}

	function drawLimitNeedle(cfg, value) {
		if (value < 0) {
			value = Math.abs(cfg.minValue - value);
		} else if (cfg.minValue > 0) {
			value -= cfg.minValue;
		} else {
			value = Math.abs(cfg.minValue) + value;
		}
		var angle = radians(cfg.offset + (cfg.ccw ? cfg.range - value : value) * cfg.angle / cfg.range);

		ctxLimits.save();
		ctxLimits.rotate(angle);

		ctxLimits.beginPath();
		ctxLimits.moveTo(-dialPad2, -dialROut * .85);
		ctxLimits.lineTo(-dialPad1, 0);
		ctxLimits.lineTo(-1, dialRIn * .85);
		ctxLimits.lineTo(1, dialRIn * .85);
		ctxLimits.lineTo(dialPad1, 0);
		ctxLimits.lineTo(dialPad2, -dialROut * .85);
		ctxLimits.closePath();

		ctxLimits.fillStyle = linearGradient(ctxLimits, config.colors.limits.start, config.colors.limits.end, dialRIn - dialROut);
		ctxLimits.fill();
		ctxLimits.restore();
	}

	function Dial(ctx, config) {
		var worker = createGaugeDialWorker();

		/**
		 * forward the value update to this dial's worker and wait for update messages
		 */
		this.setValue = function(value) {
			worker.postMessage({value: value});
		}
		this.setValue(config.startValue);

		/**
		 * set the limits of this dial and redraw them
		 */
		this.setLimits = function(min, max) {
			if (!config.limits) {
				config.limits = {};
			}
			config.limits.min = constrain(min, config.minValue, config.maxValue);
			config.limits.max = constrain(max, config.minValue, config.maxValue);
			drawLimits();
		}
		
		this.getConfig = function() {
			return config;
		}

		/**
		 * process update message from gaugeDial worker an redraw affected UI elements
		 */
		function update(data) {
			if (data.angle) {
				drawNeedle(data.angle);
				drawArc(data.arcStart, data.arcEnd);
			} else if (data.text) {
				drawValueText(data.text);
			}
		}
		
		/**
		 * Create a worker (thread) which processes value changes and generates updates to animate the dial
		 */
		function createGaugeDialWorker() {
			var worker = new Worker("worker/gaugeDial.js");
			worker.onmessage = function(event) {
				update(event.data);
			}
			worker.postMessage({config: config});
			return worker;
		}

		/**
		 * draw the dial's needle
		 */
		function drawNeedle(angle) {
			ctx.clearRect(-CX, -CY, CW, CH);
			ctx.save();
			ctx.rotate(angle);
	
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
				dialFillStyle = linearGradient(ctx, config.colors.needle.start, config.colors.needle.end, dialRIn - dialROut);
			}
			ctx.fillStyle = dialFillStyle;
			ctx.fill();
			ctx.restore();
		}

		// init as class variables so they don't have to be calculated with every update
		var
			arcR  = max / 100 * 91,
			arcWidth = max / 100 * 7,
			arcShadowColor = 'rgba(200, 200, 235, 0.9)'
		;

		/**
		 * draw the optional colored arc to indicate the needle position
		 */
		function drawArc(angleStart, angleEnd) {
			if (!config.drawArc) {
				return;
			}
			
			if (config.glow) {
				ctx.shadowOffsetX = 2;
				ctx.shadowOffsetY = 2;
				ctx.shadowBlur    = 10;
				ctx.shadowColor   = arcShadowColor;
			}

			ctx.beginPath();
			ctx.arc(0, 0, arcR, angleStart, angleEnd, false);
			ctx.lineWidth = arcWidth;

			if (!config.arcStrokeStyle) {
				var
					range = config.majorTicks.length - 1,
					p1 = rpoint(arcR, radians(config.offset)),
					p2 = rpoint(arcR, radians(config.offset + config.angle))
				;
				config.arcStrokeStyle = ctx.createLinearGradient(p1.x, p1.y, p2.x, p2.y);
				for (var i = 0; i < config.highlights.length; i++) {
					var
						hlt = config.highlights[i],
						range = (config.minValue < 0 ? config.maxValue : config.maxValue - config.minValue)
					;
					config.arcStrokeStyle.addColorStop(constrain((config.ccw ? config.maxValue - (hlt.to + hlt.from)/2 : (hlt.to + hlt.from)/2 - config.minValue) / range, 0, 1), hlt.color);  
				}
			}
			ctx.strokeStyle = config.arcStrokeStyle;
			ctx.stroke();
		}

		// init as class variables so they don't have to be calculated with every update
		var
        	textR = max * 0.45,
			numDials = gaugeConfig.dials.length,
			textSize = 42 * (max / 200) - (1.6 * (numDials - 1)),
			textYPos = textR * (config.textPosition - (numDials - 1) / 2) * (0.2 + 1 / numDials) + textSize * 0.55,
			textXPos = 0
		;

		/**
		 * write the dial's value to the center of the gauge
		 */
		function drawValueText(val) {
			ctxInfoValues.clearRect(ctxInfoValues.canvas.width / -2, textYPos - max / 5, ctxInfoValues.canvas.width, max / 4.5);
			ctxInfoValues.font = textSize + "px Led";
			ctxInfoValues.fillText(padValue(val, config.valueFormat.ints, config.valueFormat.dec), -textXPos, textYPos);
		}
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

/**
 * make the dials accessible to the world
 */
Gauge.dials = [];
window.Gauge = Gauge;

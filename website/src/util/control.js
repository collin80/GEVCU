var canvas; // global throttle canvas object
var nodecache = new Array(); // a cache with references to nodes - for faster location than DOM traversal
var worker; // a background worker to receive and process data


// resize the canvas to fill browser window dynamically
window.addEventListener('resize', resizeThrottleCanvas, false);

function resizeThrottleCanvas() {
	// adjust the width to the page width
	var canvasElement = document.getElementById("throttleCanvas");
	if (canvasElement) {
		// needs to be slightly narrower than the page width
		canvasElement.width = window.innerWidth - 60;
	}
	refreshThrottleVisualization();
}

function showTab(pageId) {
	// show the correct div and hide the others
	var tabs = document.getElementById('tabs');
	for (var i = 0; i < tabs.childNodes.length; i++) {
		var node = tabs.childNodes[i];
		if (node.nodeType == 1) { /* Element */
			node.style.display = (node.id == pageId) ? 'block' : 'none';
		}
	}

	// change the class of the selected tab
	var tabHeader = document.getElementById('tabHeader');
	var linkToActivate = document.getElementById(pageId + 'link');
	for (var i = 0; i < tabHeader.childNodes.length; i++) {
		var node = tabHeader.childNodes[i];
		if (node.nodeType == 1) { /* Element */
			node.className = (node == linkToActivate) ? 'on' : '';
		}
	}

	if (pageId == 'config') {
		resizeThrottleCanvas();
	}

	if (pageId == 'dashboard') {
		alertify.set('notifier','position', 'top-right');
		// add an event listener so sounds can get loaded on mobile devices after user interaction
		window.addEventListener('keydown', removeBehaviorsRestrictions);
		window.addEventListener('mousedown', removeBehaviorsRestrictions);
		window.addEventListener('touchstart', removeBehaviorsRestrictions);
		startWorker();
	} else {
		stopWorker();
		loadData(pageId);
	}
}

// on most mobile browsers sounds can only be loaded during a user interaction (stupid !)
function removeBehaviorsRestrictions() {
	soundError.load();
	soundWarn.load();
	soundInfo.load();
	window.removeEventListener('keydown', removeBehaviorsRestrictions);
	window.removeEventListener('mousedown', removeBehaviorsRestrictions);
	window.removeEventListener('touchstart', removeBehaviorsRestrictions);
}

function initWorker() {
	for (var name in Gauge.Collection) {
		if (name != "get") {
			var config = Gauge.Collection.get(name).config;
			
			for (var i = 0; i < config.values.length; i++) {
				worker.postMessage({cmd: 'init', config: {
					id: config.values[i].id,
					minValue: config.values[i].minValue,
					maxValue: config.values[i].maxValue,
					startValue: config.values[i].startValue,
					offset: config.values[i].offset,
					angle: config.values[i].angle,
					ccw: config.values[i].ccw,
					range: config.values[i].range,
					animation: config.animation,
				}});
			}
		}
	}
}

function startWorker() {
	if(typeof(worker) == "undefined") {
		worker = new PseudoWorker();
//		worker = new Worker("worker/handler.js");
		worker.onmessage = this.handleWorkerMessage;
	}
	worker.postMessage({cmd: 'start'});
}

function stopWorker() {
	if(typeof(worker) != "undefined") {
		worker.postMessage({cmd: 'stop'});
	}
}

this.handleWorkerMessage = function(event) {
	var data = event.data;
	if (data.dial) {
		var dial = data.dial;
		var gauge = Gauge.Collection.get(dial.id);
		if (gauge) {
			if (dial.angle) {
				gauge.drawNeedle(dial.id, dial.angle);
				gauge.drawArc(dial.id, dial.arcStart, dial.arcEnd);
			} else if (dial.text) {
				gauge.drawValue(dial.id, dial.text);
			}
		}
	} else if (data.node) {
		var name = data.node.name;
		var value = data.node.value;

		setNodeValue(name, value);

		if (name == 'systemState') {
			updateSystemState(value);
		}
		if (name == 'logMessage') {
			var message = value.message;
			var level = value.level;
			if (level == 'ERROR') {
				alertify.error(message, 0);
				soundError.play();
			} else if (level == 'WARNING') {
				alertify.warning(message, 60);
				soundWarn.play();
			} else {
				alertify.success(message, 30);
				soundInfo.play();
			}
		}
	}
}

function updateSystemState(state) {
	var div = document.getElementsByTagName('div')
	for (i = 0; i < div.length; i++) {
		var idStr = div[i].getAttribute('id');
		if (idStr && idStr.indexOf('state_') != -1) {
			if (idStr.indexOf('_' + state + '_') != -1) {
				div[i].className = 'visible';
			} else {
				div[i].className = 'hidden';
			}
		}
	}
}

// send message via handler to websocket
function sendMsg(message) {
	worker.postMessage({cmd: 'message', message: message + '     '});
}

// lazy load of page, replaces content of div with id==<pageId> with
// content of remote file <pageId>.htm
function loadPage(pageId) {
	var xmlhttp = new XMLHttpRequest();
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			document.getElementById(pageId).innerHTML = xmlhttp.responseText;
			if (pageId == 'config')
				generateRangeControls();
			if (pageId == 'dashboard') {
				loadPage("annunciator");

				// load config for dashboard gauges, then generate them
				var dashConfig = new XMLHttpRequest();
				dashConfig.onreadystatechange = function() {
					if (dashConfig.readyState == 4 && dashConfig.status == 200) {
						var data = JSON.parse(dashConfig.responseText);
						generateGauges(data);
						initWorker();
					}
				};
				dashConfig.open("GET", " config/dashboard.js", true);
				dashConfig.send();
			}
		}
	};
	xmlhttp.open("GET", pageId + ".htm", true);
	xmlhttp.send();
}

// find an attribute of a node by attribute name
function findAttribute(node, attributeName) {
	for (var x = 0; x < node.attributes.length; x++) {
		if (node.attributes[x].nodeName.toUpperCase() == attributeName.toUpperCase()) {
			return node.attributes[x];
		}
	}
	return null;
}

function getCache(name) {
	var target = nodecache[name];
	if (!target) {
		target = document.getElementById(name);
		if (!target) { // id not found, try to find by name
			var namedElements = document.getElementsByName(name);
			if (namedElements && namedElements.length) {
				target = namedElements[0];
			}
		}
		nodecache[name]=target;
	}
	return target;
}

function setNodeValue(name, value) {
	// a bitfield value for annunciator fields
	if (name.indexOf('bitfield') != -1) {
		updateAnnunciatorFields(name, value);
		return;
	}

	// set the meter value (additionally to a text node)
	target = getCache(name + "Meter");
	if (target) {
		target.value = value;
	}
	
	// a normal div/span/input to update
	target = getCache(name);
	if (target) { // found an element, update according to its type
		if (target.nodeName.toUpperCase() == 'DIV' || target.nodeName.toUpperCase() == 'SPAN') {
			target.innerHTML = value;
		} else if (target.nodeName.toUpperCase() == 'INPUT') {
			var type = findAttribute(target, "type");
			if (type && (type.value.toUpperCase() == 'CHECKBOX' || type.value.toUpperCase() == 'RADIO')) {
				target.checked = (value.toUpperCase() == 'TRUE' || value == '1');
			} else {
				target.value = value;
				// find corresponding slider element
				var slider = document.getElementById(name + "Level");
				if (slider) {
					slider.value = value;
				}
			}
		} else if (target.nodeName.toUpperCase() == 'SELECT') {
			selectItemByValue(target, value);
		}
	}
}

// load data from dynamic json and replace values in input fields, div's, gauges
function loadData(pageId) {
	try {
		var xmlhttp = new XMLHttpRequest();
		xmlhttp.onreadystatechange = function() {
			if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
				var data = JSON.parse(xmlhttp.responseText);
				processData(data);
				if (pageId == 'config') {
					refreshThrottleVisualization();
				}
			}
		};
		xmlhttp.open("GET", "config/" + pageId + ".js", true);
		xmlhttp.send();
	} catch (err) {
		alert("unable to retrieve data for page " + pageId);
	}
}

function processData(data) {
	// hide device dependent TR's so they can be shown if configured
	hideDeviceTr();
	for (name in data) {
		var value = data[name];
		if (name.indexOf('device_x') == 0 && value == '1') {
			// it's a device config, update device specific visibility
			setTrVisibility(name, true);
		} else {
			setNodeValue(name, value);
		}
	}
}

// scan through the options of a select input field and activate the one with
// the given value
function selectItemByValue(node, value) {
	for (var i = 0; i < node.options.length; i++) {
		if (node.options[i].value == value) {
			node.selectedIndex = i;
			break;
		}
	}
}

function updateRangeValue(id, source) {
	var val = parseInt(source.value);

	if (val < 0 || isNaN(val))
		val = 0;

	if (id == 'minimumLevel') {
		var max = getIntValue("maximumLevel");
		if (val > max)
			val = max;
	} else if (id == 'maximumLevel') {
		var min = getIntValue("minimumLevel");
		if (val < min)
			val = min;
	} else if (id == 'minimumLevel2') {
		var max = getIntValue("maximumLevel2");
		if (val > max)
			val = max;
	} else if (id == 'maximumLevel2') {
		var min = getIntValue("minimumLevel2");
		if (val < min)
			val = min;
	} else if (id == 'positionRegenMaximum') {
		var regen = getIntValue("positionRegenMinimum");
		if (val > regen)
			val = regen;
	} else if (id == 'positionRegenMinimum') {
		var regen = getIntValue("positionRegenMaximum");
		var fwd = getIntValue("positionForwardStart");
		if (val < regen)
			val = regen;
		if (val > fwd)
			val = fwd;
	} else if (id == 'positionForwardStart') {
		var regen = getIntValue("positionRegenMinimum");
		var half = getIntValue("positionHalfPower");
		if (val < regen)
			val = regen;
		if (val > half)
			val = half;
	} else if (id == 'positionHalfPower') {
		var half = getIntValue("positionForwardStart");
		if (val < half)
			val = half;
	} else if (id == 'minimumRegen') {
		var regen = getIntValue("maximumRegen");
		if (val > regen)
			val = regen;
	} else if (id == 'maximumRegen') {
		var regen = getIntValue("minimumRegen");
		if (val < regen)
			val = regen;
	} else if (id == 'brakeMinimumLevel') {
		var max = getIntValue("brakeMaximumLevel");
		if (val > max)
			val = max;
	} else if (id == 'brakeMaximumLevel') {
		var min = getIntValue("brakeMinimumLevel");
		if (val < min)
			val = min;
	} else if (id == 'brakeMinimumRegen') {
		var max = getIntValue("brakeMaximumRegen");
		if (val > max)
			val = max;
	} else if (id == 'brakeMaximumRegen') {
		var min = getIntValue("brakeMinimumRegen");
		if (val < min)
			val = min;
	}

	document.getElementById(id).value = val;
	source.value = val;
	refreshThrottleVisualization();
}

function refreshThrottleVisualization() {
	if (!canvas) {
		canvas = new ThrottleSettingsCanvas();
	}
	canvas.draw();
}

function getIntValue(id) {
	var node = document.getElementById(id);
	if (node)
		return parseInt(node.value);
	return 0;
}

function generateRangeControls() {
	addRangeControl("minimumLevel", 0, 4095);
	addRangeControl("minimumLevel2", 0, 4095);
	addRangeControl("maximumLevel", 0, 4095);
	addRangeControl("maximumLevel2", 0, 4095);
	addRangeControl("positionRegenMaximum", 0, 100);
	addRangeControl("positionRegenMinimum", 0, 100);
	addRangeControl("positionForwardStart", 0, 100);
	addRangeControl("positionHalfPower", 0, 100);
	addRangeControl("minimumRegen", 0, 100);
	addRangeControl("maximumRegen", 0, 100);
	addRangeControl("creep", 0, 100);
	addRangeControl("brakeMinimumLevel", 0, 4095);
	addRangeControl("brakeMinimumRegen", 0, 100);
	addRangeControl("brakeMaximumLevel", 0, 4095);
	addRangeControl("brakeMaximumRegen", 0, 100);
	addRangeControl("inputCurrent", 1, 50);
}

function addRangeControl(id, min, max) {
	var node = document.getElementById(id + "Span");
	if (node)
		node.innerHTML = "<input id='" + id + "Level' type='range' min='" + min + "' max='" + max + "' onchange=\"updateRangeValue('" + id
				+ "', this);\" onmousemove=\"updateRangeValue('" + id + "', this);\" /><input type='number' id='" + id + "' name='" + id + "' min='"
				+ min + "' max='" + max + "' maxlength='4' size='4' onchange=\"updateRangeValue('" + id + "Level', this);\"/>";
}

// hides rows with device dependent visibility
// (as a pre-requisite to re-enable it)
function hideDeviceTr() {
	tr = document.getElementsByTagName('tr')
	for (i = 0; i < tr.length; i++) {
		var idStr = tr[i].getAttribute('id');
		if (idStr && idStr.indexOf('device_x') != -1) {
			tr[i].style.display = 'none';
		}
	}
}

// shows/hides rows of a table with a certain id value
// (used for device specific parameters)
function setTrVisibility(id, visible) {
	tr = document.getElementsByTagName('tr')
	for (i = 0; i < tr.length; i++) {
		var idStr = tr[i].getAttribute('id');
		if (idStr && idStr.indexOf(id) != -1) {
			if (visible != 0) {
				tr[i].style.display = '';
			} else {
				tr[i].style.display = 'none';
			}
		}
	}
}

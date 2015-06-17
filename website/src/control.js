var intervalId=null;

var canvas; // global throttle canvas object
		
// resize the canvas to fill browser window dynamically
window.addEventListener('resize', resizeThrottleCanvas, false);

function resizeThrottleCanvas() {
	// adjust the width to the page width
	var canvasElement=document.getElementById("throttleCanvas");
	if ( canvasElement ) {
		canvasElement.width = window.innerWidth - 60; // needs to be slightly narrower than the page width
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

	// on the status && dashboard pages, set a 500ms repeating interval to load the data
	if (pageId == 'status' || pageId == 'dashboard') {
		if ( intervalId ) {
			clearInterval(intervalId);
		}
		intervalId = setInterval(function(){loadData(pageId)}, 200);
	} else {
		if (intervalId) {
			clearInterval(intervalId);
			intervalId = null;
		}
		
		if ( pageId == 'config' ) {
			resizeThrottleCanvas();
		}
	}
	loadData(pageId);
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
			if (pageId == 'status') {
				loadPage("annunciator");
			}
			if (pageId == 'dashboard') {
				generateGauges();
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

function setNodeValue(name, value) {
	if (name.indexOf('bitfield') == -1) { // a normal div/span/input to update
		var target = document.getElementById(name);
		
		if (!target) { // id not found, try to find by name
			var namedElements = document.getElementsByName(name);
			if (namedElements && namedElements.length)
				target = namedElements[0];
		}

		if (target) { // found an element, update according to its type
			if (target.nodeName.toUpperCase() == 'DIV' || target.nodeName.toUpperCase() == 'SPAN')
				target.innerHTML = value;
			if (target.nodeName.toUpperCase() == 'INPUT') {
				var type = findAttribute(target, "type");
				if (type && (type.value.toUpperCase() == 'CHECKBOX' || type.value.toUpperCase() == 'RADIO')) {
					target.checked = (value.toUpperCase() == 'TRUE' || value == '1');
				} else {
					target.value = value;
					var slider = document.getElementById(name + "Level"); // find corresponding slider element
					if (slider) {
						slider.value = value;
					}
				}
			}
			if (target.nodeName.toUpperCase() == 'SELECT') {
				selectItemByValue(target, value);
			}
		}
	} else { // an annunciator field of a bitfield value
		updateAnnunciatorFields(name, value);
//		updateAnnunciatorFields(name, Math.round(Math.random() * 0x100000000));
	}
}

// load data from dynamic xml and replace values in input fields, div's, gauges
function loadData(pageId) {
	try {
		var xmlhttp = new XMLHttpRequest();
		xmlhttp.onreadystatechange = function() {
			if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
				hideDeviceTr(); // hide device dependent TR's so they can be shown if configured
				var root = xmlhttp.responseXML.firstChild;
				for (var i = 0; i < root.childNodes.length; i++) {
					var node = root.childNodes[i]; // scan through the nodes
					if (node.nodeType == 1 && node.childNodes[0]) {
						var name = node.nodeName;
						var value = node.childNodes[0].nodeValue;
						
						if (name.indexOf('device_x') == 0 && value == '1') {
							setTrVisibility(name, true); // it's a device config, update device specific visibility
						} else if (pageId == 'dashboard') {
							refreshGaugeValue(name, value);
						} else {
							setNodeValue(name, value);
						}
					}
				}
				if (pageId == 'config') {
					refreshThrottleVisualization();
				}
			}
		};
		xmlhttp.open("GET", pageId + ".xml", true);
		xmlhttp.send();
	} catch (err) {
		alert("unable to retrieve data for page " + pageId);
	}
}

// scan through the options of a select input field and activate the one with the given value
function selectItemByValue(node, value) {
	for (var i = 0; i < node.options.length; i++) {
		if (node.options[i].value === value) {
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

//	var positionRegenMinimum = document.getElementById('positionRegenMinimum');
//	var positionForwardStart = document.getElementById('positionForwardStart');
//	if ( positionRegenMinimum && positionForwardStart ) {
//		updateThrottleGaugeHighlights(positionRegenMinimum.value, positionForwardStart.value);
//	}
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
}

function addRangeControl(id, min, max) {
	var node = document.getElementById(id + "Span");
	if (node)
		node.innerHTML = "<input id='"+id+"Level' type='range' min='"+min+"' max='"+max+"' onchange=\"updateRangeValue('"+id+"', this);\" onmousemove=\"updateRangeValue('"+id+"', this);\" /><input type='number' id='"+id+"' name='"+id+"' min='"+min+"' max='"+max+"' maxlength='4' size='4' onchange=\"updateRangeValue('"+id+"Level', this);\"/>";
}

//hides rows with device depandent visibility (as a pre-requisite to re-enable it)
function hideDeviceTr() {
	tr = document.getElementsByTagName('tr')
	for (i = 0; i < tr.length; i++) {
		var idStr = tr[i].getAttribute('id');
		if (idStr && idStr.indexOf('device_x') != -1) {
			tr[i].style.display = 'none';
		}
	}
}

// shows/hides rows of a table with a certain id value (used for device specific parameters)
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
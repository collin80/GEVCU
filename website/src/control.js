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
		intervalId = setInterval(function(){loadData(pageId)}, 500);
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

// load data from dynamic xml and replace values in div's
function loadData(pageId) {
	try {
		var xmlhttp = new XMLHttpRequest();
		xmlhttp.onreadystatechange = function() {
			if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
				var root = xmlhttp.responseXML.firstChild;
				for (var i = 0; i < root.childNodes.length; i++) {
					var node = root.childNodes[i]; // scan through the nodes
					if (node.nodeType == 1 && node.childNodes[0]) {
						var name = node.nodeName;
						var value = node.childNodes[0].nodeValue;
						refreshGaugeValue(name, value);
						if (name.indexOf('bitfield') == -1) { // a normal div/span to update
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
									target.value = value;
									var slider = document.getElementById(name + "Level");
									if (slider)
										slider.value = value;
								}
								if (target.nodeName.toUpperCase() == 'SELECT') {
									selectItemByValue(target, value);
								}
							}
						} else { // an annunciator field of a bitfield value
							updateAnnunciatorFields(name, value);
	//						updateAnnunciatorFields(name, Math.round(Math.random() * 100000));
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
		//
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
//	if (val > 1000)
//		val = 1000;
	if (val < 0 || isNaN(val))
		val = 0;
	
	if (id == 'throttleMin1') {
		var max = getIntValue("throttleMax1");
		if (val > max)
			val = max;
	} else if (id == 'throttleMax1') {
		var min = getIntValue("throttleMin1");
		if (val < min)
			val = min;
	} else if (id == 'throttleMin2') {
		var max = getIntValue("throttleMax2");
		if (val > max)
			val = max;
	} else if (id == 'throttleMax2') {
		var min = getIntValue("throttleMin2");
		if (val < min)
			val = min;
	} else if (id == 'throttleRegenMax') {
		var regen = getIntValue("throttleRegenMin");
		if (val > regen)
			val = regen;
	} else if (id == 'throttleRegenMin') {
		var regen = getIntValue("throttleRegenMax");
		var fwd = getIntValue("throttleFwd");
		if (val < regen)
			val = regen;
		if (val > fwd)
			val = fwd;
	} else if (id == 'throttleFwd') {
		var regen = getIntValue("throttleRegenMin");
		var map = getIntValue("throttleMap");
		if (val < regen)
			val = regen;
		if (val > map)
			val = map;
	} else if (id == 'throttleMap') {
		var map = getIntValue("throttleFwd");
		if (val < map)
			val = map;
	} else if (id == 'throttleMinRegen') {
		var regen = getIntValue("throttleMaxRegen");
		if (val > regen)
			val = regen;
	} else if (id == 'throttleMaxRegen') {
		var regen = getIntValue("throttleMinRegen");
		if (val < regen)
			val = regen;
	} else if (id == 'brakeMin') {
		var max = getIntValue("brakeMax");
		if (val > max)
			val = max;
	} else if (id == 'brakeMax') {
		var min = getIntValue("brakeMin");
		if (val < min)
			val = min;
	} else if (id == 'brakeMinRegen') {
		var max = getIntValue("brakeMaxRegen");
		if (val > max)
			val = max;
	} else if (id == 'brakeMaxRegen') {
		var min = getIntValue("brakeMinRegen");
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
	
	var throttleRegenMin = document.getElementById('throttleRegenMin');
	var throttleFwd = document.getElementById('throttleFwd');
	if ( throttleRegenMin && throttleFwd ) {
		updateThrottleGaugeHighlights(throttleRegenMin.value, throttleFwd.value);
	}
}

function getIntValue(id) {
	var node = document.getElementById(id);
	if (node)
		return parseInt(node.value);
	return 0;
}

function generateRangeControls() {
	addRangeControl("throttleMin1", 0, 4095);
	addRangeControl("throttleMin2", 0, 4095);
	addRangeControl("throttleMax1", 0, 4095);
	addRangeControl("throttleMax2", 0, 4095);
	addRangeControl("throttleRegenMax", 0, 100);
	addRangeControl("throttleRegenMin", 0, 100);
	addRangeControl("throttleFwd", 0, 100);
	addRangeControl("throttleMap", 0, 100);
	addRangeControl("throttleMinRegen", 0, 100);
	addRangeControl("throttleMaxRegen", 0, 100);
	addRangeControl("throttleCreep", 0, 100);
	addRangeControl("brakeMin", 0, 4095);
	addRangeControl("brakeMinRegen", 0, 100);
	addRangeControl("brakeMax", 0, 4095);
	addRangeControl("brakeMaxRegen", 0, 100);
}

function addRangeControl(id, min, max) {
	var node = document.getElementById(id + "Span");
	node.innerHTML = "<input id='"+id+"Level' type='range' min='"+min+"' max='"+max+"' onchange=\"updateRangeValue('"+id+"', this);\" onmousemove=\"updateRangeValue('"+id+"', this);\" /><input type='number' id='"+id+"' name='"+id+"' min='"+min+"' max='"+max+"' maxlength='4' size='4' onchange=\"updateRangeValue('"+id+"Level', this);\"/>";
}
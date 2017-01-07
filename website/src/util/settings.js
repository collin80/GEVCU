var canvas; // global throttle canvas object

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
	addRangeControl("creep", 0, 30);
	addRangeControl("creepSpeed", 100, 2000);
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
		var idStr = tr[i].id;
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
		var idStr = tr[i].id;
		if (idStr && idStr.indexOf(id) != -1) {
			if (visible != 0) {
				tr[i].style.display = '';
			} else {
				tr[i].style.display = 'none';
			}
		}
	}
}

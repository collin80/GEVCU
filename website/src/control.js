var intervalId=null;

function showTab(pageId) {
	// show the correct dif and hide the others
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

	// on the status page, set a 500ms repeating interval to load the data
	if (pageId == 'status' && intervalId == null) {
		intervalId = setInterval(function(){loadData(pageId)}, 500);
	} else {
		if (intervalId) {
			clearInterval(intervalId);
			intervalId = null;
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
			if (pageId == 'status')
				loadPage("annunciator");
/*			loadData(pageId);*/
		}
	};
	xmlhttp.open("GET", pageId + ".htm", true);
	xmlhttp.send();
}

// load data from dynamic xml and replace values in div's
function loadData(pageId) {
	var xmlhttp = new XMLHttpRequest();
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var root = xmlhttp.responseXML.firstChild;
			for (var i = 0; i < root.childNodes.length; i++) {
				var node = root.childNodes[i];
				if (node.nodeType == 1 && node.childNodes[0]) {
					var name = node.nodeName;
					var value = node.childNodes[0].nodeValue;
					if (name.indexOf('bitfield') == -1) {
						var target = document.getElementById(name);
						if (target)
							target.innerHTML = value;
					} else {
						updateAnnunciatorFields(name, value);
					}
				}
			}
		}
	};
	xmlhttp.open("GET", pageId + ".xml", true);
	xmlhttp.send();
}

function updateRangeValue(id, source) {
	var val = source.value;
	if (val > 1000)
		val = 1000;
	if (val < 0 || isNaN(val))
		val = 0;
	
	if (id == 'throttleMin1') {
		var node = document.getElementById("throttleMax1");
		if (node && val > node.value)
			val = node.value;
	}
	if (id == 'throttleMax1') {
		var node = document.getElementById("throttleMin1");
		if (node && val < node.value)
			val = node.value;
	}
	if (id == 'throttleMin2') {
		var node = document.getElementById("throttleMax2");
		if (node && val > node.value)
			val = node.value;
	}
	if (id == 'throttleMax2') {
		var node = document.getElementById("throttleMin2");
		if (node && val < node.value)
			val = node.value;
	}
	if (id == 'throttleRegen') {
		var node = document.getElementById("throttleFwd");
		if (node && val > node.value)
			val = node.value;
	}
	if (id == 'throttleFwd') {
		var regen = document.getElementById("throttleRegen");
		var map = document.getElementById("throttleMap");
		if (regen && map && val < regen.value)
			val = regen.value;
		if (val > map.value)
			val = map.value;
	}
	if (id == 'throttleMap') {
		var node = document.getElementById("throttleFwd");
		if (node && val < node.value)
			val = node.value;
	}
		
	document.getElementById(id).value = val;
	source.value = val;
}

function generateRangeControls() {
	addRangeControl("throttleMin1");
	addRangeControl("throttleMin2");
	addRangeControl("throttleMax1");
	addRangeControl("throttleMax2");
	addRangeControl("throttleRegen");
	addRangeControl("throttleFwd");
	addRangeControl("throttleMap");
	addRangeControl("throttleMaxRegen");
	addRangeControl("brakeMin");
	addRangeControl("brakeMinRegen");
	addRangeControl("brakeMax");
	addRangeControl("brakeMaxRegen");
}

function addRangeControl(id) {
	var node = document.getElementById(id + "Span");
	node.innerHTML = "<input id='"+id+"Level' name='"+id+"Level' type='range' min='0' max='1000' onchange=\"updateRangeValue('"+id+"', this);\" onmousemove=\"updateRangeValue('"+id+"', this);\" /><input type='number' id='"+id+"' min='0' max='1000' maxlength='4' size='4' onchange=\"updateRangeValue('"+id+"Level', this);\"/>";
}
var nodecache = new Array();

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
	if (pageId == 'control') {
		resizeThrottleCanvas();
	}
	loadData(pageId);
}

//lazy load of page, replaces content of div with id==<pageId> with
//content of remote file <pageId>.htm
function loadPage(pageId) {
	ajaxCall(pageId + ".htm", function (response) {
		document.getElementById(pageId).innerHTML = response;
		if (pageId == 'motor') {
			generateRangeControls();
			resizeThrottleCanvas();
		}
		if (pageId == 'dashboard') {
			loadPage("annunciator");

			// load config for dashboard gauges, then generate them
			ajaxCall("config/dashboard.js", function (response) {
				var data = JSON.parse(response);
				generateGauges(data);
				initWorker();
			});
		}
		if (pageId == 'annunciator') {
			foldAnnunciator(false);
		}
	});
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
	// a normal div/span/input to update
	target = getCache(name);
	if (target) { // found an element, update according to its type
		if (target.nodeName.toUpperCase() == 'DIV' || target.nodeName.toUpperCase() == 'SPAN') {
			target.innerHTML = value;
		} else if (target.nodeName.toUpperCase() == 'INPUT') {
			if (target.type && (target.type.toUpperCase() == 'CHECKBOX' || target.type.toUpperCase() == 'RADIO')) {
				target.checked = value;
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
	ajaxCall('config/' + pageId + '.js', function(response) {
		var data = JSON.parse(response);

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
		
		if (pageId == 'control') {
			refreshThrottleVisualization();
		}
	});
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

function ajaxCall(url, callback) {
	try {
		var xmlhttp = new XMLHttpRequest();
		xmlhttp.onreadystatechange = function() { 
			if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
				callback(xmlhttp.responseText);
			}
		};
		xmlhttp.open("GET", url, true);
		xmlhttp.send();
	} catch (err) {
		alert("unable to retrieve data for page " + pageId);
	}
}
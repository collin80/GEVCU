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
			loadData(pageId);
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
			var statusRoot = xmlhttp.responseXML.firstChild;
			for (var i = 0; i < statusRoot.childNodes.length; i++) {
				var node = statusRoot.childNodes[i];
				if (node.nodeType == 1) { /* Element */
					var target = document.getElementById(node.nodeName);
					if (target && node.childNodes[0])
						target.innerHTML = node.childNodes[0].nodeValue;
				}
			}
		}
	};
	xmlhttp.open("GET", pageId + ".xml", true);
	xmlhttp.send();
}

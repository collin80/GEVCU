var handler; // a background worker to receive and process data

function showTab(pageId) {
	// show the correct div and hide the others
	alertify.set('notifier', 'position', 'top-right');
	// add an event listener so sounds can get loaded on mobile devices after user interaction
	window.addEventListener('keydown', removeBehaviorsRestrictions);
	window.addEventListener('mousedown', removeBehaviorsRestrictions);
	window.addEventListener('touchstart', removeBehaviorsRestrictions);
	startHandler();
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

function initHandler() {
	for (var name in Gauge.Collection) {
		if (name != "get") {
			var config = Gauge.Collection.get(name).config;

			for (var i = 0; i < config.values.length; i++) {
				handler.postMessage({
					cmd : 'init',
					config : {
						id : config.values[i].id,
						minValue : config.values[i].minValue,
						maxValue : config.values[i].maxValue,
						startValue : config.values[i].startValue,
						offset : config.values[i].offset,
						angle : config.values[i].angle,
						ccw : config.values[i].ccw,
						range : config.values[i].range,
						animation : config.animation,
					}
				});
			}
		}
	}
}

function startHandler() {
	if (typeof (handler) == "undefined") {
		handler = new PseudoWorker();
		// handler = new Worker("worker/handler.js");
		handler.onmessage = this.processHandlerMessage;
	}
	handler.postMessage({ cmd : 'start' });
}

function stopHandler() {
	if (typeof (handler) != "undefined") {
		handler.postMessage({ cmd : 'stop' });
		handler = undefined;
	}
}

this.processHandlerMessage = function(event) {
	var data = event.data;
	if (data.dial) {
		var dial = data.dial;
		var gauge = Gauge.Collection.get(dial.id);
		if (gauge) {
			if (dial.angle != undefined) {
				gauge.drawNeedle(dial.id, dial.angle);
				gauge.drawArc(dial.id, dial.arcStart, dial.arcEnd);
			} else if (dial.text != undefined) {
				gauge.drawValue(dial.id, dial.text);
			}
		}
	} else if (data.node) {
		var name = data.node.name;
		var value = data.node.value;

		// a bitfield value for annunciator fields
		if (name.indexOf('bitfield') != -1) {
			updateAnnunciatorFields(name, value);
			return;
		}

		// set the meter value (additionally to a text node)
		var target = getCache(name + "Meter");
		if (target) {
			target.value = value;
		}

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
		var idStr = div[i].id;
		if (idStr && idStr.indexOf('state_') != -1) {
			if (idStr.indexOf('_' + state + '_') != -1) {
				div[i].style.display = '';
			} else {
				div[i].style.display = 'none';
			}
		}
	}
}

// send message via handler to websocket
function sendMsg(message) {
	handler.postMessage({ cmd: 'message', message: message });
}

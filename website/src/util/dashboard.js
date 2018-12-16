var dashboard = dashboard || {};

(function() {
	var webSocketWorker;

	dashboard.activate = activate;
	dashboard.hideStateDivs = hideStateDivs;

	function activate() {
		// show the correct div and hide the others
		// add an event listener so sounds can get loaded on mobile devices
		// after user interaction
		window.addEventListener('keydown', removeBehaviorsRestrictions);
		window.addEventListener('mousedown', removeBehaviorsRestrictions);
		window.addEventListener('touchstart', removeBehaviorsRestrictions);
		startWebSocketWorker();
	}

	// on most mobile browsers sounds can only be loaded during a user
	// interaction (stupid !)
	function removeBehaviorsRestrictions() {
		soundError.load();
		soundWarn.load();
		soundInfo.load();
		window.removeEventListener('keydown', removeBehaviorsRestrictions);
		window.removeEventListener('mousedown', removeBehaviorsRestrictions);
		window.removeEventListener('touchstart', removeBehaviorsRestrictions);
	}

	function startWebSocketWorker() {
		if (typeof (webSocketWorker) == "undefined") {
			webSocketWorker = new Worker("worker/webSocket.js");
			webSocketWorker.onmessage = function(event) {
				processWebsocketMessage(event.data)
			};
		}
		webSocketWorker.postMessage({
			cmd : 'start'
		});
	}

	function stopWebSocketWorker() {
		if (webSocketWorker) {
			webSocketWorker.postMessage({
				cmd : 'stop'
			});
			webSocketWorker = undefined;
		}
	}

	function processWebsocketMessage(message) {
		for (name in message) {
			var data = message[name];

			var dial = Gauge.dials[name];
			if (dial) {
				dial.setValue(data);
			} else if (name == 'limits') {
				for (limitName in data) {
					var limit = data[limitName];
					dial = Gauge.dials[limitName];
					if (dial) {
						dial.setLimits(limit.min, limit.max);
					}
				}
			} else if (name.indexOf('bitfield') != -1) { // a bitfield value for annunciator fields
				updateAnnunciatorFields(name, data);
			} else if (name == 'systemState') {
				updateSystemState(data);
			} else if (name == 'logMessage') {
				log(data.level, data.message);
			} else {
				// set the meter value (additionally to a text node)
				var target = getCache(name + "Meter");
				if (target) {
					target.value = data;
				}
				setNodeValue(name, data);
			}
		}
	}
	
	function hideStateDivs() {
		var div = document.getElementsByTagName('div')
		for (i = 0; i < div.length; i++) {
			var idStr = div[i].id;
			if (idStr && idStr.indexOf('state_') != -1) {
				div[i].style.display = 'none';
			}
		}
	}
	
	function log(level, message) {
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

	// send message to ichip via websocket
	function sendMsg(message) {
		webSocketWorker.postMessage(event.data);
	}
})();

var socket = null;
var heartbeat = null;
var timestampLastReception = 0;

onmessage = function(event) {
	switch (event.data.cmd) {
	case 'start':
		console.log('webSocket start');
		openWebSocket();
		break;
	case 'stop':
		console.log('webSocket stop');
		closeWebSocket();
		break;
	case 'message':
		socket.send(event.data.message);
		break;
	default:
		console.log("webSocket: received unknown command from parent: " + event.data.cmd);
	}
}

function openWebSocket() {
	if (heartbeat) {
		clearInterval(heartbeat);
		heartbeat = null;
	}
	if (socket == null) {
		var url = "ws://" + location.hostname + ":2000";
		console.log('webSocket: opening web socket connection ' + url);
		socket = new WebSocket(url);

		// Log errors and try to reconnect
		socket.onerror = function(error) {
			console.log('webSocket error: ' + error);
			closeWebSocket();
			setTimeout(function() {openWebSocket();}, 5000);
			timestampLastReception = new Date().getTime();
		};

		// process messages from the server
		socket.onmessage = function(message) {
			timestampLastReception = new Date().getTime();
			var data = JSON.parse(message.data);
			postMessage(data);
		};
	}
	timestampLastReception = new Date().getTime();
	heartbeat = setInterval(checkConnection, 10000);
}

function closeWebSocket() {
	console.log('webSocket: closing web socket connection');
	if (socket) {
		socket.close();
		socket = null;
	}
}

function checkConnection() {
	if (timestampLastReception + 25000 < new Date().getTime()) {
		postMessage({"logMessage": {"level": "WARNING","message": "websocket connection failure, trying to re-connect"}});
		closeWebSocket();
		setTimeout(function() {openWebSocket();}, 1000);
		timestampPong = new Date().getTime();
	}
}
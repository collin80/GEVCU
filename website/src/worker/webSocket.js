var socket = null;

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
	if(socket == null) {
		var url = "ws://" + location.hostname + ":2000";
		console.log('webSocket: opening web socket connection ' + url);
		socket = new WebSocket(url);
	
		// Log errors and try to reconnect
		socket.onerror = function(error) {
			console.log('webSocket error: ' + error);
			closeWebSocket();
			openWebSocket();
		};
	
		// process messages from the server
		socket.onmessage = function(message) {
			var data = JSON.parse(message.data);
			postMessage(data);
		};
	}
}

function closeWebSocket() {
	console.log('webSocket: closing web socket connection');
	if (socket) {
		socket = null;
		socket.close();
	}
}

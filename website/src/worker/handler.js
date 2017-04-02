// this is supposed to be a worker which spawns subworkers - but only FF and Edge currently support subworkers
// the following glue code makes it a pseudo worker in the same main thread but keeps the implementation almost the same
var parentOnmessage;
var PseudoWorker = function() {
	var initialized = false;
		
	this.postMessage = function(msg) {
		self.onmessage({data: msg});
		if (!initialized) {
			self.parentOnmessage = this.onmessage;
			initialized = true;
		}
	} 
}

this.postMessage = function(data) {
	parentOnmessage({data: data});
}
// end glue code - after this line the normal worker implementation follows


var webSocketWorker;
var gaugeDialWorker = [];

this.onmessage = function(event) {
	switch (event.data.cmd) {
	case 'start':
		console.log('handler start');
		startWebSocketWorker();
		break;
	case 'stop':
		console.log('handler stop');
		stopWebSocketWorker();
		break;
	case 'init':
		createGaugeDialWorker(event.data.config);
		break;
	case 'message':
		webSocketWorker.postMessage(event.data);
		break;
	default:
		console.log("handler: received unknown command from parent: " + event.data.cmd);
	}
}

function startWebSocketWorker() {
	if(typeof(webSocketWorker) == "undefined") {
		webSocketWorker = new Worker("worker/webSocket.js");

		webSocketWorker.onmessage = function(event) {
			for (name in event.data) {
				var worker = gaugeDialWorker[name];
				if(worker) {
					worker.postMessage(event.data[name]);
				} else {
					postMessage({node: {name: name, value: event.data[name]}});
				}
			}
		};
	}
	webSocketWorker.postMessage({cmd: 'start'});
}

function createGaugeDialWorker(config) {
	var worker = new Worker("worker/gaugeDial.js");
	worker.onmessage = function(event) {
		postMessage({dial: event.data});
	}
	worker.postMessage({config: config});

	gaugeDialWorker[config.id.substring(0, config.id.length - 10)] = worker;
	return worker;
}

function stopWebSocketWorker() {
	if(typeof(webSocketWorker) != "undefined") {
		webSocketWorker.postMessage({cmd: 'stop'});
	}
}

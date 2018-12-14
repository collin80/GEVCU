function foldBatteries(open) {
	var batteries = document.getElementById('batteryInfo');
	if (batteries) {
		var display = '';
		if(typeof(open) == "undefined") {
			var oldDisplay = batteries.style.display;
			display = (oldDisplay == 'none' ? '' : 'none');
		} else {
			display = (open ? '' : 'none'); 
		}
		batteries.style.display = display;
	}
}
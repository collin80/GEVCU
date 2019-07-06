function ThrottleSettingsCanvas() {
	// Just hard code theses Ids. HTML just needs to be kept in sync
    this.canvasId = "throttleCanvas";
    this.throttleRegenMaxId = "positionRegenMaximum";
    this.throttleRegenMinId = "positionRegenMinimum";
    this.throttleFwdId = "positionForwardStart";
    this.throttleMapId = "positionHalfPower";
    this.throttleMinRegenId = "minimumRegen";
    this.throttleMaxRegenId = "maximumRegen";
    this.throttleCreepId = "creepLevel";
    
    // default to white lines/text
    this.lineColor = "#FFFFFF";
}
 
ThrottleSettingsCanvas.prototype.draw = function() {
	// get canvas size
	var canvas=document.getElementById(this.canvasId);
	if ( !canvas ) {
		return; // no canvas found by this id
	}
	this.context=canvas.getContext("2d");
    this.height=canvas.height;
    this.width=canvas.width;

	this.xOffset = 150;
	this.xRightOffset = 25;
	this.yOffset = this.height - 40;
	this.yTopOffset = 15;
	this.xLength = this.width - this.xOffset - this.xRightOffset;
	this.xZeroOffset = this.xOffset + this.xLength;
	this.yLength = (this.yOffset - this.yTopOffset)/2;
	this.yZeroOffset = this.yOffset - this.yLength;
	
	// reset
	this.context.clearRect(0,0, this.width, this.height);
	
	this.drawXAxis();
	this.drawYAxis();
	this.drawCoastArea();
	this.drawForwardLines();
	this.drawRegenLines();
	this.drawCreepLines();
};

ThrottleSettingsCanvas.prototype.drawXAxis = function(){
	labelOffset = 15;
	
	this.context.save();
	
	// 0% line
	this.context.beginPath();
	this.context.strokeStyle=this.lineColor;
	this.context.lineWidth=1;
	this.context.moveTo(this.xOffset,                this.yZeroOffset);
	this.context.lineTo(this.xOffset + this.xLength, this.yZeroOffset);
	this.context.stroke();
	
	// 50% lines
	this.context.beginPath();
	this.context.strokeStyle="green";
	this.context.lineWidth=1;
	this.context.moveTo(this.xOffset,                this.yZeroOffset - this.yLength/2);
	this.context.lineTo(this.xOffset + this.xLength, this.yZeroOffset - this.yLength/2);
	this.context.stroke();
	
	this.context.beginPath();
	this.context.strokeStyle="red";
	this.context.lineWidth=1;
	this.context.moveTo(this.xOffset,                this.yZeroOffset + this.yLength/2);
	this.context.lineTo(this.xOffset + this.xLength, this.yZeroOffset + this.yLength/2);
	this.context.stroke();
	
	// X axis
	this.context.beginPath();
	this.context.strokeStyle=this.lineColor;
	this.context.lineWidth=3;
  	this.context.moveTo(this.xOffset,                this.yOffset);
	this.context.lineTo(this.xOffset + this.xLength, this.yOffset);
	this.context.stroke();
	
	// scale
	this.context.font="12px Arial";
	this.context.fillStyle=this.lineColor;
	this.context.fillText("0%", this.xOffset-10, this.yOffset + labelOffset);
	this.context.fillText("25%", this.xOffset-10 + (this.xLength/4), this.yOffset + labelOffset);
	this.context.fillText("50%", this.xOffset-10 + (this.xLength/2), this.yOffset + labelOffset);
	this.context.fillText("75%", this.xOffset-10 + (this.xLength*3/4), this.yOffset + labelOffset);
	this.context.fillText("100%", this.xOffset-10 + this.xLength, this.yOffset + labelOffset);
	
	this.context.font="14px Arial";
	this.context.fillText("Throttle level",  this.width/2, this.height - 5);
	
	this.context.restore();
}

ThrottleSettingsCanvas.prototype.drawYAxis = function() {
	labelOffset = 10;
	labelHeightOffset = 3;


	// axis
	this.context.beginPath();
	this.context.lineWidth=3;
	this.context.strokeStyle=this.lineColor;
  	this.context.moveTo(this.xOffset, this.yOffset + 2); // +2 to overlap x line
	this.context.lineTo(this.xOffset, this.yOffset-this.yLength*2 - 5); // -5 to go past the 100% a bit
	this.context.stroke();
	
	// scale
	this.context.save();
	this.context.textAlign="end";
	this.context.font="12px Arial";
	this.context.fillStyle=this.lineColor;
	this.context.fillText("-100%", this.xOffset-labelOffset, this.yOffset); // keep this higher for looks
	this.context.fillText("-50%", this.xOffset-labelOffset, this.yOffset - this.yLength/2 + labelHeightOffset);
	this.context.fillText("0%", this.xOffset-labelOffset, this.yOffset - this.yLength + labelHeightOffset);
	this.context.fillText("50%", this.xOffset-labelOffset, this.yOffset - this.yLength*3/2 + labelHeightOffset);
	this.context.fillText("100%", this.xOffset-labelOffset, this.yOffset - this.yLength*2 + labelHeightOffset);
	this.context.restore();
	
	this.context.font="14px Arial";
	this.context.fillStyle="green";
	this.context.fillText("Motor",  5, this.height/2 - 10);
	this.context.fillStyle=this.lineColor;
	this.context.fillText(" / ",  40, this.height/2 - 10);
	this.context.fillStyle="red";
	this.context.fillText("Regen",  50, this.height/2 - 10);
	this.context.fillStyle=this.lineColor;
	this.context.fillText("Power",  5, this.height/2 + 10);
}

ThrottleSettingsCanvas.prototype.drawForwardLines = function() {
	// get percentages from form
	throttleFwd = document.getElementById(this.throttleFwdId);
	minPercent = throttleFwd.value/100;
	throttleMap = document.getElementById(this.throttleMapId);
	halfPercent = throttleMap.value/100;
	fullPercent = 1;
	
	// gradient for forward throttle
	var grd=this.context.createLinearGradient(this.xOffset + this.xLength, this.yOffset, this.xOffset, this.yOffset);
	grd.addColorStop(0,"green");
	grd.addColorStop(1,"transparent");
	this.context.fillStyle=grd;
	this.context.beginPath();
	this.context.moveTo(this.xOffset + this.xLength*minPercent, this.yZeroOffset - this.yLength*0 - 1); // -1 to not overwrite the X axis
	this.context.lineTo(this.xOffset + this.xLength*halfPercent, this.yZeroOffset - this.yLength*0.5);
	this.context.lineTo(this.xOffset + this.xLength*fullPercent, this.yZeroOffset - this.yLength*1 - 1);
	this.context.lineTo(this.xOffset + this.xLength, this.yZeroOffset - 1);
	this.context.closePath();
	this.context.fill();
	
	// label
	this.context.save();
	this.context.font="14px Arial";
	this.context.textAlign="center";
	this.context.fillStyle=this.lineColor;
	this.context.fillText("Forward", this.xOffset + this.xLength*minPercent + (this.xLength-this.xLength*minPercent)*3/4, this.yZeroOffset - this.yLength*0.25);
	this.context.restore();
	
	// the dots
	this.drawDot("green", minPercent, 0);
	this.drawDot("green", halfPercent, 0.5);
	this.drawDot("green", fullPercent, 1);
	
	// Lines between the dots
	this.context.beginPath();
	this.context.lineWidth=5;
	this.context.strokeStyle="green";
	this.context.moveTo(this.xOffset + this.xLength*minPercent, this.yZeroOffset - this.yLength*0);
	this.context.lineTo(this.xOffset + this.xLength*halfPercent, this.yZeroOffset - this.yLength*0.5);
	this.context.lineTo(this.xOffset + this.xLength*fullPercent, this.yZeroOffset - this.yLength*1);
	this.context.stroke();
}

ThrottleSettingsCanvas.prototype.drawRegenLines = function() {
	// get percentages from form
	throttleMinRegen = document.getElementById(this.throttleMinRegenId); // percent torque
	minPercent = throttleMinRegen.value/-100; 
	throttleRegenMin = document.getElementById(this.throttleRegenMinId); // throttle pos
	minThrottlePos = throttleRegenMin.value/100;

	throttleMaxRegen = document.getElementById(this.throttleMaxRegenId); // percent torque
	maxPercent = throttleMaxRegen.value/-100;
	throttleRegenMax = document.getElementById(this.throttleRegenMaxId); // throttle pos
	maxThrottlePos = throttleRegenMax.value/100;
	
	// gradient for regen (two directions)
	// max to min/start
	var grd=this.context.createLinearGradient(this.xOffset + this.xLength*maxThrottlePos, this.yOffset, this.xOffset + this.xLength*minThrottlePos, this.yOffset);
	grd.addColorStop(0,"red");
	grd.addColorStop(1,"transparent");
	this.context.fillStyle=grd;
	this.context.beginPath();
	this.context.moveTo(this.xOffset + this.xLength*minThrottlePos, this.yZeroOffset);
	this.context.lineTo(this.xOffset + this.xLength*minThrottlePos, this.yZeroOffset - this.yLength*minPercent + 1); // +1 to not overwrite the X axis
	this.context.lineTo(this.xOffset + this.xLength*maxThrottlePos, this.yZeroOffset - this.yLength*maxPercent);
	this.context.lineTo(this.xOffset + this.xLength*maxThrottlePos, this.yZeroOffset);
	this.context.closePath();
	this.context.fill();
	
	// max to zero
	grd=this.context.createLinearGradient(this.xOffset + this.xLength*maxThrottlePos, this.yOffset, this.xOffset, this.yOffset);
	grd.addColorStop(0,"red");
	grd.addColorStop(1,"transparent");
	this.context.fillStyle=grd;
	this.context.beginPath();
	this.context.moveTo(this.xOffset + this.xLength*maxThrottlePos, this.yZeroOffset);
	this.context.lineTo(this.xOffset + this.xLength*maxThrottlePos, this.yZeroOffset - this.yLength*maxPercent);
	this.context.lineTo(this.xOffset, this.yZeroOffset + 1);
	this.context.closePath();
	this.context.fill();
	
	// label
	this.context.save();
	this.context.font="14px Arial";
	this.context.textAlign="center";
	this.context.fillStyle=this.lineColor;
	this.context.fillText("Regen", this.xOffset + this.xLength*maxThrottlePos + this.xLength*minThrottlePos/4 , this.yZeroOffset - this.yLength*maxPercent*0.4);
	this.context.restore();
	
	
    // the dots
	this.drawDot("red", 0, 0);
	this.drawDot("red", minThrottlePos, 0);
	this.drawDot("red", minThrottlePos, minPercent);
	this.drawDot("red", maxThrottlePos, maxPercent);
	
	// Lines between the dots
	this.context.beginPath();
	this.context.lineWidth=5;
	this.context.strokeStyle="red";
	this.context.moveTo(this.xOffset + this.xLength*minThrottlePos, this.yZeroOffset);
	this.context.lineTo(this.xOffset + this.xLength*minThrottlePos, this.yZeroOffset - this.yLength*minPercent - 1); // -1 to not overwrite the X axis
	this.context.lineTo(this.xOffset + this.xLength*maxThrottlePos, this.yZeroOffset - this.yLength*maxPercent);
	this.context.lineTo(this.xOffset, this.yZeroOffset - 1);
	this.context.stroke();
	
}

ThrottleSettingsCanvas.prototype.drawCreepLines = function() {
	// get percentages from form
	throttleCreep = document.getElementById(this.throttleCreepId);
	creepPercent = throttleCreep.value/100;

	if ( creepPercent > 0 ) {
		// the dot
		this.drawDot("green", 0, creepPercent);
	
		// Lines between the dots
		this.context.beginPath();
		this.context.lineWidth=5;
		this.context.strokeStyle="green";
		this.context.moveTo(this.xOffset, this.yZeroOffset);
		this.context.lineTo(this.xOffset, this.yZeroOffset - this.yLength*creepPercent);
		this.context.stroke();
	}
}

ThrottleSettingsCanvas.prototype.drawCoastArea = function() {
	// get percentages from form
	throttleFwd = document.getElementById(this.throttleFwdId);
	minThrottlePercent = throttleFwd.value/100;
	throttleRegenMin = document.getElementById(this.throttleRegenMinId); // throttle pos
	minRegenPercent = throttleRegenMin.value/100;
	
	height = this.yLength*0.45;
	width = this.xLength*minThrottlePercent-this.xLength*minRegenPercent;
	
	// gradient above the X axis
	var grd=this.context.createLinearGradient(this.xOffset, this.yZeroOffset, this.xOffset, this.yZeroOffset-height);
	grd.addColorStop(0,"yellow");
	grd.addColorStop(1,"transparent");
	this.context.fillStyle=grd;
	this.context.fillRect(this.xOffset + this.xLength*minRegenPercent ,this.yZeroOffset-height, width, height);
	
	// and below
	grd=this.context.createLinearGradient(this.xOffset, this.yZeroOffset, this.xOffset, this.yZeroOffset+height);
	grd.addColorStop(0,"yellow");
	grd.addColorStop(1,"transparent");
	this.context.fillStyle=grd;
	this.context.fillRect(this.xOffset + this.xLength*minRegenPercent ,this.yZeroOffset, width, height);
	
	if ( width > 25 ) {
		this.context.save();
		this.context.font="14px Arial";
		this.context.fillStyle=this.lineColor;
		this.context.textAlign="center";
		degrees = -90;
		if ( width < 60 ) {
 			this.context.translate(this.xOffset + this.xLength*minRegenPercent+width/2+3, this.yZeroOffset);
 			this.context.rotate(degrees*Math.PI/180);
 			this.context.fillText("coast", 0,0);	
		} else {
			this.context.fillText("coast", this.xOffset + this.xLength*minRegenPercent+width/2, this.yZeroOffset);
		}
		this.context.restore();
	}
	
}



ThrottleSettingsCanvas.prototype.drawDot = function(color, throttlePercent, powerPercent) {

	x = this.xOffset + this.xLength*throttlePercent;
	y = this.yZeroOffset - this.yLength*powerPercent;
	radius = 10;
	this.context.beginPath();
	this.context.arc(x,y,radius,0,2*Math.PI);
	//this.context.stroke(); //of we want a border
	this.context.fillStyle=color;
	this.context.fill();
}
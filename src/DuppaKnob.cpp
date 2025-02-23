//
//  DuppaKnob.cpp
//  carradio
//
//  Created by Vincent Moscaritolo on 5/17/22.
//

#include "DuppaKnob.hpp"

#include "ErrorMgr.hpp"



DuppaKnob::DuppaKnob(){
	_isSetup = false;
	_currentColor = RGB::Black;
	_brightness = 1.0;
	_deviceAddress = -1;
 }

DuppaKnob::~DuppaKnob(){
	stop();
}

bool DuppaKnob::begin() {
	if (_deviceAddress < 0) {
		return false;
	}
	int error;
	return begin(_deviceAddress, error);
}

bool DuppaKnob::begin(uint8_t i2cAddr) {
	int error = 0;
	return begin(i2cAddr, error);
}

bool DuppaKnob::begin(uint8_t i2cAddr, int &error) {
	bool status = false;

	_deviceAddress = i2cAddr;

	uint8_t config = DuppaEncoder::INT_DATA
	| DuppaEncoder::WRAP_DISABLE
	| DuppaEncoder::DIRE_LEFT
	| DuppaEncoder::IPUP_ENABLE
	| DuppaEncoder::RMOD_X1
	| DuppaEncoder::RGB_ENCODER;
	
	// pull the interrupt line low when the knob is pressed or moved
	uint8_t interrupt_config =
		DuppaEncoder::PUSHR
	| 	DuppaEncoder::PUSHP
	| 	DuppaEncoder::RINC
	| 	DuppaEncoder::RDEC;

	status = _duppa.begin(i2cAddr, config, interrupt_config, error);
	if(status){
		_isSetup = true;
	}
	
	return status;
}

void DuppaKnob::stop() {
	if(_isSetup){
		_duppa.stop();
		_isSetup = false;
	}
}

bool DuppaKnob::isConnected() {
	return _isSetup;
}

bool DuppaKnob::updateStatus() {
	uint8_t status = 0;
	return _duppa.updateStatus(status);
}

bool DuppaKnob::updateStatus(uint8_t &statusOut) {
	return _isSetup && _duppa.updateStatus(statusOut);
}

bool DuppaKnob::wasClicked(){
 	return _isSetup && _duppa.wasClicked();
}

bool DuppaKnob::wasDoubleClicked(){
	return _isSetup && _duppa.wasDoubleClicked();
}

bool DuppaKnob::wasMoved( bool &cw){
	return _isSetup && _duppa.wasMoved(cw);
}
 
bool DuppaKnob::isPressed(){
	return _isSetup && _duppa.isPressed();
}
 

bool DuppaKnob::setBrightness(double level){
	
	if(_isSetup){
		_brightness = level > 1.0?1.0:level;
		if(_currentColor != RGB::Black){
			return setColor(_currentColor);
		}
		return true;
	}
 
	return false;
}


bool DuppaKnob::setColor(RGB color){
	return setColor(color.r, color.g, color.b);
}

bool DuppaKnob::setColor(uint8_t red, uint8_t green, uint8_t blue ){
	
	red = red 		* _brightness;
	green = green 	* _brightness;
	blue = blue 	* _brightness;
 	return _duppa.setColor(red, green, blue);
}


bool DuppaKnob::setAntiBounce(uint8_t period){
	return _isSetup && _duppa.setAntiBounce(period);
}

bool DuppaKnob::setDoubleClickTime(uint8_t period){
	return _isSetup && _duppa.setDoubleClickTime(period);
}

//
//  DuppaKnob.hpp
//  carradio
//
//  Created by Vincent Moscaritolo on 5/17/22.
//

#pragma once

#include "EncoderBase.hpp"
#include "DuppaEncoder.hpp"
#include <stdlib.h>

using namespace std;
 
class DuppaKnob : public EncoderBase{
 
public:

	DuppaKnob();
	virtual ~DuppaKnob();

    // EncoderBase interface implementation
    virtual bool begin() override;
    virtual void stop() override;
    virtual bool updateStatus() override;
    virtual bool wasClicked() override;
    virtual bool wasDoubleClicked() override;
    virtual bool wasMoved(bool &clockwise) override;
    virtual bool isPressed() override;
    virtual bool setAntiBounce(uint8_t period) override;
    virtual bool setDoubleClickTime(uint8_t period) override;

    // Additional DuppaKnob specific methods
    bool begin(uint8_t i2cAddr);
    bool begin(uint8_t i2cAddr, int &error);
	bool isConnected();
	bool setColor(uint8_t red, uint8_t green, uint8_t blue);
	bool setColor(RGB color);
	bool setBrightness(double level);
	
private:
	
	bool					_isSetup;
	DuppaEncoder		_duppa;
	
	RGB 					_currentColor;
	double				_brightness;
	int					_deviceAddress;
};

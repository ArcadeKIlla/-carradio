void DisplayMgr::stop() {
	// Turn off LEDs if any
	LEDeventStop();
	
	// If we have a display, clear and release it
	if (_vfd && _vfd->isSetup()) {
		_vfd->clearDisplay();
		_vfd->display();
	}
	
	// Stop any threads or timers if needed
	_isRunning = false;
}

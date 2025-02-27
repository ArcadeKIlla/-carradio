//
//  PiCarMgr.c
//  carradio
//
//  Created by Vincent Moscaritolo on 5/8/22.
//

#include "PiCarMgr.hpp"

#include <iostream>
#include <iostream>
#include <filesystem> // C++17
#include <fstream>
#include <execinfo.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <regex>
#include <limits.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>

#include <sys/types.h>
#include <sys/wait.h>

 
#if defined(__APPLE__)
#else
#include <linux/reboot.h>
#include <sys/reboot.h>

#endif

#include "Utils.hpp"
#include "TimeStamp.hpp"
#include "timespec_util.h"

#pragma clang diagnostic ignored "-Wc99-designator"

using namespace std;
using namespace timestamp;
using namespace nlohmann;
using namespace Utils;

const char* 	PiCarMgr::PiCarMgr_Version = "1.0.1";

// Update the path to use I2C
const char* gpioPath 				= "/dev/gpiochip0";
constexpr uint 	gpio_relay1_line_number	= 26;
#if USE_GPIO_INTERRUPT
constexpr uint 	gpio_int_line_number	= 27;
const char*			 GPIOD_CONSUMER 		=  "gpiod-PiCar";
#endif


constexpr int  pcmrate = 44100;

typedef void * (*THREADFUNCPTR)(void *);

PiCarMgr *PiCarMgr::sharedInstance = NULL;


static void CRASH_Handler()
{
	void *trace_elems[20];
	int trace_elem_count(backtrace( trace_elems, 20 ));
	char **stack_syms(backtrace_symbols( trace_elems, trace_elem_count ));
	for ( int i = 0 ; i < trace_elem_count ; ++i )
	{
		std::cout << stack_syms[i] << "\r\n";
	}
	free( stack_syms );
	
	exit(1);
}

static void sigHandler (int signum) {
	
	printf("%s sigHandler %d\n", TimeStamp(false).logFileString().c_str(), signum);
	
	// ignore hangup
	if(signum == SIGHUP)
		return;
	
	auto picanMgr = PiCarMgr::shared();
	picanMgr->stop();
	exit(0);
}


static int getPidByName(const char *name)
{
	char exe_file[] = "/proc/1000000000/exe";
	char exe_path[256];
	if(NULL == name)
		return -1;
	DIR* dir = opendir("/proc");
	if(NULL == dir)
		return -1;
	struct dirent* de = 0;
	while((de = readdir(dir)) != 0)
	{
		if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
			continue;
		int pid = -1;
		int res = sscanf(de->d_name, "%d", &pid);
		if(res != 1)
			continue;
		snprintf(exe_file, sizeof(exe_file), "/proc/%d/exe", pid);
		ssize_t n = readlink(exe_file, exe_path, sizeof(exe_path));
		if(n == -1)
			continue;
		exe_path[n] = '\0';
		// puts(exe_path);
		if(strstr(exe_path, name) != 0)
		{
			closedir(dir);
			return pid;
		}
	}
	closedir(dir);
	return -1;
}




PiCarMgr * PiCarMgr::shared() {
	if(!sharedInstance){
		sharedInstance = new PiCarMgr;
	}
	return sharedInstance;
}


PiCarMgr::PiCarMgr(): _display(DisplayMgr::U8G2_OLED_DISPLAY){
	_isSetup = false;
	_isRunning = false;
	_autoShutdownMode = false;
	_shutdownDelay = 0;
	_autoDimmerMode = false;
	_dimLevel = 1.0;
	_isDayTime = true;
	_clocksync_gps = true;
	_clocksync_gps_secs = 30;
	_part_number = "JKR_ECU";
	_serial_number = "JKR_SERIAL";
	_canPeriodRadioTaskID = 0;
	_canPeriodRadio293TaskID = 0;
	_canPeriodAudioTaskID = 0;
	_shouldSendRadioCAN = false;
	_tuner_mode = TUNE_ALL;
	
	signal(SIGKILL, sigHandler);
	signal(SIGHUP, sigHandler);
	signal(SIGQUIT, sigHandler);
	
	signal(SIGTERM, sigHandler);
	signal(SIGINT, sigHandler);
	
	std::set_terminate( CRASH_Handler );
	
	_main_menu_map = {
		{MENU_SELECT_AUDIO_SOURCE,	"Source"},
		{MENU_AUDIO,	"Audio"},
		{MENU_GPS,		"GPS"},
		{MENU_WAYPOINTS,	"Waypoints"},
		{MENU_CANBUS,	"Engine Status"},
		{MENU_DTC,		"Diagnostics"},
 		{MENU_SETTINGS, "Settings"},
		{MENU_DEBUG, 	"Debug"},
		{MENU_TIME,		"Time"},
		{MENU_EXIT, 	 "Exit"}
	};
	
	_isRunning = true;
	
	pthread_create(&_piCarLoopTID, NULL,
						(THREADFUNCPTR) &PiCarMgr::PiCarLoopThread, (void*)this);
		
	_stations.clear();
	_preset_stations.clear();
	_scanner_freqs.clear();
	
}

PiCarMgr::~PiCarMgr(){
	stop();
	_isRunning = false;
	pthread_join(_piCarLoopTID, NULL);
}


bool PiCarMgr::begin(){
	
	signal(SIGTERM, sigHandler);
	signal(SIGINT, sigHandler);
	signal(SIGHUP, sigHandler);
	
	_isRunning = true;
	
	// start display with I2C
#if defined(__APPLE__)
	const char* path_display = "/dev/i2c.display";  // Mac testing path
#else
	const char* path_display = "/dev/i2c-1";  // Raspberry Pi default I2C bus
#endif

	if(_display.begin(path_display)) {  
		printf("Display started\n");
		_display.showStartup();  // Use the dedicated startup display method
	} else {
		printf("Failed to start Display\n");
		return false;
	}
	
	try {
		int error = 0;
		
		clock_gettime(CLOCK_MONOTONIC, &_startTime);
		_lastActivityTime = _startTime;
		
		_lastRadioMode = RadioMgr::MODE_UNKNOWN;
		_lastFreqForMode.clear();
		_tuner_mode = TUNE_ALL;
		_dimLevel =  1.0; // full
		_isDayTime = true;
		_autoDimmerMode = false;
		_autoShutdownMode = false;
		_shutdownDelay = UINT16_MAX;
		
		// clear DB
		_db.clearValues();
		_waypoints.clear();
		
		// read in any properties
		_db.restorePropertiesFromFile();
		
		// start 1Wire devices
		_w1.begin();
		
		startCPUInfo();
		startFan();
		
#if USE_TMP_117
		startTempSensors();
#endif
		
#if USE_COMPASS
		startCompass();
#endif
		startControls();
		
		// set initial brightness
		_display.setKnobBackLight(false);
		_display.setBrightness(_dimLevel);
		
		// SETUP CANBUS
		_can.begin();
		
		// find first RTS device
		auto devices = RtlSdr::get_devices();
		if(devices.size() > 0) {
			if(!_radio.begin(devices[0].index, pcmrate))
				throw Exception("failed to setup Radio ");
		}
		
#if defined(__APPLE__)
		const char* path_gps  = "/dev/cu.usbmodem14101";
#else
		const char* path_gps  = "/dev/ttyAMA1";
#endif
		
		if(!_gps.begin(path_gps, B38400, error))
			throw Exception("failed to setup GPS.  error: %d", error);
		
		// setup audio out
		if(!_audio.begin(pcmrate, true ))
			throw Exception("failed to setup Audio ");
		
		// quiet audio first
		if(!_audio.setVolume(0)
			|| ! _audio.setBalance(0))
			throw Exception("failed to setup Audio levels ");
		
		restoreStationsFromFile();
		restoreRadioSettings();
		
		
		_can.setPeriodicCallback(PiCarCAN::CAN_JEEP, 1000,
										 _canPeriodRadio293TaskID,  this, periodicCAN_CB_Radio293_wrapper);
	
		_can.setPeriodicCallback(PiCarCAN::CAN_JEEP, 1000,
										 _canPeriodRadioTaskID,  this, periodicCAN_CB_Radio_wrapper);
		
		_can.setPeriodicCallback(PiCarCAN::CAN_JEEP, 1000,
										 _canPeriodAudioTaskID,  this, periodicCAN_CB_Audio_wrapper);
		
		_dtc.begin();
		
		_isSetup = true;
		
		bool firstRunToday = true;
		time_t now = time(NULL);
		time_t lastRun = 0;
		
		if(_db.getTimeProperty(PROP_LAST_WRITE_DATE, &lastRun)){
			if( (now-lastRun) < 60 ){
				firstRunToday = false;
			}
		}
	 
		if(firstRunToday){
			LOGT_INFO("Hello Moto\n");
			 
			//	//		_audio.playSound("BTL.wav", [=](bool success){
			//
			//				printf("playSound() = %d\n", success);
			//			});
			
		}
 
		_gps.setTimeSyncCallback([=](time_t deviation, struct timespec gpsTime){
			if(shouldSyncClockToGPS(deviation)){
				setRTC(gpsTime);
				setECUtime(gpsTime);
			}
		} );
	 
	}
	catch ( const Exception& e)  {
		
		// display error on fail..
		
		printf("\tError %d %s\n\n", e.getErrorNumber(), e.what());
	}
	catch (std::invalid_argument& e)
	{
		// display error on fail..
		
		printf("EXCEPTION: %s ",e.what() );
	}
	
	
	return _isSetup;
}



void PiCarMgr::stop(){
	
	// Signal the main loop to stop
	_isRunning = false;
	
	// Wait for the thread to finish
	{
		std::unique_lock<std::mutex> lock(_shutdownMutex);
		_shutdownCV.wait_for(lock, std::chrono::seconds(5), [this] { return !_isRunning; });
	}
	
	int pid = getPidByName("shairport-sync");
	if(pid > 0){
		kill(pid, SIGTERM);
	}
	
	if(_isSetup){
		_isSetup = false;
		
		_dtc.stop();
		_can.removePeriodicCallback(_canPeriodRadioTaskID);
		_can.removePeriodicCallback(_canPeriodAudioTaskID);
		
		_display.setKnobBackLight(false);
		_gps.stop();
		_can.stop();
		_w1.stop();
		_display.stop();
		
		stopControls();
#if USE_COMPASS
		stopCompass();
#endif
#if USE_TMP_117
		stopTempSensors();
#endif
		
		stopCPUInfo();
		stopFan();
		
		_audio.setVolume(0);
		_audio.setBalance(0);
		_audio.setFader(0);
		
		_radio.stop();
		_audio.stop();
		sleep(1);
	}
}


void PiCarMgr::doShutdown(){
	
	stop();

#if defined(__APPLE__)
	//	system("/bin/sh shutdown -P now");
#else
	sync();
	sleep(1);
	reboot(RB_POWER_OFF);
#endif
	exit(EXIT_SUCCESS);

}


// MARK: -  convert to/from JSON


nlohmann::json PiCarMgr::GetAudioJSON(){
	json j;
	
	double  vol = _audio.volume();
	double  bal = _audio.balance();
	double  fade = _audio.fader();
	double  bass = _audio.bass();
	double  treble = _audio.treble();
	double  midrange = _audio.midrange();

	// limit the precisopn on these
	bass = std::floor((bass * 100) + .5) / 100;
	treble = std::floor((treble * 100) + .5) / 100;
	midrange = std::floor((midrange * 100) + .5) / 100;

	j[PROP_LAST_AUDIO_SETTING_VOL] 	=  vol;
	j[PROP_LAST_AUDIO_SETTING_BAL] 	=  bal;
	j[PROP_LAST_AUDIO_SETTING_FADER] =  fade;
 	j[PROP_LAST_AUDIO_SETTING_BASS]	 =  bass;
	j[PROP_LAST_AUDIO_SETTING_TREBLE] 	=  treble;
	j[PROP_LAST_AUDIO_SETTING_MIDRANGE] =  midrange;

	return j;
}

bool PiCarMgr::SetAudio(nlohmann::json j){
	bool success = false;
	
	if( j.is_object() ){
		
		if(  j.contains(PROP_LAST_AUDIO_SETTING_VOL)
			&&  j.at(PROP_LAST_AUDIO_SETTING_VOL).is_number()){
			double  val = j[PROP_LAST_AUDIO_SETTING_VOL];
			val = std::floor((val * 100) + .5) / 100;
			_audio.setVolume(val);
			success= true;
		}
		
		if(  j.contains(PROP_LAST_AUDIO_SETTING_BAL)
			&&  j.at(PROP_LAST_AUDIO_SETTING_BAL).is_number()){
			double  val = j[PROP_LAST_AUDIO_SETTING_BAL];
			val = std::floor((val * 100) + .5) / 100;
			_audio.setBalance(val);
			success= true;
		}
		
		if(  j.contains(PROP_LAST_AUDIO_SETTING_FADER)
			&&  j.at(PROP_LAST_AUDIO_SETTING_FADER).is_number()){
			double  val = j[PROP_LAST_AUDIO_SETTING_FADER];
			val = std::floor((val * 100) + .5) / 100;
			_audio.setFader(val);
			success= true;
		}
		
		if(  j.contains(PROP_LAST_AUDIO_SETTING_BASS)
			&&  j.at(PROP_LAST_AUDIO_SETTING_BASS).is_number()){
			double  val = j[PROP_LAST_AUDIO_SETTING_BASS];
			val = std::floor((val * 100) + .5) / 100;
			_audio.setBass(val);
			success= true;
		}
		if(  j.contains(PROP_LAST_AUDIO_SETTING_TREBLE)
			&&  j.at(PROP_LAST_AUDIO_SETTING_TREBLE).is_number()){
			double  val = j[PROP_LAST_AUDIO_SETTING_TREBLE];
			val = std::floor((val * 100) + .5) / 100;
			_audio.setTreble(val);
			success= true;
		}
		if(  j.contains(PROP_LAST_AUDIO_SETTING_MIDRANGE)
			&&  j.at(PROP_LAST_AUDIO_SETTING_MIDRANGE).is_number()){
			double  val = j[PROP_LAST_AUDIO_SETTING_MIDRANGE];
			val = std::floor((val * 100) + .5) / 100;
			_audio.setMidrange(val);
			success= true;
		}
	}
	
	return success;
}

nlohmann::json PiCarMgr::GetRadioModesJSON(){
	json j;
	
	for (auto& entry : _lastFreqForMode) {
		json j1;
		j1[PROP_LAST_RADIO_MODES_MODE] = RadioMgr::modeString(entry.first);
		j1[PROP_LAST_RADIO_MODES_FREQ] =  entry.second;
		j.push_back(j1);
	}
	
	return j;
}

bool PiCarMgr::updateRadioPrefs() {
	bool didUpdate = false;
	
	if(_radio.radioMode() == RadioMgr::AUX){
		_lastFreqForMode[RadioMgr::AUX] = 0;
		_lastRadioMode = RadioMgr::AUX;
		didUpdate = true;
	}
	else if(_radio.radioMode() == RadioMgr::AIRPLAY){
		_lastFreqForMode[RadioMgr::AIRPLAY] = 1;
		_lastRadioMode = RadioMgr::AIRPLAY;
		didUpdate = true;
	}
 	else if(_radio.isScannerMode()){
			_lastFreqForMode[RadioMgr::SCANNER] =2;
			_lastRadioMode = RadioMgr::SCANNER;
			didUpdate = true;
	}
	else if(_radio.radioMode() != RadioMgr::MODE_UNKNOWN
		&&  _radio.frequency() != 0) {
		
		auto mode = _radio.radioMode();
		_lastFreqForMode[mode] = _radio.frequency();
		_lastRadioMode = mode;
		didUpdate = true;
	}
	
	return didUpdate;
}

void PiCarMgr::saveRadioSettings(){
 
	updateRadioPrefs();
	updateWaypointProps();
 
	_db.setProperty(PROP_SYNC_CLOCK_TO_GPS, _clocksync_gps?_clocksync_gps_secs: 0);

	// dimmer mode
	if(_autoDimmerMode)
		_db.removeProperty(PROP_DIMMER_LEVEL);
	else
	{
		// force to tenths
		char buffer[8];
		sprintf(buffer, "%0.1f", _dimLevel );
 		_db.setProperty(PROP_DIMMER_LEVEL,atof(buffer));
	}
	_db.setProperty(PROP_AUTO_DIMMER_MODE, _autoDimmerMode);
 
	// shutdown mode
	if(_autoShutdownMode)
		_db.setProperty(PROP_SHUTDOWN_DELAY, _shutdownDelay);
	else
		_db.removeProperty(PROP_SHUTDOWN_DELAY);
	
 
	_db.setProperty(PROP_LONG_PRESS_MS, _long_press_ms);
	_db.setProperty(PROP_AUTO_SHUTDOWN_MODE, _autoShutdownMode);
 	_db.setProperty(PROP_SEND_RADIO_CAN, _shouldSendRadioCAN);
 
	_db.setProperty(PROP_TUNER_MODE, _tuner_mode);
	_db.setProperty(PROP_SQUELCH_LEVEL, _radio.getSquelchLevel());
	_db.setProperty(PROP_GAIN_LEVEL, _radio.getTunerGain());
	_db.setProperty(PROP_LAST_RADIO_MODES, GetRadioModesJSON());
	_db.setProperty(PROP_LAST_RADIO_MODE, RadioMgr::modeString(_lastRadioMode));
	_db.setProperty(PROP_LAST_AUDIO_SETTING, GetAudioJSON());
	_db.setProperty(PROP_PRESETS, GetRadioPresetsJSON());
	_db.setProperty(SCANNER_FREQS, GetScannerChannelJSON());
	
}

void PiCarMgr::restoreRadioSettings(){
	
	nlohmann::json j = {};
	
	// get jeep radio emulation numbers
 	_db.getProperty(SERIAL_NUM, &_serial_number);
	_db.getProperty(PART_NUM, &_part_number);
  
	// SET GPS CLOCK SYNC PREFS
	{
		_clocksync_gps = false;
		_clocksync_gps_secs = 0;
		
		uint16_t syncVal = 0;
		if(_db.getUint16Property(PROP_SYNC_CLOCK_TO_GPS,&syncVal) && syncVal > 0){
			_clocksync_gps = true;
			_clocksync_gps_secs = syncVal;
		}
	}
	
	// SET shutdown mode
 	if(_db.getBoolProperty(PROP_AUTO_SHUTDOWN_MODE,&_autoShutdownMode) && _autoShutdownMode){
		_db.getUint16Property(PROP_SHUTDOWN_DELAY, &_shutdownDelay);
	}
   else
		_shutdownDelay = UINT16_MAX;
 
	if(!_db.getUint16Property(PROP_LONG_PRESS_MS,&_long_press_ms))
		_long_press_ms = 750;  //default
 
	// Send radio CAN messages
	_db.getBoolProperty(PROP_SEND_RADIO_CAN,&_shouldSendRadioCAN) ;
 
	// SET Dimmer
	if(_db.getBoolProperty(PROP_AUTO_DIMMER_MODE,&_autoDimmerMode) && _autoDimmerMode){
		setDimLevel(1.0);
	}
	else {
		float dimLevel = 1.0;
		if(_db.getFloatProperty(PROP_DIMMER_LEVEL, &dimLevel)){
			setDimLevel(dimLevel);
		}
	}
	
	// SET Audio
	if(!( _db.getJSONProperty(PROP_LAST_AUDIO_SETTING,&j)
		  && SetAudio(j))){
		_audio.setVolume(.6);
		_audio.setBalance(0.0);
		_audio.setFader(0.0);
	}
	
	// SET RADIO Info
	
	_lastFreqForMode.clear();
	
	if(_db.getJSONProperty(PROP_LAST_RADIO_MODES,&j)
		&&  j.is_array()){
		
		for(auto item : j ){
			if(item.is_object()
				&&  item.contains(PROP_LAST_RADIO_MODES_MODE)
				&&  item[PROP_LAST_RADIO_MODES_MODE].is_string()
				&&  item.contains(PROP_LAST_RADIO_MODES_FREQ)
				&&  item[(PROP_LAST_RADIO_MODES_FREQ)].is_number()){
				
				auto mode = RadioMgr::stringToMode( item[PROP_LAST_RADIO_MODES_MODE]);
				auto freq = item[PROP_LAST_RADIO_MODES_FREQ] ;
				_lastFreqForMode[mode] = freq;
			}
		}
	}
	
	// SET SQUELCH
	int squelch_level = 0;
	_db.getIntProperty(PROP_SQUELCH_LEVEL, &squelch_level);
	if(squelch_level < _radio.getMaxSquelchRange())
		squelch_level = _radio.getMaxSquelchRange();
	_radio.setSquelchLevel(squelch_level);
 
	// SET Tuner Gain
	int tuner_gain = INT_MIN;
	_db.getIntProperty(PROP_GAIN_LEVEL, &tuner_gain);
	_radio.setTunerGain(tuner_gain);

	// SET Preset stations
	
	_preset_stations.clear();
	if(_db.getJSONProperty(PROP_PRESETS,&j)
		&&  j.is_array()){
		
		vector < pair<RadioMgr::radio_mode_t,uint32_t>>  presets;
		presets.clear();
		
		for(auto item : j ){
			if(item.is_object()
				&&  item.contains(PROP_PRESET_MODE)
				&&  item[PROP_PRESET_MODE].is_string()
				&&  item.contains(PROP_PRESET_FREQ)
				&&  item[(PROP_PRESET_FREQ)].is_number()){
				
				auto mode = RadioMgr::stringToMode( item[PROP_PRESET_MODE]);
				auto freq = item[PROP_PRESET_FREQ] ;
				
				presets.push_back(make_pair(mode, freq));
			}
		}
		
		//sort them in order of frequency
		if( presets.size() >0 ){
			sort(presets.begin(), presets.end(),
				  [] (const pair<RadioMgr::radio_mode_t,uint32_t>& a,
						const pair<RadioMgr::radio_mode_t,uint32_t>& b) { return a.second < b.second; });
		}
		_preset_stations = presets;
	}
	
	// SET Scanner Freq
	
	_scanner_freqs.clear();
	if(_db.getJSONProperty(SCANNER_FREQS,&j)
		&&  j.is_array()){
		
		vector < pair<RadioMgr::radio_mode_t,uint32_t>>  presets;
		presets.clear();
		
		for(auto item : j ){
			if(item.is_object()
				&&  item.contains(PROP_PRESET_MODE)
				&&  item[PROP_PRESET_MODE].is_string()
				&&  item.contains(PROP_PRESET_FREQ)
				&&  item[(PROP_PRESET_FREQ)].is_number()){
				
				auto mode = RadioMgr::stringToMode( item[PROP_PRESET_MODE]);
				auto freq = item[PROP_PRESET_FREQ] ;
				
				presets.push_back(make_pair(mode, freq));
			}
		}
		
		//sort them in order of frequency
		if( presets.size() >0 ){
			sort(presets.begin(), presets.end(),
				  [] (const pair<RadioMgr::radio_mode_t,uint32_t>& a,
						const pair<RadioMgr::radio_mode_t,uint32_t>& b) { return a.second < b.second; });
		}
		
		_scanner_freqs = presets;
	}
 
	_tuner_mode = TUNE_ALL;
	uint16_t val = 0;
	if(_db.getUint16Property(PROP_TUNER_MODE, &val)){
		_tuner_mode = static_cast<tuner_knob_mode_t>(val);
	}
	
	string str;
	if(_db.getProperty(PROP_LAST_RADIO_MODE,&str)) {
		auto mode = RadioMgr::stringToMode(str);
		_lastRadioMode = mode;
	}
	
	// read the 1-wire device map
	_w1Map.clear();
	if(_db.getJSONProperty(PROP_W1_MAP,&j)
		&&  j.is_array()){
		for(auto item : j ){
			if(item.is_object()
				&&  item.contains(PROP_ID)
				&&  item[PROP_ID].is_string()
				&&  item.contains(PROP_KEY)
				&&  item[PROP_KEY].is_string()){
				
				auto deviceid  =   item[PROP_ID];
				auto dbKey 		  =   item[PROP_KEY];
				string title = dbKey;
				
				if(item.contains(PROP_TITLE)
					&&  item[PROP_TITLE].is_string()){
					title = item[PROP_TITLE];
				}
				
				w1_map_entry entry = {
					.deviceID = deviceid,
					.dbKey = dbKey,
					.title = title
				};
			_w1Map[deviceid] = entry;
			}
		}
	}
	
	getWaypointProps();
}


// MARK: - Presets

nlohmann::json PiCarMgr::GetRadioPresetsJSON(){
	json j;
	
	for (auto& entry : _preset_stations) {
		json j1;
		j1[PROP_PRESET_MODE] = RadioMgr::modeString(entry.first);
		j1[PROP_PRESET_FREQ] =  entry.second;
		j.push_back(j1);
	}

	return j;
}

bool PiCarMgr::setPresetChannel(RadioMgr::radio_mode_t mode, uint32_t  freq){
	
	if(!isPresetChannel(mode,freq)){
		
		auto presets = _preset_stations;
		
		presets.push_back(make_pair(mode, freq));
		
		// re-sort them
		sort(presets.begin(), presets.end(),
			  [] (const pair<RadioMgr::radio_mode_t,uint32_t>& a,
					const pair<RadioMgr::radio_mode_t,uint32_t>& b) { return a.second < b.second; });
		
		_preset_stations = presets;
		return true;
	}
	
	return false;
}

bool PiCarMgr::clearPresetChannel(RadioMgr::radio_mode_t mode, uint32_t  freq){
	
	for(auto it = _preset_stations.begin(); it != _preset_stations.end(); it++){
		if(it->first == mode && it->second == freq){
			_preset_stations.erase(it);
			return true;
		}
	}
	
	return false;
}

bool PiCarMgr::isPresetChannel(RadioMgr::radio_mode_t mode, uint32_t  freq){
	
	for(auto e: _preset_stations){
		if(e.first == mode && e.second == freq){
			return true;
		}
		else if(mode == RadioMgr::SCANNER && e.first == RadioMgr::SCANNER) {
			return true;
		}
	}
	return false;
}


bool PiCarMgr::nextPresetStation(RadioMgr::radio_mode_t band,
											uint32_t frequency,
											bool tunerMovedCW,
											station_info_t &info){
	
	if(_preset_stations.size() == 0 ) {
		// if there are no known frequencies  all then to fallback to all.
		info.band = band;
		info.frequency =  _radio.nextFrequency(tunerMovedCW);
		info.title = "";
		info.location = "";
		return true;
	}
	
	auto v = _preset_stations;
	
	if(tunerMovedCW){
		
		for ( auto i = v.begin(); i != v.end(); ++i ) {
			if(frequency >= i->second)
				continue;
			
			info.title = "";
			info.location = "";
			info.band = i->first;
			info.frequency =  i->second;
			return true;;
		}
	}
	else {
		for ( auto i = v.rbegin(); i != v.rend(); ++i ) {
			if(frequency <= i->second)
				continue;
			
			info.title = "";
			info.location = "";
			info.band = i->first;
			info.frequency =  i->second;
			return true;;
		}
	}
	
	return false;
}

// MARK: - Scanner

nlohmann::json PiCarMgr::GetScannerChannelJSON(){
	json j;
	
	for (auto& entry : _scanner_freqs) {
		json j1;
		j1[PROP_PRESET_MODE] = RadioMgr::modeString(entry.first);
		j1[PROP_PRESET_FREQ] =  entry.second;
		j.push_back(j1);
	}

	return j;
}

bool PiCarMgr::setScannerChannel(RadioMgr::radio_mode_t mode, uint32_t  freq){
	
	if(!isScannerChannel(mode,freq)){
		
		auto presets = _scanner_freqs;
		
		presets.push_back(make_pair(mode, freq));
		
		// re-sort them
		sort(presets.begin(), presets.end(),
			  [] (const pair<RadioMgr::radio_mode_t,uint32_t>& a,
					const pair<RadioMgr::radio_mode_t,uint32_t>& b) { return a.second < b.second; });
		
		_scanner_freqs = presets;
		return true;
	}
	
	return false;
}

bool PiCarMgr::clearScannerChannel(RadioMgr::radio_mode_t mode, uint32_t  freq){
	
	for(auto it = _scanner_freqs.begin(); it != _scanner_freqs.end(); it++){
		if(it->first == mode && it->second == freq){
			_scanner_freqs.erase(it);
			return true;
		}
	}
	
	return false;
}

bool PiCarMgr::isScannerChannel(RadioMgr::radio_mode_t mode, uint32_t  freq){
	
	for(auto e: _scanner_freqs){
		if(e.first == mode && e.second == freq){
			return true;
		}
	}
	return false;
}


bool PiCarMgr::nextScannerStation(RadioMgr::radio_mode_t band,
											uint32_t frequency,
											bool tunerMovedCW,
											station_info_t &info){
	
	if(_preset_stations.size() == 0 ) {
		// if there are no known frequencies  all then to fallback to all.
		info.band = band;
		info.frequency =  _radio.nextFrequency(tunerMovedCW);
		info.title = "";
		info.location = "";
		return true;
	}
	
	auto v = _scanner_freqs;
	
	if(tunerMovedCW){
		
		for ( auto i = v.begin(); i != v.end(); ++i ) {
			if(frequency >= i->second)
				continue;
			
			info.title = "";
			info.location = "";
			info.band = i->first;
			info.frequency =  i->second;
			return true;;
		}
	}
	else {
		for ( auto i = v.rbegin(); i != v.rend(); ++i ) {
			if(frequency <= i->second)
				continue;
			
			info.title = "";
			info.location = "";
			info.band = i->first;
			info.frequency =  i->second;
			return true;;
		}
	}
	
	return false;
}

//  MARK: - FrequencyandMode

void PiCarMgr::getSavedFrequencyandMode( RadioMgr::radio_mode_t &modeOut, uint32_t &freqOut){
	
	if(_lastRadioMode != RadioMgr::MODE_UNKNOWN
		&& _lastFreqForMode.count(_lastRadioMode)){
		modeOut = _lastRadioMode;
		freqOut =  _lastFreqForMode[_lastRadioMode];
	}
	else {
		// set it to something;
		modeOut =  _lastRadioMode = RadioMgr::BROADCAST_FM;
		freqOut = _lastFreqForMode[_lastRadioMode] = 101500000;
	}
}

bool PiCarMgr::getSavedFrequencyForMode( RadioMgr::radio_mode_t mode, uint32_t &freqOut){
	bool success = false;
	
	if( _lastFreqForMode.count(mode)){
		freqOut =  _lastFreqForMode[mode];
		success = true;
	}
	
	return success;
}



// MARK: - stations File


bool PiCarMgr::restoreStationsFromFile(string filePath){
	bool success = false;
	
	std::ifstream	ifs;
	
	// create a file path
	if(filePath.size() == 0)
		filePath = "stations.tsv";
	
	try{
		string line;
		
		_stations.clear();
		
		// open the file
		ifs.open(filePath, ios::in);
		if(!ifs.is_open()) return false;
		
		while ( std::getline(ifs, line) ) {
			
			// split the line looking for a token: and rest and ignore comments
			line = Utils::trimStart(line);
			if(line.size() == 0) continue;
			if(line[0] == '#')  continue;
			
			vector<string> v = split<string>(line, "\t");
			if(v.size() < 3)  continue;
			
			RadioMgr::radio_mode_t mode =  RadioMgr::stringToMode(v[0]);
			uint32_t freq = atoi(v[1].c_str());
			
			string title = trimCNTRL(v[2]);
			string location = trimCNTRL((v.size() >2) ?v[2]: string());
			
			if(freq != 0
				&& mode != RadioMgr::MODE_UNKNOWN){
				station_info_t info = {mode, freq, title, location};
				_stations[mode].push_back(info);
				
			}
		}
		for( auto &[mode, items]: _stations){
			sort(items.begin(), items.end(),
				  [] (const station_info_t& a,
						const station_info_t& b) { return a.frequency < b.frequency; });
		}
		
		
		success = _stations.size() > 0;
		ifs.close();
	}
	catch(std::ifstream::failure &err) {
		ELOG_MESSAGE("READ stations:FAIL: %s", err.what());
		success = false;
	}
	return success;
}

bool PiCarMgr::getStationInfo(RadioMgr::radio_mode_t band,
										uint32_t frequency,
										station_info_t &info){
	
	if(_stations.count(band)){
		for(const auto& e : _stations[band]){
			if(e.frequency == frequency){
				info = e;
				return true;
			}
		}
	}
	
	return false;
}

bool PiCarMgr::nextKnownStation(RadioMgr::radio_mode_t band,
										  uint32_t frequency,
										  bool tunerMovedCW,
										  station_info_t &info){
	
 	if(_stations.count(band) == 0 ) {
		// if there are no known frequencies  all then to fallback to all.
		info.band = band;
		info.frequency =  _radio.nextFrequency(tunerMovedCW);
		info.title = "";
		info.location = "";
		return true;
	}
	
	auto v = _stations[band];
	// scan for next known freq
	
	if(tunerMovedCW){
		
		for ( auto i = v.begin(); i != v.end(); ++i ) {
			if(frequency >= i->frequency)
				continue;
 			info =  *(i);
 			return true;;
		}
	}
	else {
		for ( auto i = v.rbegin(); i != v.rend(); ++i ) {
			if(frequency <= i->frequency)
				continue;
			info =  *(i);
 			return true;;
		}
	}
 	return false;
}



// MARK: -  Waypoints

void PiCarMgr::getWaypointProps(){
	
	_waypoints.clear();
	
	nlohmann::json j = {};
	
	if(_db.getJSONProperty(PROP_WAYPOINTS,&j)
		&&  j.is_array()){
		
		for(auto item : j ){
			if(item.is_object()
				&&  item.contains(PROP_UUID)  &&  item[PROP_UUID].is_string()
				&&  item.contains(PROP_TITLE) &&  item[(PROP_TITLE)].is_string()
				&&  item.contains(PROP_LONGITUDE)   &&  item[(PROP_LONGITUDE)].is_number_float()
				&&  item.contains(PROP_LATITUDE)   &&  item[(PROP_LATITUDE)].is_number_float()
				){
				
 				double 		longitude  = item[PROP_LONGITUDE];
				double 		latitude  = item[PROP_LATITUDE];
				string 		name  = item[PROP_TITLE];
				string 		uuid  = item[PROP_UUID];
		 
				waypoint_prop_t wp = {
					.uuid = uuid,
					.name = name,
					.location.latitude = latitude,
					.location.longitude = longitude,
					.location.timestamp = {0,0},
					.location.DOP = 255,
					.location.isValid = true
				};
				
				if(item.contains(PROP_ALTITUDE)   &&  item[(PROP_ALTITUDE)].is_number_float()){
					wp.location.altitude  =  item[PROP_ALTITUDE];
					wp.location.altitudeIsValid = true;
				}
				
				if(item.contains(PROP_DOP)   &&  item[(PROP_DOP)].is_number_unsigned()){
					wp.location.DOP  =  item[PROP_DOP];
 				}
	 
				if(item.contains(PROP_TIMESTAMP)   &&  item[(PROP_TIMESTAMP)].is_string()){
					string timeStr = item[PROP_TIMESTAMP];
	 					 wp.location.timestamp.tv_sec =  	TimeStamp(timeStr).getTime();
	 			}
				
				_waypoints.push_back(wp);
			}
		}
	}
	sortWaypoints();
}

void PiCarMgr::updateWaypointProps(){
	nlohmann::json j = {};

	for(waypoint_prop_t wp : _waypoints){
		
		json item;
		item[PROP_UUID] = wp.uuid;
		item[PROP_TITLE] = wp.name;
		item[PROP_LONGITUDE] = wp.location.longitude;
		item[PROP_LATITUDE] = wp.location.latitude;
		item[PROP_DOP] = wp.location.DOP;
		item[PROP_TIMESTAMP] = TimeStamp(wp.location.timestamp.tv_sec).RFC1123String();
		 
		if(wp.location.altitudeIsValid ){
			item[PROP_ALTITUDE] = wp.location.altitude;
		}

		j.push_back(item);
	}
 
	_db.setProperty(PROP_WAYPOINTS, j);
}

bool PiCarMgr::createWaypoint(string name, waypoint_prop_t &wpout ) {
	
	bool success = false;
	waypoint_prop_t wp = {};
	GPSLocation_t location = {};
	
	// create default name
	if (name.empty()) {
		name = "WP_" + TimeStamp().ISO8601String();
	}
	wp.name = name;
	wp.uuid = _db.generateUUID_v4();
	
	if(_gps.GetLocation(location) & location.isValid){
		wp.location = location;
		success = true;
	}
	
	if(success)
		wpout = wp;
	
	return success;
}

bool PiCarMgr::updateWaypoint(string uuid) {
	bool success = false;
	
	GPSLocation_t location = {};
	
	if(_gps.GetLocation(location) & location.isValid){
		
		for( int i = 0; i < _waypoints.size(); i++){
			auto wp = &_waypoints[i];
			if(wp->uuid == uuid){
				wp->location = location;
				updateWaypointProps();
				_db.savePropertiesToFile();
				success = true;
				break;
			}
		}
 	}
 
	return success;
}

 bool PiCarMgr::deleteWaypoint(string uuid) {
 
	 _waypoints.erase(
							remove_if(_waypoints.begin(), _waypoints.end(), ([=](const  waypoint_prop_t &wp){
		 return wp.uuid == uuid;
	 })));
	 
	 sortWaypoints();
	 updateWaypointProps();
	 _db.savePropertiesToFile();
	return true;
}
 
void PiCarMgr::sortWaypoints() {

	if( _waypoints.size() >0 ){
		sort(_waypoints.begin(), _waypoints.end(),
			  [] (const waypoint_prop_t& a,
					const waypoint_prop_t& b) { return a.name < b.name; });
	}
 }
 

bool PiCarMgr::getWaypointInfo(string uuid, waypoint_prop_t &prop){
	bool success = false;
	
	for( int i = 0; i < _waypoints.size(); i++){
		auto wp = &_waypoints[i];
		if(wp->uuid == uuid){
			
			prop = *wp;
			success = true;
			break;
		}
	}
	
	
	return success;
	
}

// MARK: -  CAN BUS PERIODIC CALLBACKS

// these probably exist to tell the amplifier to turn on or set audio
bool PiCarMgr::periodicCAN_CB_Audio_wrapper(void* context,  canid_t &can_id, vector<uint8_t> &bytes){
	PiCarMgr* mgr = (PiCarMgr*)context;
	return mgr->periodicCAN_CB_Audio(can_id, bytes);
	
}

bool PiCarMgr::periodicCAN_CB_Radio_wrapper(void* context,  canid_t &can_id, vector<uint8_t> &bytes){
	PiCarMgr* mgr = (PiCarMgr*)context;
	return mgr->periodicCAN_CB_Radio(can_id, bytes);
}

bool PiCarMgr::periodicCAN_CB_Radio293_wrapper(void* context,  canid_t &can_id, vector<uint8_t> &bytes){
	PiCarMgr* mgr = (PiCarMgr*)context;
	return mgr->periodicCAN_CB_Radio293(can_id, bytes);
}
 

bool PiCarMgr::periodicCAN_CB_Audio(canid_t &can_id, vector<uint8_t> &bytes){
 
	if(!_shouldSendRadioCAN) return false;

	/*
 3D9 Radio Settings broadcast
	 [7]  Vl Bl Fa Ba Mi Tr FF
		 Vl - Volume  		00-26x   00 - 38
		 Bl - Balance		1 (-9)  - 10 (0) - 19 (+9)
		 Fa - Fader
		 Ba - Bass
		 Mi - Midrange
		 Tr - Treble
 */


	double  vol = _audio.volume();
	double  bal = _audio.balance();
	double  fade = _audio.fader();
	double  bass = _audio.bass();
	double  treble = _audio.treble();
	double  midrange = _audio.midrange();

	vector<uint8_t>  packet = {
		static_cast<uint8_t>(vol * 38),
		static_cast<uint8_t> (bal * 10  + 10),
		static_cast<uint8_t> (fade * 10  + 10),
		static_cast<uint8_t> (bass * 10  + 10),
		static_cast<uint8_t> (midrange * 10  + 10),
		static_cast<uint8_t> (treble * 10  + 10),
		0xff
	};

	can_id = 0x3D9;
	bytes = packet;

	return true;
	}

bool PiCarMgr::periodicCAN_CB_Radio(canid_t &can_id, vector<uint8_t> &bytes){

	if(!_shouldSendRadioCAN) return false;
	
	/*
	 291 Radio Mode
		 [7]  ss 0D 05 30 tt 00 07
			  ss = Mode
				 00 - AM
				 01 - FM
				 06 - Aux
				 09 - SAT
				 1D - OFF
			 
			 tt  10 Tunned
				  80 Changed
	 */
	
	uint8_t mode = 0x1D;
 
	if(_radio.isConnected() && _radio.isOn()){
		
		switch(_lastRadioMode){
			case  RadioMgr::BROADCAST_AM:
				mode = 00;
				break;
				
			case  RadioMgr::BROADCAST_FM:
				mode = 01;
				break;
				
			case RadioMgr::AUX:
				mode = 0x06;
				break;
	 
				// I dont have a clear st of equivalants  - so treat like AUX
			case  RadioMgr::VHF:
			case  RadioMgr::UHF:
			case RadioMgr::AIRPLAY:
			case RadioMgr::SCANNER:
			default:
				mode = 0x06;
				break;
		}
	};
	
	vector<uint8_t>  packet = {
		static_cast<uint8_t> (mode),
		0x0D,
		0x05,
		0x30,
		0x00,
		0x00,
		0x07
	};

	can_id = 0x291;
	bytes = packet;

	return true;
}


bool PiCarMgr::periodicCAN_CB_Radio293(canid_t &can_id, vector<uint8_t> &bytes){

	if(!_shouldSendRadioCAN) return false;

	/*
	 293 Radio Station
		 [8] 00 cc cc pp 00 FF FF FF
			 cc = Channel / Station
			 pp = is preselect button ID
	 */
	
	 
	if(_radio.isConnected() && _radio.isOn()){
		RadioMgr::radio_mode_t  mode  = _radio.radioMode();
		
		if(mode == RadioMgr::BROADCAST_FM){
			uint16_t  freq =  _radio.frequency() /1.0e5;
 
			vector<uint8_t>  packet = {
				static_cast<uint8_t>(mode),
				0x01,
				static_cast<uint8_t> (freq >> 8),
				static_cast<uint8_t> (freq & 0xFF) ,
				0x00,
				0xFF,
				0xFF,
				0xFF
			};

			can_id = 0x293;
			bytes = packet;

			return true;
		}
		
	};
	
 	return false;
}

// MARK: -  PiCarMgr main loop  thread

void PiCarMgr::PiCarLoop(){
	
	PRINT_CLASS_TID;
	
	bool 	firstRun = false;
	bool	waitingForTunerLongPress 	= false;
	struct timespec	lastTunerPressed = {0,0};
	
	bool tunerWasMoved = false;
	bool tunerMovedCW = false;
	bool tunerWasClicked = false;
	bool tunerWasDoubleClicked = false;
	bool tunerIsPressed = false;
	bool tunerLongPress = false;
	bool volWasClicked = false;
	bool volWasDoubleClicked = false;
	bool volWasMoved = false;
	bool volMovedCW = false;  // Add this variable declaration

	try{
		

		while(_shouldRun){
			
			// if not setup // check back shortly -  we are starting up
			if(!_isSetup){
				usleep(200000);
				continue;
			}
			
			// call things that need to be setup,
			// run idle first to prevent delays on first run.
			
			if(!firstRun){
				idle();
				firstRun = true;
			}
			
			// knob management -
			
			DuppaKnob* tunerKnob =	_display.rightKnob();
			DuppaKnob* volKnob = 	_display.leftKnob();
			
			uint8_t volKnobStatus __attribute__((unused)) = 0;
			uint8_t tunerKnobStatus __attribute__((unused))  = 0;
			
			// loop until status changes
			for(;;) {
				
				if(!_isSetup) break;
				
				// check if knobs changed
				{
					bool status_changed __attribute__((unused)) = false;
					
					volKnob->updateStatus();
					volWasClicked = volKnob->wasClicked();
					volWasDoubleClicked = volKnob->wasDoubleClicked();
					volWasMoved = volKnob->wasMoved(volMovedCW);
	
					tunerKnob->updateStatus();
					tunerWasClicked = tunerKnob->wasClicked();
					tunerWasDoubleClicked = tunerKnob->wasDoubleClicked();
					tunerWasMoved = tunerKnob->wasMoved(tunerMovedCW);
					tunerIsPressed = tunerKnob->isPressed();
	
					// if changed, break
					if(volWasClicked ||  volWasDoubleClicked || volWasMoved
						|| tunerWasClicked  || tunerWasDoubleClicked
						|| tunerWasMoved  || tunerIsPressed){
						break;
					}
 				}
 
				// or take a nap
				
#if USE_GPIO_INTERRUPT
				int err = 0;
				
				struct timespec timeout;
				// Timeout in polltime seconds
				timeout.tv_sec =  0;
				timeout.tv_nsec = waitingForTunerLongPress?1e8:5e8;  //  .1 while waiting else.5 sec
				gpiod_line_event evt;
				
				// gpiod_line_event_wait return 0 if wait timed out,
				// -1 if an error occurred,
				//  1 if an event occurred.
				err = gpiod_line_event_wait(_gpio_line_int, &timeout);
				if(err == -1){
					break;
					//				throw Exception("gpiod_line_event_wait failed ");
				}
				// gpiod_line_event_wait only blocks until there's an event or a timeout
				// occurs, it does not read the event.
				// call gpiod_line_event_read  to consume the event.
				else if (err == 1) {
					gpiod_line_event_read(_gpio_line_int, &evt);
				}
				else if (err == 0){
					// timeout occured ..
					//  call idle when nothing else is going on
					idle();
					if(waitingForTunerLongPress) break;
				}
#else
				usleep(10000);
				idle();
#endif
			}
			
			// maybe we quit while I was asleep.  bail now
			if(!_isRunning) continue;
			
			// MARK:   Tuner button long  press
	
			if(!tunerWasClicked && tunerIsPressed){ //button hold down
				// record the time we pressed
				clock_gettime(CLOCK_MONOTONIC, &lastTunerPressed);
				waitingForTunerLongPress = true;
	
 			} else  if(tunerWasClicked && !tunerIsPressed){ //button let go
 				// ignore this cycle
	 			waitingForTunerLongPress = false;
				lastTunerPressed = {0,0};
				continue;
			}
			else if(waitingForTunerLongPress) {
				struct timespec now;
				clock_gettime(CLOCK_MONOTONIC, &now);
				
				long ms =  timespec_to_ms(timespec_sub(now, lastTunerPressed)) ;
				
				if(ms > _long_press_ms)  {
					waitingForTunerLongPress = false;
					tunerLongPress = true;
				}
			}

			// mark the last time any user activity
			if(volWasClicked ||  volWasDoubleClicked || volWasMoved
				|| tunerWasClicked  || tunerWasDoubleClicked
				|| tunerWasMoved  || tunerIsPressed){
				clock_gettime(CLOCK_MONOTONIC, &_lastActivityTime);
 			}
	 
			// MARK:   Volume button Clicked
			if(volWasDoubleClicked){
				// toggle mute
				
				if(_radio.isConnected() && _radio.isOn()){
					_audio.setMute(!_audio.isMuted());
					
					if(_audio.isMuted())
						_display.LEDeventMute();
					else
						_display.LEDeventVol();
				}
			}
			
			if(volWasClicked){
				
				if(_radio.isConnected()){
					bool isOn = _radio.isOn();
					
					if(isOn){
						// just turn it off
						_radio.setON(false);
						setRelay1(false);
						
						// always unmute after
						_audio.setMute(false);
						
						// stop any Mute blinking
						_display.LEDeventStop();
						
						// clear any Airplay metadata
						_display.clearAPMetaData();
						
						// turn it off forces save of stations.
						saveRadioSettings();
						_db.savePropertiesToFile();
						_display.enableAutoPlay(false);
						
					}
					else {
						RadioMgr::radio_mode_t mode ;
						uint32_t freq;
						
						// stop any animation
						_display.LEDeventStop();
						
						_audio.setMute(false);
						getSavedFrequencyandMode(mode,freq);
						
						if(mode == RadioMgr::SCANNER){
							_radio.scanChannels(_scanner_freqs);
						}
						else {
							_radio.setFrequencyandMode(mode, freq);
						}
						
						_radio.setON(true);
						setRelay1(true);
						
						_display.LEDeventVol();
						
						if(_radio.isScannerMode())
							_display.showScannerChange();
						else
							_display.showRadioChange();
						
						_db.setProperty(PROP_LAST_MENU_SELECTED, to_string(main_menu_map_offset(MENU_RADIO)));
						
						_display.enableAutoPlay(true);
					}
				}
				else {
					auto devices = RtlSdr::get_devices();
					if(devices.size() == 0){
						printf("no Radio found\n");
						_display.showDevStatus();  // show startup
						
					}
					//					if(devices.size() > 0) {
					//						if(!_radio.begin(devices[0].index))
					
				}
				
			}
			
			// MARK:   Volume button moved
			if(volWasMoved && _radio.isOn() ){
				
				//quit mute
				if(_audio.isMuted())
					_audio.setMute(false);
				
				// change  volume
				double volume = _audio.volume();
				
				if(volMovedCW){
					if(volume < 1) {						// twist up
						volume += 0.1; // Increase by 10%
						if(volume > 1) volume = 1.0;	// pin volume
						_audio.setVolume(volume);
						_db.updateValue(VAL_AUDIO_VOLUME, volume);
					}
				}
				else {
					if(volume > 0) {							// twist down
						volume -= 0.1; // Decrease by 10%
						if(volume < 0) volume = 0.0;		// twist down
						_audio.setVolume(volume);
						_db.updateValue(VAL_AUDIO_VOLUME, volume);
					}
				}
				
				_display.LEDeventVol();
				
				// if the radio was not displayed, set it there now
				auto dMode =  _display.active_mode();
 				if(_radio.isScannerMode()){
					if(dMode != DisplayMgr::MODE_SCANNER)
						_display.showScannerChange();
				}
				else {
					if(dMode != DisplayMgr::MODE_RADIO)
						_display.showRadioChange();
				}
			}
			
			// MARK:   Tuner button moved
			if(tunerWasMoved) {
				
				bool didChangeChannel = false;
				bool tunnerPinned = false;
				
				if(_display.usesSelectorKnob()
					&& _display.selectorKnobAction(tunerMovedCW?DisplayMgr::KNOB_UP:DisplayMgr::KNOB_DOWN)){
					// was handled - do nothing
				}
				// change tuner
				else if( _radio.isOn()
						  &&  ( _display.active_mode() == DisplayMgr::MODE_RADIO
								 || _display.active_mode() == DisplayMgr::MODE_SCANNER)){
					
					auto nextFreq = _radio.frequency();
		 
					auto mode 	   = _radio.radioMode();
					bool isScanning = _radio.isScannerMode();
					switch(_tuner_mode){
						case TUNE_ALL:
							if(isScanning) {
								// if you are scanning an roll tuner - do nothing
							}
							else if(mode == RadioMgr::AIRPLAY || mode == RadioMgr::AUX){
								break;
							}
		 					else {
								auto newFreq = _radio.nextFrequency(tunerMovedCW);
								if(newFreq == nextFreq)
									tunnerPinned = true;
								nextFreq = newFreq;
								
								didChangeChannel = true;
							}
							break;
							
						case TUNE_KNOWN:
						{
							if(isScanning) {
								// if you are scanning an roll tuner - do nothing
							}
							else if(mode == RadioMgr::AIRPLAY || mode == RadioMgr::AUX){
								break;
							}
							else {
								
								PiCarMgr::station_info_t info;
								if(nextKnownStation(mode, nextFreq, tunerMovedCW, info)){
									
									if(info.frequency == nextFreq && info.band == mode)
										tunnerPinned = true;
		
									nextFreq = info.frequency;
								}
								else
									tunnerPinned = true;
									
								didChangeChannel = true;
							}
						}
							break;
							
						case TUNE_PRESETS:
							// if you are scanning an roll tuner - move to next preset
							PiCarMgr::station_info_t info;
							if(nextPresetStation(mode, nextFreq, tunerMovedCW, info)){
								
								if(info.frequency == nextFreq && info.band == mode)
									tunnerPinned = true;
	
								nextFreq = info.frequency;
								mode = info.band;
								isScanning =( mode == RadioMgr::SCANNER);
							}
							else tunnerPinned = true;
 							didChangeChannel = true;
							
							break;
					}
					
					if( isScanning){
						_radio.scanChannels(_scanner_freqs);
						_display.showScannerChange();
					}
					else {
						_radio.setFrequencyandMode(mode, nextFreq);
						_display.showRadioChange();
						
						if(didChangeChannel){
							if(tunerMovedCW)
								_display.LEDTunerUp(tunnerPinned);
							else
								_display.LEDTunerDown(tunnerPinned);

						}
					}
				}
			}
			
			
			// MARK:   Tuner button double clicked
			if(tunerWasDoubleClicked){
				
				if(_display.usesSelectorKnob()
					&& _display.selectorKnobAction(DisplayMgr::KNOB_DOUBLE_CLICK)){
					// was handled - do nothing
				}
				else{
					tunerDoubleClicked();
				}
 				continue;
			}
	 
			// MARK:   Tuner long press
			if( tunerLongPress) {
				// special case ,, we are scanning and long press tuner knob
				// go right to squelch
					if(_radio.isOn() && _radio.canSquelch()){
	 				_display.showSquelchChange();
 				}
				continue;;
			}
 
			// MARK:   Tuner button click
			if(tunerWasClicked  && tunerIsPressed){
				if(_display.usesSelectorKnob()
					&& _display.selectorKnobAction(DisplayMgr::KNOB_CLICK)){
					// was handled - do nothing
				}
				else {
				displayMenu();
				}
			}
		}
	}
	catch ( const Exception& e)  {
		
		// display error on fail..
		
		printf("\tError %d %s\n\n", e.getErrorNumber(), e.what());
		
		if(e.getErrorNumber()	 == ENXIO){
			
		}
	}
	
}
// occasionally called durring idle time

void PiCarMgr::idle(){
									 
#if USE_TMP_117
	_tempSensor1.idle();
#endif
	
	_cpuInfo.idle();
	_fan.idle();
	
#if USE_COMPASS
	_compass.idle();
	if(_compass.isConnected()){
		// handle input
		_compass.rcvResponse([=]( map<string,string> results){
			
			//			for (const auto& [key, value] : results) {
			//				printf("Compass %s = %s\n", key.c_str(), value.c_str());
			//			}
			
			_db.updateValues(results);
		});
	}
#endif
	
	// grab temp data from 1 wire
	{
		map<string,float> temps = {};
		
		if(_w1.getTemperatures(temps)){
			for( auto &[devID, temp]: temps){
				// find it's db entry name
				if(_w1Map.count(devID)){
					w1_map_entry map = _w1Map[devID];
					_db.updateValue(map.dbKey,temp);
				}
			}
			
		}
	}
#if USE_TMP_117
	if(_tempSensor1.isConnected()){
		// handle input
		_tempSensor1.rcvResponse([=]( map<string,string> results){
			_db.updateValues(results);
		});
	}
#endif
	
	if(_cpuInfo.isConnected()){
		// handle input
		_cpuInfo.rcvResponse([=]( map<string,string> results){
			_db.updateValues(results);
		});
	}
	
	if(_fan.isConnected()){
		// handle input
		_fan.rcvResponse([=]( map<string,string> results){
			_db.updateValues(results);
		});
	}
	
	// check for change in dimmer
	
	FrameDB*	fDB 	= can()->frameDB();
	string JK_DIMMER_SW = "JK_DIMMER_SW";
	string DAYTIME = "DAYTIME";
	
	static bool isDayTime = true;
	
	string rawValue;
	if( fDB->valueWithKey(JK_DIMMER_SW, &rawValue)
		&& fDB->boolForKey(DAYTIME, isDayTime) ) {
		
		double dimSW = fDB->normalizedDoubleForValue(JK_DIMMER_SW,rawValue) / 100. ;
		
		// did anything change
		if(_isDayTime	!= isDayTime || dimSW != _dimLevel) {
			
			if(isDayTime){
				_display.setKnobBackLight(false);
				if( _autoDimmerMode ){
					setDimLevel(1.0);
					_display.setBrightness(_dimLevel);
				}
			}
			else {
				_display.setKnobBackLight(true);
				
				if( _autoDimmerMode ){
					if(dimSW != _dimLevel){
						setDimLevel(dimSW);
						// update the brightness
						_display.setBrightness(_dimLevel);
					}
				}
			}
			_isDayTime = isDayTime;
		}
	}
 
	// ocassionally save properties
	saveRadioSettings();
	if(_db.propertiesChanged()){
		_db.savePropertiesToFile();
	}
	
	// check if we need to shutdown
	
	if(_autoShutdownMode && _shutdownDelay > 0) {

		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		int64_t nowSecs = timespec_to_ms(now) / 1000;

		time_t diff = 0;
		
		// if we saw can packets use the last time
		time_t lastTime = 0;
		if(_can.lastFrameTime(PiCarCAN::CAN_ALL, lastTime) && (lastTime != 0)){
 				diff = nowSecs - lastTime ;
		}
		else {
			// we never saw CAN - so use startup time.
			int64_t lastSecs = timespec_to_ms(_lastActivityTime) / 1000;
			diff = nowSecs - lastSecs;
		}
		
		if(diff > _shutdownDelay) {
			// initiate shutdown
			doShutdown();
		}
 	}
 }

void* PiCarMgr::PiCarLoopThread(void *context){
	PiCarMgr* d = (PiCarMgr*)context;
	
	//   the pthread_cleanup_push needs to be balanced with pthread_cleanup_pop
	pthread_cleanup_push(   &PiCarMgr::PiCarLoopThreadCleanup ,context);
	
	d->PiCarLoop();
	
	pthread_exit(NULL);
	
	pthread_cleanup_pop(0);
	return((void *)1);
}


void PiCarMgr::PiCarLoopThreadCleanup(void *context){
	PiCarMgr* d = (PiCarMgr*)context;
	
	// Signal that we're done
	{
		std::unique_lock<std::mutex> lock(d->_shutdownMutex);
		d->_isRunning = false;
		d->_shutdownCV.notify_all();
	}
	
	//	printf("cleanup sdr\n");
}



// MARK: -   Menu Management

PiCarMgr::menu_mode_t PiCarMgr::currentMode(){
	menu_mode_t mode = MENU_UNKNOWN;
	
	switch( _display.active_mode()){
			
		case DisplayMgr::MODE_RADIO:
			if(_radio.isOn()){
				mode = MENU_RADIO;
			}
			break;
			
		case DisplayMgr::MODE_TIME:
			mode = MENU_TIME;
			break;
			
		case DisplayMgr::MODE_GPS:
			mode = MENU_GPS;
			break;

		case DisplayMgr::MODE_GPS_WAYPOINTS:
			mode = MENU_WAYPOINTS;
			break;

		case DisplayMgr::MODE_CANBUS:
			mode = MENU_CANBUS;
			break;
			
		case DisplayMgr::MODE_DTC:
			mode = MENU_DTC;
			break;
			
		default:
			break;
	}
	
	return mode;
}


int PiCarMgr::main_menu_map_offset(PiCarMgr::menu_mode_t mode ){
	int selectedItem = -1;
	
	for(int i = 0; i < _main_menu_map.size(); i++){
		auto e = _main_menu_map[i];
		if(selectedItem == -1)
			if(e.first == mode) selectedItem = i;
	}
	
	return selectedItem;
}


void PiCarMgr::displayMenu(){
	
	constexpr time_t timeout_secs = 10;
	
	vector<string> menu_items = {};
	int selectedItem = -1;
	menu_mode_t mode = currentMode();
	_lastMenuMode = mode;
	
	// if the radio is on.. keep that mode
	if( mode == MENU_RADIO){
		// keep that mode.
	}
	else {
		// fall back the selection for usability
		uint16_t lastSelect;
		if(_db.getUint16Property(PROP_LAST_MENU_SELECTED, &lastSelect)){
			selectedItem = lastSelect;
		}
		else if(selectedItem == -1
				  && (mode == MENU_TIME || mode == MENU_UNKNOWN))
			mode = MENU_RADIO;
	}
	
	menu_items.reserve(_main_menu_map.size());
	
	for(int i = 0; i < _main_menu_map.size(); i++){
		auto e = _main_menu_map[i];
		menu_items.push_back(e.second);
	}
	
	if(selectedItem == -1)
		selectedItem = main_menu_map_offset(mode);
	
	_display.showMenuScreen(menu_items,
									selectedItem,
									"Select Screen",
									timeout_secs,
									[=](bool didSucceed,
										 uint newSelectedItem,
										 DisplayMgr::knob_action_t action ){
		
		if(didSucceed && action == DisplayMgr::KNOB_CLICK) {
			menu_mode_t selectedMode = _main_menu_map[newSelectedItem].first;
			
			if(selectedMode != MENU_UNKNOWN)
				_db.setProperty(PROP_LAST_MENU_SELECTED, to_string(newSelectedItem));
			
			setDisplayMode(selectedMode);
		}
	});
}

void PiCarMgr::setDisplayMode(menu_mode_t menuMode){
	
	switch (menuMode) {
			
		case MENU_RADIO:
			_display.showRadioChange();
			break;
			
		case MENU_SELECT_AUDIO_SOURCE:
			displayRadioMenu();
			break;
			
		case MENU_AUDIO:
			displayAudioMenu();
			break;
 
		case MENU_DEBUG:
			displayDebugMenu();
			break;
  
		case MENU_TIME:
			_display.showTime();
			break;
			
		case MENU_GPS:
			displayGPS();
			break;
			
		case MENU_WAYPOINTS:
			displayWaypoints();
			break;

		case MENU_CANBUS:
			_display.showCANbus(1);
			break;
			
		case MENU_SETTINGS:
			displaySettingsMenu();
			break;
 
		case MENU_DTC:
			_display.showDTC();
			break;
			
		case MENU_EXIT:
			if(_radio.isOn()) {
				if(_radio.isScannerMode())
					_display.showScannerChange();
				else
					_display.showRadioChange();
				
			}
			 else _display.showTime();
			break;
			
			
		case 	MENU_UNKNOWN:
		default:
			// do nothing
			break;
	}
}


void PiCarMgr::displayGPS(){
	_display.showGPS( [=](DisplayMgr::knob_action_t action ){
		if(action == DisplayMgr::KNOB_DOUBLE_CLICK) {
			
			vector<string> menu_items = {
				"Save Waypoint",
				"Exit",
			};
			
			constexpr time_t timeout_secs = 10;

			_display.showMenuScreen(menu_items,
											0,
											"Current Waypoint",
											timeout_secs,
											[=](bool didSucceed,
												 uint newSelectedItem,
												 DisplayMgr::knob_action_t action ){
				
				if(didSucceed) {
					switch(newSelectedItem) {
						case 0:		// new
						{
							waypoint_prop_t  wp;
							
							if(createWaypoint("",wp)){
								_waypoints.push_back(wp);
								sortWaypoints();
								updateWaypointProps();
								_db.savePropertiesToFile();
								
								_display.showMessage("Waypoint Created", 2,[=](){
									displayGPS();
								});
								return;
								
							}
						}
							break;
							
						default:;
					}
				}
 				displayGPS();
 			});
		}
	});
}


void PiCarMgr::displayWaypoints(string intitialUUID){
	
	constexpr time_t timeout_secs = 10;
	
	_display.showWaypoints(intitialUUID, timeout_secs,
								  [=](bool didSucceed,
										string uuid,
										DisplayMgr::knob_action_t action ){
		
		if(!didSucceed){
			displayMenu();
		}
		else if( action == DisplayMgr::KNOB_CLICK) {
			displayWaypoint(uuid);
		}
		else if( action == DisplayMgr::KNOB_DOUBLE_CLICK) {
			waypointEditMenu(uuid);
		}
	});
}


void PiCarMgr::waypointEditMenu(string uuid){
	
	vector<string> menu_items = {
		"Edit Waypoint",
		"Update Waypoint",
		"Delete Waypoint",
		"Exit",
	};
	
	constexpr time_t timeout_secs = 10;
	
	string name = "Waypoint";
	waypoint_prop_t wp;
	
	if(getWaypointInfo(uuid, wp)){
		name = wp.name;
		
	}
	 
	_display.showMenuScreen(menu_items,
									0,
									name,
									timeout_secs,
									[=](bool didSucceed,
										 uint newSelectedItem,
										 DisplayMgr::knob_action_t action ){
		
		if(didSucceed) {
			switch(newSelectedItem) {
				case 0:		// edit
				{
					string name = "";
					
					for( int i = 0; i < _waypoints.size(); i++){
						auto wp = &_waypoints[i];
						if(wp->uuid == uuid){
							name = wp->name;
							break;
						}}
					_display.editString("Edit Waypoint", name,
											  [=](bool didSucceed,
													string newName) {
						if(didSucceed){
							for( int i = 0; i < _waypoints.size(); i++){
								auto wp = &_waypoints[i];
								if(wp->uuid == uuid){
									wp->name = newName;
									updateWaypointProps();
									_db.savePropertiesToFile();
									break;
								}}
						}
						displayWaypoint(uuid);
					});
				}
					
					break;
				case 1:		// update
					updateWaypoint(uuid);
					displayWaypoint(uuid);
					break;
					
				case 2:		//delete
					deleteWaypoint(uuid);
					displayWaypoints();
					break;
					
				default:
					displayWaypoint(uuid);
			}
		}
	});
	
}


void PiCarMgr::displayWaypoint(string uuid){
	// push selected waypoints
	_display.showWaypoint(uuid,[=](bool didSucceed,
											 string uuid,
											 DisplayMgr::knob_action_t action ){
		
		if(didSucceed) {
		
			if(action == DisplayMgr::KNOB_CLICK) {
				displayWaypoints(uuid);
			}
	
			else if(action == DisplayMgr::KNOB_DOUBLE_CLICK) {
				waypointEditMenu(uuid);
			}
		}
	});
}

void PiCarMgr::displayRadioMenu() {
    if(!_radio.isOn()) return;
    
    RadioMgr::radio_mode_t mode;
    uint32_t freq;
    _radio.queueGetFrequencyandMode(mode, freq);
    
    vector<string> items = {"Tune", "Presets", "Scanner"};
    _display.showMenuScreen(items, 0, "Radio Menu", 0, [=](bool didSucceed, uint selectedIndex, DisplayMgr::knob_action_t action) {
        if(didSucceed && action == DisplayMgr::KNOB_CLICK) {
            switch(selectedIndex) {
                case 0: // Tune
                    _radio.setFrequencyandMode(mode, freq);
                    break;
                    
                case 1: // Presets
                    displayScannerChannels({mode,freq});
                    break;
                    
                case 2: // Scanner
                    _radio.scanChannels(_scanner_freqs);
                    break;
            }
        }
    });
}

void PiCarMgr::displayDebugMenu() {
    vector<string> items = {"System Info", "Radio Debug", "Display Test"};
    
    _display.showMenuScreen(items, 0, "Debug Menu", 0, [=](bool didSucceed, uint selectedIndex, DisplayMgr::knob_action_t action) {
        if(didSucceed && action == DisplayMgr::KNOB_CLICK) {
            switch(selectedIndex) {
                case 0: // System Info
                    setDisplayMode(MENU_DEBUG);
                    break;
                    
                case 1: // Radio Debug
                    // Add radio debug functionality
                    break;
                    
                case 2: // Display Test
                    _display.showMessage("Display Test");
                    break;
            }
        }
    });
}

vector<string> PiCarMgr::settingsMenuItems(){
	string dim_entry = _autoDimmerMode ? "Dim Screen (auto)": "Dim Screen";
 
	vector<string> menu_items = {
		dim_entry,
		"Shutdown Delay",
		"Set ECU Time",
		"Info",
		"Exit",
	};

	return menu_items;
}

void PiCarMgr::displaySettingsMenu(){
	
	constexpr time_t timeout_secs = 10;
		
	_display.showMenuScreen(settingsMenuItems(),
									2,
									"Settings",
									timeout_secs,
									[=](bool didSucceed,
										 uint newSelectedItem,
										 DisplayMgr::knob_action_t action ){
		
		if(didSucceed) {
			
			if(action){
				switch (newSelectedItem) {
						
					case 0:
						if(action == DisplayMgr::KNOB_CLICK){
							if(!_autoDimmerMode)
								_display.showDimmerChange();
 						}
						else if(action == DisplayMgr::KNOB_DOUBLE_CLICK){
							_autoDimmerMode = !_autoDimmerMode;
							_display.updateMenuItems(settingsMenuItems());
 						}
						break;

					case 1:
						displayShutdownMenu();
						break;
						
					case 2:
					{
						struct timespec  gpsTime;
						
						bool success = false;
						if( _gps.GetTime(gpsTime)) {
							success = setECUtime(gpsTime);
						}
						_display.showMessage( success?"Set Time Succeeed":"Set Time Failed" , 2,[=](){
							displayMenu();
						});
						
					}
						break;

					case 3:
						_display.showInfo();
						break;
 
			 
					default:
						displayMenu();
						break;
				}
				
			}
			
		}
	});
	
}

void PiCarMgr::displayShutdownMenu() {
    vector<string> items = {"1 min", "5 min", "10 min", "30 min", "Never"};
    
    _display.showMenuScreen(items, 0, "Shutdown Delay", 0, [=](bool didSucceed, uint selectedIndex, DisplayMgr::knob_action_t action) {
        if(didSucceed && action == DisplayMgr::KNOB_CLICK) {
            uint16_t delaySeconds = 0;
            switch(selectedIndex) {
                case 0: // 1 min
                    delaySeconds = 60;
                    break;
                case 1: // 5 min
                    delaySeconds = 300;
                    break;
                case 2: // 10 min
                    delaySeconds = 600;
                    break;
                case 3: // 30 min
                    delaySeconds = 1800;
                    break;
                case 4: // Never
                    delaySeconds = 0;
                    break;
            }
            _shutdownDelay = delaySeconds;
            _autoShutdownMode = (delaySeconds > 0);
            displaySettingsMenu();
        }
    });
}

void PiCarMgr::showVolumeChange() {
    _display.showSliderScreen(
        "Volume",
        "Max",
        "Min",
        0,
        [=]() { return _audio.volume(); },
        [=](double val) { _audio.setVolume(val); }
    );
}

void PiCarMgr::showBalanceChange() {
    _display.showSliderScreen(
        "Balance",
        "Right",
        "Left",
        0,
        [=]() { return _audio.balance(); },
        [=](double val) { _audio.setBalance(val); }
    );
}

void PiCarMgr::showFaderChange() {
    _display.showSliderScreen(
        "Fader",
        "Front",
        "Rear",
        0,
        [=]() { return _audio.fader(); },
        [=](double val) { _audio.setFader(val); }
    );
}

void PiCarMgr::showBassChange() {
    _display.showSliderScreen(
        "Bass",
        "Max",
        "Min",
        0,
        [=]() { return _audio.bass(); },
        [=](double val) { _audio.setBass(val); }
    );
}

void PiCarMgr::showTrebleChange() {
    _display.showSliderScreen(
        "Treble",
        "Max",
        "Min",
        0,
        [=]() { return _audio.treble(); },
        [=](double val) { _audio.setTreble(val); }
    );
}

void PiCarMgr::showMidrangeChange() {
    _display.showSliderScreen(
        "Midrange",
        "Max",
        "Min",
        0,
        [=]() { return _audio.midrange(); },
        [=](double val) { _audio.setMidrange(val); }
    );
}

void PiCarMgr::displayAudioMenu() {
    vector<string> menu_items = {
        "Volume",
        "Balance",
        "Fader",
        "Bass",
        "Treble",
        "Midrange",
        "Exit"
    };

    _display.showMenuScreen(menu_items, 0, "Audio Menu", 0, [=](bool didSucceed, uint selectedIndex, DisplayMgr::knob_action_t action) {
        if (didSucceed && action == DisplayMgr::KNOB_CLICK) {
            switch (selectedIndex) {
                case 0: // Volume
                    showVolumeChange();
                    break;

                case 1: // Balance
                    showBalanceChange();
                    break;

                case 2: // Fader
                    showFaderChange();
                    break;

                case 3: // Bass
                    showBassChange();
                    break;

                case 4: // Treble
                    showTrebleChange();
                    break;

                case 5: // Midrange
                    showMidrangeChange();
                    break;

                default:
                    break;
            }
        }
    });
}

void PiCarMgr::scannerDoubleClicked(){
	
	if(_radio.isScannerMode()){
		
		_radio.pauseScan(true);
		
		RadioMgr::radio_mode_t  mode  = _radio.radioMode();
		uint32_t						freq;
		_radio.queueGetFrequencyandMode(mode, freq);  // Use queueGetFrequencyandMode instead of getCurrentFrequencyandMode
		
		displayScannerChannels({mode,freq});
	}
		 
}
 
void PiCarMgr::scannerChannelMenu(RadioMgr::channel_t selectedChannel ){
	
	RadioMgr::radio_mode_t  mode  = selectedChannel.first;
	uint32_t 					freq =  selectedChannel.second;
	constexpr time_t timeout_secs = 10;

	vector<string> menu_items = {
		isScannerChannel(mode, freq)?"Remove":"Add",
		"-",
		"Exit"
	};
	
	_display.showMenuScreen(menu_items,
									2,
									"Scanner List",
									timeout_secs,
									[=](bool didSucceed,
										 uint newSelectedItem,
										 DisplayMgr::knob_action_t action ){
		
		if(didSucceed && action == DisplayMgr::KNOB_CLICK) {
			switch (newSelectedItem) {
				case 0:
					// remove/ add
				{
					if(isScannerChannel(mode, freq)){
						
						if(clearScannerChannel(mode, freq))
							saveRadioSettings();
					}
					else
					{
						if(setScannerChannel(mode, freq))
							saveRadioSettings();
					}
				}
				default:
					break;
 			};
		};
		
		displayScannerChannels(selectedChannel);
	});
	
}


void PiCarMgr::displayScannerChannels(RadioMgr::channel_t selectedChannel ){
	constexpr time_t timeout_secs = 20;
	
	_radio.pauseScan(true);
	
	_display.showScannerChannels(selectedChannel,
										  timeout_secs,
										  [=](bool didSucceed,
												RadioMgr::channel_t selectedChannel,
												DisplayMgr::knob_action_t action ){
			if(didSucceed) {
			if(action == DisplayMgr::KNOB_CLICK) {
		 
				_display.showChannel(selectedChannel, [=](bool didSucceed,
																		RadioMgr::channel_t channel,
																		DisplayMgr::knob_action_t action ){
					
					if(action == DisplayMgr::KNOB_CLICK) {
						displayScannerChannels(channel);
					}
					else if(action == DisplayMgr::KNOB_DOUBLE_CLICK) {
						scannerChannelMenu(channel);
					}
					else
					{
						_radio.pauseScan(false);
					}
				});
				
				return;
			}
			else if(action == DisplayMgr::KNOB_DOUBLE_CLICK){
				scannerChannelMenu(selectedChannel);
				return;
			}
			else if(action ==  DisplayMgr::KNOB_SELECTING){
				_radio.tuneScannerToChannel(selectedChannel);
				return;
			}
		}
	
 		_radio.pauseScan(false);
		
	});
	
}
 
void PiCarMgr::tunerDoubleClicked(){
	DisplayMgr::mode_state_t dMode = _display.active_mode();
	
	if( _radio.isOn()) {
		
		if(_radio.isScannerMode()){
			// display scanner menu
			scannerDoubleClicked();
		}
		else if(dMode == DisplayMgr::MODE_RADIO ){
			// display set/preset menu
			
			RadioMgr::radio_mode_t  mode;
			uint32_t						freq;
			_radio.queueGetFrequencyandMode(mode, freq);  // Use queueGetFrequencyandMode instead of getCurrentFrequencyandMode
			
			displayScannerChannels({mode,freq});
		}
	}
		 
}
 
// MARK: -  Knobs and Buttons

void PiCarMgr::updateEncoders(){
	
	if(!_isSetup){
		return;
	}
	
	if(_volKnob){
		bool clockwise = false;
		
		if(_volKnob->updateStatus()){
			
			if(_volKnob->wasMoved(clockwise)){
				if(clockwise)
					volumeUp();
				else
					volumeDown();
			}
			
			if(_volKnob->wasClicked()){
				toggleMute();
			}
		}
	}
	
	if(_tunerKnob){
		bool clockwise = false;
		
		if(_tunerKnob->updateStatus()){
			
			if(_tunerKnob->wasMoved(clockwise)){
				if(clockwise)
					tunerUp();
				else
					tunerDown();
			}
			
			if(_tunerKnob->wasClicked()){
				toggleTunerMenu();
			}
		}
	}
}

void PiCarMgr::volumeUp() {
    // Implement volume up logic
    double vol = _audio.volume();
    _audio.setVolume(vol + 0.1); // Increase by 10%
    showVolumeChange();
}

void PiCarMgr::volumeDown() {
    // Implement volume down logic
    double vol = _audio.volume();
    _audio.setVolume(vol - 0.1); // Decrease by 10%
    showVolumeChange();
}

void PiCarMgr::toggleMute() {
    // Implement mute toggle logic
    _audio.setMute(!_audio.isMuted());
    showVolumeChange();
}

void PiCarMgr::tunerUp() {
    // Implement tuner up logic
    switch (_tuner_mode) {
        case TUNE_ALL:
            _radio.nextFrequency(true);
            break;
            
        case TUNE_KNOWN: {
            station_info_t info;
            RadioMgr::radio_mode_t mode;
            uint32_t freq;
            if (_radio.queueGetFrequencyandMode(mode, freq) && 
                nextKnownStation(mode, freq, true, info)) {
                _radio.setFrequencyandMode(mode, info.frequency);
            }
            break;
        }
            
        case TUNE_PRESETS: {
            station_info_t info;
            RadioMgr::radio_mode_t mode;
            uint32_t freq;
            if (_radio.queueGetFrequencyandMode(mode, freq) && 
                nextPresetStation(mode, freq, true, info)) {
                _radio.setFrequencyandMode(mode, info.frequency);
            }
            break;
        }
    }
}

void PiCarMgr::tunerDown() {
    // Implement tuner down logic
    switch (_tuner_mode) {
        case TUNE_ALL:
            _radio.nextFrequency(false);
            break;
            
        case TUNE_KNOWN: {
            station_info_t info;
            RadioMgr::radio_mode_t mode;
            uint32_t freq;
            if (_radio.queueGetFrequencyandMode(mode, freq) && 
                nextKnownStation(mode, freq, false, info)) {
                _radio.setFrequencyandMode(mode, info.frequency);
            }
            break;
        }
            
        case TUNE_PRESETS: {
            station_info_t info;
            RadioMgr::radio_mode_t mode;
            uint32_t freq;
            if (_radio.queueGetFrequencyandMode(mode, freq) && 
                nextPresetStation(mode, freq, false, info)) {
                _radio.setFrequencyandMode(mode, info.frequency);
            }
            break;
        }
    }
}

void PiCarMgr::toggleTunerMenu() {
    // Implement tuner menu toggle logic
    switch (_tuner_mode) {
        case TUNE_ALL:
            _tuner_mode = TUNE_KNOWN;
            break;
            
        case TUNE_KNOWN:
            _tuner_mode = TUNE_PRESETS;
            break;
            
        case TUNE_PRESETS:
            _tuner_mode = TUNE_ALL;
            break;
    }
    displayRadioMenu();
}
 
void PiCarMgr::startControls( std::function<void(bool didSucceed, std::string error_text)> cb){
	int  errnum = 0;
	bool didSucceed = false;
	
	// setup GPIO lines
	_gpio_chip = gpiod_chip_open(gpioPath);
	if(!_gpio_chip) {
		ELOG_MESSAGE("Error open GPIO chip(\"%s\") : %s \n",gpioPath,strerror(errno));
		goto done;
	}

	_gpio_relay1 = gpiod_chip_get_line(_gpio_chip, gpio_relay1_line_number);
	if(!_gpio_relay1) {
		ELOG_MESSAGE("Error open GPIO line(%d) : %s \n",gpio_relay1_line_number,strerror(errno));
		goto done;
	}

	if( gpiod_line_request_output(_gpio_relay1, "gpio_relay_1", 0) != 0){
		ELOG_MESSAGE("Error gpiod_line_request_output line(%d) : %s \n",gpio_relay1_line_number,strerror(errno));
		goto done;
	}

	
#if USE_GPIO_INTERRUPT
	// get refs to the lines
	_gpio_line_int =  gpiod_chip_get_line(_gpio_chip, gpio_int_line_number);
	if(!_gpio_line_int){
		ELOG_MESSAGE("Error gpiod_chip_get_line %d: %s \n",  gpio_int_line_number, strerror(errno));
		goto done;
	}
	
	// setup the l;ine for input and select pull up resistor
	if(  gpiod_line_request_falling_edge_events_flags(_gpio_line_int,
																	  GPIOD_CONSUMER,
																	  GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP) != 0){
		ELOG_MESSAGE("Error gpiod_line_request_falling_edge_events %d: %s \n",  gpio_int_line_number, strerror(errno));
		goto done;
	}
	
	
#endif
	
	didSucceed = true;
done:
	
	if(cb)
		(cb)(didSucceed, didSucceed?"": string(strerror(errnum) ));
}

void PiCarMgr::stopControls(){
	
#if USE_GPIO_INTERRUPT
	if(_gpio_line_int)
		gpiod_line_release (_gpio_line_int);
	_gpio_line_int = NULL;
#endif
	
	if(_gpio_relay1)
		gpiod_line_release(_gpio_relay1);
	_gpio_relay1 = NULL;
 
	if(_gpio_chip)
		gpiod_chip_close(_gpio_chip);
	_gpio_chip = NULL;
}

void PiCarMgr::setDimLevel(double dimLevel){
	
	if(dimLevel < 0) dimLevel = 0;
	_dimLevel = dimLevel;
}

double PiCarMgr::dimLevel(){
	return _dimLevel;
}

bool PiCarMgr::setRelay1(bool state){
	bool didSucceed = false;
	
	if(_gpio_relay1) {
		gpiod_line_set_value(_gpio_relay1, state);
	}
	
	return didSucceed;
}




// MARK: -   Devices

void PiCarMgr::startCPUInfo( std::function<void(bool didSucceed, std::string error_text)> cb){
	
	int  errnum = 0;
	bool didSucceed = false;
	
	didSucceed =  _cpuInfo.begin(errnum);
	if(didSucceed){
		
		uint16_t queryDelay = 0;
		if(_db.getUint16Property(PROP_CPU_TEMP_QUERY_DELAY, &queryDelay)){
			_cpuInfo.setQueryDelay(queryDelay);
		}
		
	}
	else {
		ELOG_ERROR(ErrorMgr::FAC_SENSOR, 0, errnum,  "Start CPUInfo");
	}
	
	
	if(cb)
		(cb)(didSucceed, didSucceed?"": string(strerror(errnum) ));
}

void PiCarMgr::stopCPUInfo(){
	_cpuInfo.stop();
	
}

void PiCarMgr::startFan( std::function<void(bool didSucceed, std::string error_text)> cb){
	
	int  errnum = 0;
	bool didSucceed = false;
	
	didSucceed =  _fan.begin(errnum);
	if(didSucceed){
		
		uint16_t queryDelay = 0;
		if(_db.getUint16Property(PROP_FAN_QUERY_DELAY, &queryDelay)){
			_fan.setQueryDelay(queryDelay);
		}
		
	}
	else {
		ELOG_ERROR(ErrorMgr::FAC_SENSOR, 0, errnum,  "Start ArgononeFan");
	}
	
	
	if(cb)
		(cb)(didSucceed, didSucceed?"": string(strerror(errnum) ));
}

void PiCarMgr::stopFan(){
	_fan.stop();
}


#if USE_TMP_117


void PiCarMgr::startTempSensors( std::function<void(bool didSucceed, std::string error_text)> cb){
	
	int  errnum = 0;
	bool didSucceed = false;
	
	
	uint8_t deviceAddress = 0x4A;
	
	didSucceed =  _tempSensor1.begin(deviceAddress, VAL_OUTSIDE_TEMP, errnum);
	if(didSucceed){
		
		uint16_t queryDelay = 0;
		if(_db.getUint16Property(PROP_TEMPSENSOR_QUERY_DELAY, &queryDelay)){
			_tempSensor1.setQueryDelay(queryDelay);
		}
		
		//		_db.addSchema(resultKey,
		//						  CoopMgrDB::DEGREES_C, 2,
		//						  "Coop Temperature",
		//						  CoopMgrDB::TR_TRACK);
		
		//		LOGT_DEBUG("Start TempSensor 1 - OK");
	}
	else {
		ELOG_ERROR(ErrorMgr::FAC_SENSOR, deviceAddress, errnum,  "Start TempSensor 1 ");
	}
	
	
	if(cb)
		(cb)(didSucceed, didSucceed?"": string(strerror(errnum) ));
}

void PiCarMgr::stopTempSensors(){
	_tempSensor1.stop();
}

PiCarMgrDevice::device_state_t PiCarMgr::tempSensor1State(){
	return _tempSensor1.getDeviceState();
}
#endif

// MARK: -   I2C Compass
#if USE_COMPASS

void PiCarMgr::startCompass ( std::function<void(bool didSucceed, std::string error_text)> cb){
	
	int  errnum = 0;
	bool didSucceed = false;
	
	
	uint8_t deviceAddress = 0;  // use default
	
	didSucceed =  _compass.begin(deviceAddress, errnum);
	if(didSucceed){
		
		uint16_t queryDelay = 0;
		if(_db.getUint16Property(PROP_COMPASS_QUERY_DELAY, &queryDelay)){
			_compass.setQueryDelay(queryDelay);
		}
	}
	else {
		ELOG_ERROR(ErrorMgr::FAC_SENSOR, deviceAddress, errnum,  "Start Compass");
	}
	
	
	if(cb)
		(cb)(didSucceed, didSucceed?"": string(strerror(errnum) ));
}

void PiCarMgr::stopCompass(){
	_compass.stop();
}



PiCarMgrDevice::device_state_t PiCarMgr::compassState(){
	return _compass.getDeviceState();
}

#endif

// MARK: -   Network and system interface


bool  PiCarMgr::hasWifi(stringvector *ifnames){
	bool  has_wifi = false;
	
	struct ifaddrs *ifaddr, *ifa;
	int family, n;
	
	if(getifaddrs(&ifaddr) == 0){
		
		stringvector names;
		for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
			if (ifa->ifa_addr == NULL)
				continue;
			
			if(strncmp( ifa->ifa_name, "wlan", 4) == 0){
				
				string name = string(ifa->ifa_name);
				
				family = ifa->ifa_addr->sa_family;
				
				if (family == AF_INET || family == AF_INET6) {
					
					if (std::find(names.begin(), names.end(),name) == names.end()){
						names.push_back(string(ifa->ifa_name));
					}
				}
			}
		}
		
		freeifaddrs(ifaddr);
		
		if( names.size() > 0){
			
			*ifnames = names;
			has_wifi = true;
		}
	}
 
	return has_wifi;
}

bool PiCarMgr::isAirPlayRunning(){
	bool isRunning = (getPidByName("shairport-sync") > 0);
	return isRunning;
}


// MARK: - realtime clock sync


bool 	PiCarMgr::shouldSyncClockToGPS(time_t &deviation){
	if( _clocksync_gps ){
		deviation = _clocksync_gps_secs;
		return true;
	}
	deviation = 0;
	return false;
}
 
bool PiCarMgr::setRTC(struct timespec gpsTime ){
	
	bool success = false;
	int r = clock_settime(CLOCK_REALTIME, &gpsTime);
	if(r == 0){
 		struct tm *tm = localtime(&gpsTime.tv_sec);
 		LOGT_INFO("System Clock synced to GPS %d:%d:%d",tm->tm_hour, tm->tm_min, tm->tm_sec );
		success = true;
	}
	else {
		ELOG_ERROR(ErrorMgr::FAC_GPS, 0, errno, "clock sync failed");
	}
 
	return success;
}
 

bool PiCarMgr::setECUtime(  struct timespec ts){
	bool success = false;
 	struct tm *tm = localtime(&ts.tv_sec);
 
	LOGT_INFO("Set ECU Clock %d:%d:%d",tm->tm_hour, tm->tm_min, tm->tm_sec );

	success = _can.sendFrame(PiCarCAN::CAN_JEEP, 0x2e9, {0x03,
		static_cast<uint8_t>(tm->tm_hour),
		static_cast<uint8_t>(tm->tm_min),
		static_cast<uint8_t>(tm->tm_sec)});
 
	return success;

}

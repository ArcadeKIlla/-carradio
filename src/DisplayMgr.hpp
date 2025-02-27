//
//  DisplayMgr.hpp
//  vfdtest
//
//  Created by Vincent Moscaritolo on 4/25/22.
//

#pragma once

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <mutex>
#include <bitset>
#include <utility>      // std::pair, std::make_pair
#include <string>       // std::string
#include <queue>

#include <sys/time.h>

#include "VFD.hpp"
#include "SSD1306_VFD.hpp"
#include "U8G2_VFD.hpp"
#include "ErrorMgr.hpp"
#include "CommonDefs.hpp"
#include "DuppaLEDRing.hpp"
#include "DuppaKnob.hpp"
#include "RadioMgr.hpp"
#include "GPSmgr.hpp"
#include "EncoderBase.hpp"
#include "GenericEncoder.hpp"

using namespace std;

class DisplayMgr {
	
public:
	// LED event constants
	static const uint32_t LED_EVENT_SCAN_STEP = 0x01;
	static const uint32_t LED_EVENT_SCAN_HOLD = 0x02;
	static const uint32_t LED_EVENT_SCAN_MASK = 0x03;

	enum DisplayType {
		VFD_DISPLAY,
		OLED_DISPLAY,
		U8G2_OLED_DISPLAY
	};
	
	enum EncoderType {
		DUPPA_ENCODER,
		GENERIC_ENCODER
	};

	struct EncoderConfig {
		EncoderType type;
		union {
			struct {
				uint8_t address;
			} duppa;
			struct {
				int clkPin;
				int dtPin;
				int swPin;
			} generic;
		};
	};

	DisplayMgr(DisplayType displayType = VFD_DISPLAY,
               EncoderConfig leftConfig = {DUPPA_ENCODER, {.duppa = {0x40}}},
               EncoderConfig rightConfig = {DUPPA_ENCODER, {.duppa = {0x41}}});
	~DisplayMgr();
		
	bool begin(const char* path, speed_t speed =  B19200);
	bool begin(const char* path, speed_t speed, int &error);
	void stop();

	bool reset();
	
	// Check if display is available
	bool hasDisplay() const { return _vfd && _vfd->isSetup(); }
	
	// Clear the display screen
	bool clearScreen() { return _vfd ? _vfd->clearScreen() : false; }
		
	// LED effects
	void LEDeventStartup();
	void LEDeventVol();
	void LEDeventMute();
	void LEDeventStop();
	void LEDTunerUp(bool pinned = false);
	void LEDTunerDown(bool pinned = false);
 
	void LEDeventScannerStep();
	void LEDeventScannerHold();
	void LEDeventScannerStop();
	 
// active mode
	typedef enum  {
		MODE_UNKNOWN = 0,
		MODE_NOCHANGE,	  // no change in mode
		
		MODE_STARTUP,
		MODE_TIME,
		MODE_DIMMER,
		MODE_SQUELCH,
		MODE_RADIO,
		MODE_SCANNER,
		MODE_CANBUS,
		MODE_GPS,
		MODE_GPS_WAYPOINTS,
		MODE_GPS_WAYPOINT,
		MODE_DTC,
		MODE_DTC_INFO,
		MODE_INFO,
		MODE_DEV_STATUS,
		MODE_MENU,
		MODE_MESSAGE,
		MODE_EDIT_STRING,
		MODE_SCANNER_CHANNELS,
		MODE_CHANNEL_INFO,
		MODE_SLIDER,
		MODE_SELECT_SLIDER
	}mode_state_t;

	mode_state_t active_mode();
 
	// knobs
	
	typedef enum  {
		KNOB_EXIT = 0,
		KNOB_UP,
		KNOB_DOWN,
		KNOB_CLICK,
		KNOB_DOUBLE_CLICK,
 		KNOB_SELECTING 	// special callback for things like scanner channels
	}knob_action_t;
 
	typedef enum  {
		KNOB_RIGHT,
		KNOB_LEFT,
 	}knob_id_t;

	// display related
	bool setBrightness(double level);   // 0.0 -  1.0
	bool setKnobBackLight(bool isOn);    

	bool setKnobColor(knob_id_t, RGB);
	
	// multi page display
	bool usesSelectorKnob();
	bool selectorKnobAction(knob_action_t action);

	void showTime();

	void showMessage(string message = "", time_t timeout = 0, voidCallback_t cb = nullptr );
	
	typedef std::function<void(knob_action_t action)> knobCallBack_t;
	void showGPS(knobCallBack_t cb = nullptr);

	typedef std::function<void(bool didSucceed,
										string uuid,
										knob_action_t action)> showWaypointsCallBack_t;
	
	void showWaypoints( string intitialUUID,
 							 time_t timeout = 0,
							 showWaypointsCallBack_t cb = nullptr);
	
	void showWaypoint(string uuid,  showWaypointsCallBack_t cb = nullptr) ;

	
	typedef std::function<void(bool didSucceed,
										RadioMgr::channel_t channel,
										knob_action_t action)> showScannerChannelsCallBack_t;

	void showScannerChannels( RadioMgr::channel_t initialChannel = {RadioMgr::MODE_UNKNOWN, 0},
							 		time_t timeout = 0,
									 showScannerChannelsCallBack_t cb = nullptr);
 
	typedef std::function<void(bool didSucceed,
										RadioMgr::channel_t channel,
										knob_action_t action)> showChannelCallBack_t;

	void showChannel( RadioMgr::channel_t channel,
						  showChannelCallBack_t cb = nullptr);
  
	typedef std::function<void(bool didSucceed,
										string strOut)> editStringCallBack_t;

	void editString(string title = "", string strIn = "",
						 editStringCallBack_t cb = nullptr);

	void showStartup();
	void showInfo(time_t timeout = 0);
	void showDTC();
	void showDTCInfo(string code);
 
	void showDimmerChange();	
	void showBalanceChange();
	void showFaderChange();
	void showSquelchChange();
	
	typedef std::function<void(double)> menuSliderSetterCallBack_t;
	typedef std::function<double()> menuSliderGetterCallBack_t;

	void showSliderScreen(
								 string title,
								 string right_text 	= "R",
								 string left_text 	= "L",
								 time_t timeout = 0,
								 menuSliderGetterCallBack_t getterCB = nullptr,
								 menuSliderSetterCallBack_t setCB = nullptr,
 								 boolCallback_t doneCB  = nullptr);
	
	typedef std::function<void(int)>menuSelectionSilderSetterCallBack_t;
	
	void showSelectionSilderScreen(
								 string title,
								 std::vector<string> choices,
								 int initialChoice = 0,
 								 time_t timeout = 0,
								 menuSelectionSilderSetterCallBack_t setCB = nullptr,
								 boolCallback_t doneCB  = nullptr);
 
	void showRadioChange();
	void showScannerChange(bool force = true);
	void showAirplayChange();
	void enableAutoPlay(bool val) { _shouldAutoPlay	= val;};
 
	void showCANbus(uint8_t page = 0);
	 
	void showDevStatus();

 	// Menu Screen Management
	typedef string menuItem_t;
	typedef std::function<void(bool didSucceed, uint selectedItemID, knob_action_t action)> menuSelectedCallBack_t;
	void showMenuScreen(vector<menuItem_t> items,
							  uint intitialItem,
							  string title,
							  time_t timeout = 0,
							  menuSelectedCallBack_t cb = nullptr);
 
	
	void updateMenuItems(vector<menuItem_t> items);   // can be called from menuSelectedCallBack_t
	
	void clearAPMetaData();

	DuppaKnob* rightKnob() { return _rightKnob; }
    DuppaKnob* leftKnob() { return _leftKnob; }
    EncoderBase* rightEncoder() { return _rightEncoder; }
    EncoderBase* leftEncoder() { return _leftEncoder; }

private:
	
	typedef enum  {
		EVT_NONE = 0,
		EVT_PUSH,
		EVT_POP,
		EVT_REDRAW,
 	}event_t;

	typedef enum  {
		TRANS_ENTERING = 0,
		TRANS_REFRESH,
		TRANS_IDLE,
		TRANS_LEAVING,
	}modeTransition_t;
		
	bool isMultiPage(mode_state_t mode);
	bool processSelectorKnobAction( knob_action_t action);
	
	uint8_t pageCountForMode(mode_state_t mode);
 
	void drawMode(modeTransition_t transition, mode_state_t mode, string eventArg = "");
	void drawStartupScreen(modeTransition_t transition);
	void drawDeviceStatusScreen(modeTransition_t transition);
	void drawTimeScreen(modeTransition_t transition);
	void drawDimmerScreen(modeTransition_t transition);

	void drawSliderScreen(modeTransition_t transition);
	bool processSelectorKnobActionForSlider( knob_action_t action);

	void drawSelectSliderScreen(modeTransition_t transition);
	bool processSelectorKnobActionForSelectSlider( knob_action_t action);
 
	void drawSquelchScreen(modeTransition_t transition);
	bool processSelectorKnobActionForSquelch( knob_action_t action);
	bool processSelectorKnobActionForDimmer( knob_action_t action);
	bool processSelectorKnobActionForDTC( knob_action_t action);
	bool processSelectorKnobActionForGPS( knob_action_t action);
	bool processSelectorKnobActionForGPSWaypoints( knob_action_t action);
	bool processSelectorKnobActionForGPSWaypoint( knob_action_t action);
	bool processSelectorKnobActionForEditString( knob_action_t action);
	bool processSelectorKnobActionForScannerChannels( knob_action_t action);
	bool processSelectorKnobActionForChannelInfo( knob_action_t action);
	bool processSelectorKnobActionForInfo( knob_action_t action);
	 
	void drawRadioScreen(modeTransition_t transition);
	void drawScannerScreen(modeTransition_t transition);
 
	void drawGPSScreen(modeTransition_t transition);
 
	void drawCANBusScreen(modeTransition_t transition);
	void drawCANBusScreen1(modeTransition_t transition);

 	void drawDTCScreen(modeTransition_t transition);
 	void drawDTCInfoScreen(modeTransition_t transition, string code);
	bool processSelectorKnobActionForDTCInfo( knob_action_t action);
 
	void drawSettingsScreen(modeTransition_t transition);
	
	void drawInfoScreen(modeTransition_t transition);
 	void drawMessageScreen(modeTransition_t transition);

	void drawInternalError(modeTransition_t transition);
 
	void drawShutdownScreen();
	void drawDeviceStatus();
	
	void drawEngineCheck();
	void drawTemperature();
	void drawAirplayLogo(uint8_t x, uint8_t y, string text = "");
	
	void drawReceptionBars(uint8_t x,  uint8_t y, double dBm = 0, bool displayNumber = false );

	void drawTimeBox();
	void drawEditStringScreen(modeTransition_t transition);

	// waypoint stuff
	void drawGPSWaypointsScreen(modeTransition_t transition);
	void drawGPSWaypointScreen(modeTransition_t transition);

	//chanel management stuff
	void drawScannerChannels(modeTransition_t transition);
	void drawChannelInfo(modeTransition_t transition);
 
	typedef struct {
		string title;
		string right_text;
		string left_text;
		time_t timeout;
		menuSliderGetterCallBack_t getCB;
		menuSliderSetterCallBack_t setCB;
		boolCallback_t doneCB;
	} menuSliderCBInfo_t;
	
	menuSliderCBInfo_t * _menuSliderCBInfo = NULL;
  
	typedef struct {
		string title;
 		time_t timeout;
		std::vector<string> choices;
		int currentChoice;
 		menuSelectionSilderSetterCallBack_t setCB;
		boolCallback_t doneCB;
	} menuSelectionSliderCBInfo_t;

	menuSelectionSliderCBInfo_t * _menuSelectionSliderCBInfo = NULL;
 
	
	showWaypointsCallBack_t _wayPointCB;
	showScannerChannelsCallBack_t _scannnerChannelsCB;
	knobCallBack_t _knobCB;
	voidCallback_t	_simpleCB;
	
	editStringCallBack_t _editCB;
	string				 	_editString;
	bool 						_isEditing;
	int 						_editChoice;
	
	showChannelCallBack_t	_showChannelCB;
	RadioMgr::channel_t		_currentChannel;
	
// display value formatting
 	bool normalizeCANvalue(string key, string & value);
	
//Menu stuff
	void resetMenu();
	bool menuSelectAction(knob_action_t action);
	void drawMenuScreen(modeTransition_t transition);

	vector<menuItem_t>	_menuItems;
	int						_currentMenuItem;
	int						_menuCursor;			// item at top of screen

	time_t					 _menuTimeout;
	menuSelectedCallBack_t _menuCB;
	string					  _menuTitle;
	
	uint8_t   	_lineOffset = 0;	// used for multi-line
	
	mode_state_t _current_mode = MODE_UNKNOWN;
	uint8_t		 _currentPage  = 0;			// used for multipage
	mode_state_t _saved_mode   = MODE_UNKNOWN;
	
	struct timespec	_lastEventTime = {0,0};
 
	mode_state_t handleRadioEvent();
	bool isStickyMode(mode_state_t);
	bool pushMode(mode_state_t);
	void popMode();
	
	
	void setEvent(event_t event, mode_state_t mode = MODE_UNKNOWN, string arg = "");
 
	// MARK: - LED EFFECTS
	// LED effects Bit map
	
#define LED_EVENT_ALL					0xFFFFFFFFFFFFFFFF
#define LED_EVENT_MASK					0x00000000FFFFFFFF
#define LED_STATUS_MASK					0xFFFFFFFF00000000

#define LED_EVENT_STARTUP				0x0000000000000001
#define LED_EVENT_VOL 					0x0000000000000002
#define LED_EVENT_MUTE					0x0000000000000004
	
#define LED_EVENT_SCAN_STEP			0x0000000000000008
#define LED_EVENT_SCAN_HOLD			0x0000000000000010
#define LED_EVENT_SCAN_STOP			0x0000000000000020
	
#define LED_EVENT_TUNE_UP				0x0000000000000040
#define LED_EVENT_TUNE_DOWN			0x0000000000000080
#define LED_EVENT_TUNE_UP_PIN			0x0000000000000100
#define LED_EVENT_TUNE_DOWN_PIN		0x0000000000000200

	
#define LED_EVENT_STOP					0x0000000080000000
	
#define LED_EVENT_STARTUP_RUNNING	0x0000000100000000
#define LED_EVENT_VOL_RUNNING			0x0000000200000000
#define LED_EVENT_MUTE_RUNNING		0x0000000400000000
#define LED_EVENT_SCAN_RUNNING		0x0000000800000000
#define LED_EVENT_TUNE_RUNNING		0x0000001000000000

 
	uint64_t  _ledEvent  = 0;
	void ledEventSet(uint64_t set, uint64_t reset);
 	void runLEDEventStartup();
	void runLEDEventVol();
	void runLEDEventMute();
	void runLEDEventScanner();
	void runLEDEventTuner();

	void LEDUpdateLoop();
 	static void* LEDUpdateThread(void *context);
	static void LEDUpdateThreadCleanup(void *context);
	pthread_cond_t 	_led_cond = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t 	_led_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_t			_ledUpdateTID;
 

	// MARK: -  Display Loop
	
	void DisplayUpdateLoop();		// C++ version of thread
	// C wrappers for DisplayUpdate;
	static void* DisplayUpdateThread(void *context);
	static void DisplayUpdateThreadCleanup(void *context);
 
	// MARK: - Metadata Loop
	void MetaDataReaderLoop();		// C++ version of thread
 	static void* MetaDataReaderThread(void *context);
	static void MetaDataReaderThreadCleanup(void *context);
	pthread_t	 _metaReaderTID;
 
	void processMetaDataString(string);
	void processAirplayMetaData(string type, string code, vector<uint8_t> payload);
	void airplayStarted();

	pthread_mutex_t 		_apmetadata_mutex = PTHREAD_MUTEX_INITIALIZER;
	map<string, string> 	_airplayMetaData = {};
	uint8_t	  				_airplayStatus;
	struct timespec		_lastAirplayStatusTime;
	bool						_shouldAutoPlay = false;

	
	// MARK: -
	
	typedef struct {
		event_t			evt :8;
		mode_state_t	mode:8;
		string			arg;
	}  eventQueueItem_t;

	queue<eventQueueItem_t> _eventQueue; // upper 8 bits is mode . lower 8 is event
	 
	pthread_cond_t 	_cond = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t 	_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_t			_updateTID;
	bool 					_isRunning = false;
 
 	// display
	bool _isSetup;
	DisplayType _displayType;
	VFD* _vfd;

    // Encoder configuration
    EncoderConfig _leftEncoderConfig;
    EncoderConfig _rightEncoderConfig;
    
    // Base class pointers for encoders
    EncoderBase* _leftEncoder;
    EncoderBase* _rightEncoder;

    // Duppa knobs (only for Duppa encoders)
    DuppaKnob* _leftKnob;
    DuppaKnob* _rightKnob;

    // Optional LED rings (only for Duppa encoders)
    DuppaLEDRing* _leftRing;
    DuppaLEDRing* _rightRing;

    // Thread management
    pthread_t _displayThread;
    pthread_t _ledThread;
    bool _running;

    // colors and brightness
    RGB _rightKnobColor;
    RGB _leftKnobColor;
    double _dimLevel;       // 0.0 = off ,  1.0  = full on.
    bool _backlightKnobs;
    bool _hasLEDs;  // Flag to track if LED functionality is enabled
};

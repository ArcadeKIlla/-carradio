//
//  FrameDB.hpp
//  canhacker
//
//  Created by Vincent Moscaritolo on 3/22/22.
//

#pragma once

#include "CommonDefs.hpp"

#include <vector>
#include <map>
#include <algorithm>
#include <mutex>
#include <bitset>
#include <strings.h>
#include <cstring>
#include <string_view>

#include "CanProtocol.hpp"

using namespace std;

typedef uint8_t  ifTag_t;		// we combine ifTag and frameiD to create a refnum
typedef uint64_t frameTag_t;		// < ifTag_t | canID>

struct  frame_entry{
	can_frame_t 	frame;
	unsigned long	timeStamp;	// from canbus (tv.tv_sec - start_tv.tv_sec) * 100 + (tv.tv_usec / 10000);
	long				avgTime;		 // how often do we see these  ((now - lastTime) + avgTime) / 2
	eTag_t 			eTag;
	time_t			updateTime;
	bitset<8> 		lastChange;
};

class FrameDB {

public:

	FrameDB();
	~FrameDB();
	
	bool registerProtocol(string_view ifName,  CanProtocol *protocol = NULL);
	void unRegisterProtocol(string_view ifName, CanProtocol *protocol);
	vector<CanProtocol*>	protocolsForTag(frameTag_t tag);
	vector<string_view> pollableInterfaces();
	 

	eTag_t lastEtag() { return  _lastEtag;};
 
// Frame database
	void saveFrame(string_view ifName, can_frame_t frame, unsigned long timeStamp);
	void clearFrames(string_view ifName = "");
	
	vector<frameTag_t> 	allFrames(string_view ifName);
	vector<frameTag_t>  	framesUpdateSinceEtag(string_view ifName, eTag_t eTag, eTag_t *newEtag);
	vector<frameTag_t>  	framesOlderthan(string_view ifName, time_t time);
	bool 						frameWithTag(frameTag_t tag, frame_entry *frame, string_view *ifNameOut = NULL);
	int						framesCount();

// value Database
	
	typedef enum {
		INVALID = 0,
		BOOL,				// Bool ON/OFF
		INT,				// Int
		BINARY,			// Binary 8 bits 000001
		STRING,			// string
		
		PERCENT, 		// (per hundred) sign ‰
		MILLIVOLTS,		// mV
		MILLIAMPS,		// mA
		SECONDS,			// sec
		MINUTES,			// mins
		DEGREES_C,		// degC
		KPA,				// kilopascal
		PA,				// pascal

		DEGREES,			// Degrees (heading)

		VOLTS,			// V
		AMPS,				// A

		RPM,				// Rev per minute
		KPH,
		LPH,				// Liters / Hour
		GPS,				// grams per second
		KM,				// Kilometers
		RATIO,			//
		FUEL_TRIM,		//
		NM,				// Newton Meters
		DATA,				// DATA block
		DTC,				// DTC codes 4 bytes each
		SPECIAL,
		IGNORE,
		UNKNOWN,
	}valueSchemaUnits_t;

	typedef struct {
		string_view             title;
		string_view             description;
		valueSchemaUnits_t  	units;
	} valueSchema_t;


	void addSchema(string_view key,  valueSchema_t schema, vector<uint8_t>obd_request = {});
	valueSchema_t schemaForKey(string_view key);
	
	bool obd_request(string_view key, vector <uint8_t> & request);
	
	void updateValue(string_view key, string_view value, time_t when);
	void clearValue(string_view key);

	void clearValues();
	int valuesCount();

	vector<string_view> 		allValueKeys();
	vector<string_view>  	valuesUpdateSinceEtag(eTag_t eTag, eTag_t *newEtag);
	vector<string_view>  	valuesOlderthan(time_t time);
	bool 							valueWithKey(string_view key, string *value);
	bool							boolForKey(string_view key, bool &state);
	bool							bitsForKey(string_view key, bitset<8> &bits);

	valueSchemaUnits_t 		unitsForKey(string_view key);
	string 						unitSuffixForKey(string_view key);
	double 						normalizedDoubleForValue(string_view key, string_view value);
	int 							intForValue(string_view key, string_view value);
	
 protected:
 
private:
 
	mutable std::mutex _mutex;
	eTag_t 		_lastEtag;
	eTag_t 		_lastValueEtag;

	ifTag_t _lastInterfaceTag;
 
	typedef struct {
		string_view							ifName;
		ifTag_t							ifTag;		// we combine ifTag and frameiD to create a refnum
		vector<CanProtocol*>   		protocols;
		map<canid_t,frame_entry> 	frames;
	} interfaceInfo_t;

// frames and interfaces
	map<string_view, interfaceInfo_t> _interfaces;
	
	// value database
	
	typedef struct {
		time_t			lastUpdate;
		eTag_t 			eTag;
		string			value;
		} value_t;

	map<string_view, valueSchema_t>			_schema;
	map<string_view, vector <uint8_t>>		_obd_request;
	map<string_view, value_t> _values;
  };

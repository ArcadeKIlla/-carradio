//
//  FrameDB.cpp
//  canhacker
//
//  Created by Vincent Moscaritolo on 3/22/22.
//

#include "FrameDB.hpp"
#include <regex>

static inline frameTag_t makeFrameTag(ifTag_t tag, canid_t canID){
	return  (( (uint64_t) tag) << 32) | canID;
};

static inline void  splitFrameTag(frameTag_t fTag, ifTag_t * ifTag, canid_t *canID){
	
	if(canID) *canID = fTag & 0xFFFFFFFF;
	if(ifTag) *ifTag = ((fTag >> 32) & 0xFF);
};


FrameDB::FrameDB(){
	_lastEtag = 0;
	_lastValueEtag = 0;
	_lastInterfaceTag = 0;
	_interfaces.clear();
	_schema.clear();
	_obd_request.clear();
	_values.clear();
}

FrameDB::~FrameDB(){
	
}
 

bool FrameDB::registerProtocol(string_view ifName, CanProtocol *protocol) {

	// create the interface if it doesnt already exist?
	if(_interfaces.count(ifName) == 0){
		interfaceInfo_t ifInfo;
		ifInfo.ifName = ifName;
		ifInfo.protocols.clear();
		ifInfo.frames.clear();
		ifInfo.ifTag = _lastInterfaceTag++;
		_interfaces[ifName] = ifInfo;
	}

	// get the map entry for that interface.
	auto m1 = _interfaces.find(ifName);
	auto protoList = &m1->second.protocols;
	 
	// is it an already registered ?
	for(auto it = protoList->begin();  it != protoList->end(); ++it) {
		if(*it == protocol)
			return false;
	}
 
	protoList->push_back(protocol);	
	return true;
}

void FrameDB::unRegisterProtocol(string_view ifName, CanProtocol *protocol){
	
	// erase all interfaces?
	if(_interfaces.count(ifName)){
		_interfaces.erase(ifName);
		_values.clear();
		return;
	}
	
	// get the map entry for that interface.
	auto m1 = _interfaces.find(ifName);
	if(m1 == _interfaces.end())
		return;
	
	auto protoList = &m1->second.protocols;
	// erase all protocols?
	if(!protocol){
		protoList->clear();
		return;
	}
	
	// erase it if registered  ?
	for(auto it = protoList->begin();  it != protoList->end(); ++it) {
		if(*it == protocol) {
			protoList->erase(it);
			return;
		}
	}
	
}
 

vector<string_view> FrameDB::pollableInterfaces(){

	vector<string_view> ifNames;
	
	for (const auto& [name ,_ ] : _interfaces){
		auto info = &_interfaces[name];
		for( auto p : info->protocols){
			if(p->canBePolled()){
				ifNames.push_back(name);
				break;
 			}
		}
 	}
 
	return ifNames;
}


vector<CanProtocol*>	FrameDB::protocolsForTag(frameTag_t tag){
	vector<CanProtocol*> protos;
	
 	ifTag_t	ifTag = 0;
	
	splitFrameTag(tag, &ifTag, NULL);

	for (const auto& [name ,_ ] : _interfaces){
		auto info = &_interfaces[name];
		if(info->ifTag	== ifTag){
			protos = info->protocols;
			break;
		}
	}

 	return protos;
}


FrameDB::valueSchema_t FrameDB::schemaForKey(string_view key){
	valueSchema_t schema = {"", "", UNKNOWN};
 
	if(_schema.count(key)){
		schema =  _schema[key];
	}
	
	return schema;
}

void FrameDB::addSchema(string_view key,  valueSchema_t schema, vector<uint8_t>obd_request){
	if( _schema.find(key) == _schema.end()){
		_schema[key] = schema;
		if(!obd_request.empty()){
			_obd_request[key] = obd_request;
		}
	}
}
 

bool FrameDB::obd_request(string_view key, vector <uint8_t> & request){
	bool success = false;
	if(_obd_request.count(key)){
		auto foundReq = _obd_request[key];
		uint8_t len = static_cast<uint8_t>(foundReq.size());
			vector <uint8_t> req;
		req.reserve(len+1);
		req.push_back(len);
		move(foundReq.begin(), foundReq.end(), std::back_inserter(req));
		
//
//		req.insert(req.end(), foundReq.begin(), foundReq.end());
		request = req;
		success = true;
	}
	
	return success;
}


// MARK: -  FRAMES
 
int FrameDB::framesCount(){

	int count = 0;
	
	for (auto& [key, entry]  : _interfaces){
		count += entry.frames.size();
	}

	return count;
}


void FrameDB::clearFrames(string_view ifName){
	
	for (auto& [key, entry]  : _interfaces)
		for(auto proto : entry.protocols){
			proto->reset();
		}
	
	// clear all interfaces?
	if(ifName.empty()){
		_interfaces.clear();
		clearValues();
	}
	else for (auto& [key, entry]  : _interfaces){
		if (strcasecmp(key.data(), ifName.data()) == 0){
			entry.frames.clear();
			return;
		}
	}
}

 
void  FrameDB::saveFrame(string_view ifName, can_frame_t frame, unsigned long  timeStamp){
	
	std::lock_guard<std::mutex> lock(_mutex);

	// find the interface
	auto m1 = _interfaces.find(ifName);
	if(m1 == _interfaces.end())
		return;
	
	auto ifInfo = &m1->second;
	auto frames = &ifInfo->frames;
	
	// calculate time since last frame
	time_t now = time(NULL);
	
	// is this a new frame?
	bool isNew = frames->count(frame.can_id) == 0;
	
	frame_entry entry;
	
	if(!isNew){
		entry = (*frames)[frame.can_id];
		
		// calculate average time
		long timeDiff = timeStamp - entry.timeStamp;
		entry.avgTime = ((timeDiff) + entry.avgTime) / 2;
		
		// see what changed
		entry.lastChange.reset();
		for(int i = 0; i < frame.can_dlc; i++){
			if(frame.data[i] != entry.frame.data[i])
				entry.lastChange.set(i);
		}
	}
	else {
		entry.avgTime = 0;
		entry.lastChange.reset();
	}
	
	entry.frame = frame;
	entry.timeStamp = timeStamp;
	entry.updateTime = now;
	entry.eTag = _lastEtag++;
	
	(*frames)[frame.can_id] = entry;
	
	// process frame with each protocol
	for(auto proto : ifInfo->protocols){
		proto->processFrame(this, string(ifName), frame, now );
	}
}

vector<frameTag_t> FrameDB::framesUpdateSinceEtag(string_view ifName, eTag_t eTag, eTag_t *eTagOut ){
	
	std::lock_guard<std::mutex> lock(_mutex);
	vector<frameTag_t> tags = {};
	
	for (const auto& [name ,_ ] : _interfaces)
		if(ifName.empty() || ifName == name ) {
			auto info = &_interfaces[name];
			
			auto theFrames = &info->frames;
			
			for (const auto& [canid, frame] : *theFrames) {
				if(frame.eTag <= eTag){
					tags.push_back(makeFrameTag(info->ifTag, canid));
				}
			}
		}
	
	if(eTagOut)
		*eTagOut = _lastEtag;
	
	return tags;
}


vector<frameTag_t> 	FrameDB::allFrames(string_view ifName){
	vector<frameTag_t> tags = {};
	
	std::lock_guard<std::mutex> lock(_mutex);
	for (const auto& [name ,_ ] : _interfaces)
		if(ifName.empty() || ifName == name ) {
			auto info = &_interfaces[name];
			
			auto theFrames = &info->frames;
			
			for (const auto& [canid, frame] : *theFrames) {
				tags.push_back(makeFrameTag(info->ifTag, canid));
	 		}
		}
	
	return tags;

}

vector<frameTag_t>  	FrameDB::framesOlderthan(string_view ifName, time_t time){
	vector<frameTag_t> tags = {};
	
	std::lock_guard<std::mutex> lock(_mutex);

	
	for (const auto& [name ,_ ] : _interfaces)
		if(ifName.empty() || ifName == name ) {
			auto info = &_interfaces[name];
			
			auto theFrames = &info->frames;
			
			for (const auto& [canid, frame] : *theFrames) {
				if(frame.updateTime < time)
					tags.push_back(makeFrameTag(info->ifTag, canid));
			}
		}
	
	return tags;
}

bool FrameDB::frameWithTag(frameTag_t tag, frame_entry *frameOut, string_view *ifNameOut){
	
	std::lock_guard<std::mutex> lock(_mutex);
	
	frame_entry entry;
	
	canid_t 	can_id = 0;
	ifTag_t	ifTag = 0;
	
	splitFrameTag(tag, &ifTag, &can_id);
	
	for (const auto& [name ,_ ] : _interfaces) {
		auto info = &_interfaces[name];
		if(info->ifTag == ifTag){
			auto theFrames = &info->frames;
			if(theFrames->count(can_id) == 0 )
				return false;

			if(frameOut){
				auto e = theFrames->find(can_id);
				*frameOut =  e->second;
				
				if(ifNameOut)
					*ifNameOut = info->ifName;
				return true;
			}
 		}
	}
	
	return false;
}

// MARK: -   VALUES

void  FrameDB::clearValues(){
	_values.clear();
	_lastEtag = 0;
}

int FrameDB::valuesCount() {
	return (int) _values.size();
}



void FrameDB::clearValue(string_view key){
	_values.erase(key);
}

void FrameDB::updateValue(string_view key, string_view value, time_t when){
 
	if(when == 0)
		when = time(NULL);
	
	bool shouldUpdate = true;
	
	if(_values.count(key)){
		auto oldVal = _values[key];
		
		if(oldVal.value == value)
			shouldUpdate = false;
	}
	
	if(shouldUpdate){
		value_t val = {when, _lastValueEtag++, string(value)};
		_values[key] = val;
	}
	
	// DEBUG
	if(shouldUpdate)
		printf("\t %20s : %s \n", string(key).c_str(), string(value).c_str());
}



vector<string_view> FrameDB::allValueKeys(){
	std::lock_guard<std::mutex> lock(_mutex);

	vector<string_view> keys;
	keys.clear();
	
	for (const auto& [key, value] : _values) {
			keys.push_back(key);
	}

	return keys;
}
  

vector<string_view> FrameDB::valuesUpdateSinceEtag(eTag_t eTag, eTag_t *eTagOut){
	
	std::lock_guard<std::mutex> lock(_mutex);
	vector<string_view> keys = {};
	
	for (const auto& [key, value] : _values) {
		if(value.eTag <= eTag)
			keys.push_back(key);
	}

	if(eTagOut)
		*eTagOut = _lastValueEtag;

	return keys;
};

vector<string_view> FrameDB::valuesOlderthan(time_t time){
	
	std::lock_guard<std::mutex> lock(_mutex);
	vector<string_view> keys = {};
	
	for (const auto& [key, value] : _values) {
		if(value.lastUpdate < time)
			keys.push_back(key);
	}

	return keys;
};


bool FrameDB::valueWithKey(string_view key, string *valueOut){
	std::lock_guard<std::mutex> lock(_mutex);
	
	if(_values.count(key) == 0 )
		return false;

	if(valueOut){
		*valueOut = _values[key].value;
	}
 	
	return true;
};


FrameDB::valueSchemaUnits_t FrameDB::unitsForKey(string_view key){
	valueSchema_t schema = schemaForKey(key);
	return schema.units;
}

string   FrameDB::unitSuffixForKey(string_view key){
	string suffix = {};
	
	switch(unitsForKey(key)){
			
			case VOLTS:
			suffix = "V";
			break;

		case MILLIAMPS:
		case AMPS:
			suffix = "A";
			break;
 
		case DEGREES_C:
			suffix = "ºC";
			break;
 
	 	case PERCENT:
			suffix = "%";
			break;
 
		case SECONDS:
			suffix = "Seconds";
			break;
			
		case MINUTES:
			suffix = "Minutes";
			break;

	 
		default:
			break;
	}
	
	return suffix;
}

double FrameDB::normalizedDoubleForValue(string_view key, string_view value){
	
	double retVal = 0;
	
	// see if it's a number
	char   *p;
	double val = strtod(value.data(), &p);
	if(*p == 0) {
 
		// normalize number
		
		switch(unitsForKey(key)){
				
			case MILLIVOLTS:
			case MILLIAMPS:
 				retVal = val / 1000;
				break;
				
			case  RPM:
	 			retVal = val /4;
				break;
				
			case PERCENT:
			case DEGREES_C:
 			case VOLTS:
			case AMPS:
			case SECONDS:
			case MINUTES:
			case KPA:
			case FUEL_TRIM:
			case KM:
			case KPH:
			default:
				retVal = val;
				
				break;
		}
	}
	return retVal;
}

 int FrameDB::intForValue(string_view key, string_view value){
	
	int retVal = 0;
	
	 switch(unitsForKey(key)){

		 case MINUTES:
		 case SECONDS:
		 case INT:
		 {
			 int intval = 0;

			 if(sscanf(value.data(), "%d", &intval) == 1){
				retVal = intval;
			}
		 }
			 break;
 
			 
			 default:
			 break;
	 }
	  
	return retVal;
}

bool	 FrameDB::boolForKey(string_view key, bool &state){
	
	bool valid = false;

	if(_values.count(key) &&  unitsForKey(key) == BOOL){
		string str = _values[key].value;
		
		const char * param1 = str.data();
		int intValue = atoi(param1);

		// check for level
		if(std::regex_match(param1, std::regex("^[0-1]$"))){
			state = bool(intValue);
			valid = true;
		}
		else {
			if(caseInSensStringCompare(str,"off")) {
				state = false;
				valid = true;
			}
			else if(caseInSensStringCompare(str,"on")) {
				state = true;
				valid = true;
			}
			else if(caseInSensStringCompare(str,"true")) {
				state = true;
				valid = true;
			}
			else if(caseInSensStringCompare(str,"false")) {
				state = false;
				valid = true;
			}
 		}
	}
	
	return valid;

};


bool	  FrameDB::bitsForKey(string_view key, bitset<8> &bitsout){
	bool valid = false;
	
	try {
		if(_values.count(key) &&  unitsForKey(key) == BINARY){
			string bit_string = _values[key].value;
			std::bitset<8> bits(bit_string);
			valid = true;
			bitsout = bits;
		}
	}
	catch (...) {
		valid = false;
	}
	
	return valid;

}

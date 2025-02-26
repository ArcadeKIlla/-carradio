//
//  main.cpp
//  vfdtest
//
//  Created by Vincent Moscaritolo on 4/13/22.
//

#include <stdio.h>
#include <stdlib.h>   // exit()

#include <stdexcept>
#include <sstream>
#include <ctime>
#include <cmath>
#include <iomanip>
#include <time.h>
#include <sys/time.h>
#include <iostream>
#include <filesystem> // C++17
#include <fstream>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include "CommonDefs.hpp"

#include "DisplayMgr.hpp"
#include "RadioMgr.hpp"
#include "TMP117.hpp"
#include "QwiicTwist.hpp"
#include "AudioOutput.hpp"
#include "PiCarDB.hpp"
#include "PiCarMgr.hpp"
#include "Utils.hpp"


// Check if port 5000 is in use
bool isPortInUse(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("WARNING: Failed to create socket for port check\n");
        return false;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int result = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    close(sock);

    return result < 0 && errno == EADDRINUSE;
}

// Kill existing Shairport process
void killExistingShairport() {
    system("pkill shairport-sync");
    // Give it a moment to fully terminate
    usleep(500000);  // 500ms
}

int main(int argc, const char * argv[]) {
	
    // Check if port 5000 is in use
    if (isPortInUse(5000)) {
        printf("WARNING: Port 5000 is in use. Attempting to kill existing Shairport process...\n");
        killExistingShairport();
    }

    int maxRetries = 3;
    int retryCount = 0;
    int childpid = -1;

    while (retryCount < maxRetries) {
        if (isPortInUse(5000)) {
            printf("WARNING: Port 5000 still in use after cleanup. Retrying... (%d/%d)\n", 
                   retryCount + 1, maxRetries);
            usleep(1000000);  // Wait 1 second before retry
            retryCount++;
            continue;
        }

        if((childpid = fork()) == -1) {
            perror("ERROR: Can't fork");
            exit(1);
        }
        else if(childpid == 0) {
            // Child process - launch shairport-sync
            char *binaryPath = (char*) "/usr/local/bin/shairport-sync";
            char *args[] = {binaryPath, (char*)"--output=pipe", (char*)"-M", NULL};
            
            execv(binaryPath, args);
            exit(0);
        }
        else {
            // Parent process - continue with normal initialization
            break;
        }
    }

    if (retryCount >= maxRetries) {
        printf("ERROR: Failed to start Shairport after %d attempts\n", maxRetries);
        // Continue anyway - the app should work without Airplay
    }
	
    PiCarMgr* pican = PiCarMgr::shared();
		
    // annoying log messages in librtlsdr
    freopen( "/dev/null", "w", stderr );
		
    if(!pican->begin()) {
        return 0;
    }
		
    // run the main loop.
    PRINT_CLASS_TID;
		
    bool firstrun = true;
    while(true) {
        if(firstrun){
            // Wait a moment for initialization
            sleep(2);
            
#if defined(__APPLE__)
            pican->audio()->setVolume(.5);
            pican->radio()->setFrequencyandMode(RadioMgr::BROADCAST_FM, 101.900e6);
            pican->radio()->setON(true);
#else
            // Initialize for non-Apple platforms (Raspberry Pi)
            printf("Initializing radio and display...\n");
            
            // Clear the display first to remove the init test message
            printf("Clearing display screen...\n");
            pican->display()->clearScreen();
            
            // Set initial volume and frequency
            pican->audio()->setVolume(.5);
            pican->radio()->setFrequencyandMode(RadioMgr::BROADCAST_FM, 101.900e6);
            pican->radio()->setON(true);
            
            // Process events to ensure display is updated
            printf("Forcing transition to time display...\n");
            
            // First, give the system time to finish startup mode
            sleep(3);
            
            // Force a direct mode transition to time display
            pican->display()->clearScreen();
            pican->display()->showTime();
            
            // Wait for the display to update
            sleep(1);
#endif
            firstrun = false;
            continue;
        }
        // Process events more frequently
        usleep(100000); // 100ms
    }
    return 0;
}

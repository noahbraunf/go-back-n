//
// Created by Phillip Romig on 7/16/24.
//
#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <array>

#include "timerC.h"
#include "unreliableTransport.h"
#include "logging.h"


#define WINDOW_SIZE 10
int main(int argc, char* argv[]) {

    // Defaults
    uint16_t portNum(12345);
    std::string hostname("");
    std::string inputFilename("");
    int requiredArgumentCount(0);


    int opt;
    try {
        while ((opt = getopt(argc, argv, "f:h:p:d:")) != -1) {
            switch (opt) {
                case 'p':
                    portNum = std::stoi(optarg);
                    break;
                case 'h':
                    hostname = optarg;
		    requiredArgumentCount++;
                    break;
                case 'd':
                    LOG_LEVEL = std::stoi(optarg);
                    break;
                case 'f':
                    inputFilename = optarg;
		    requiredArgumentCount++;
                    break;
                case '?':
                default:
                    std::cout << "Usage: " << argv[0] << " -f filename -h hostname [-p port] [-d debug_level]" << std::endl;
                    break;
            }
        }
    } catch (std::exception &e) {
        std::cout << "Usage: " << argv[0] << " -f filename -h hostname [-p port] [-d debug_level]" << std::endl;
        FATAL << "Invalid command line arguments: " << e.what() << ENDL;
        return(-1);
    }

    if (requiredArgumentCount != 2) {
        std::cout << "Usage: " << argv[0] << " -f filename -h hostname [-p port] [-d debug_level]" << std::endl;
	std::cerr << "hostname and filename are required." << std::endl;
	return(-1);
    }


    TRACE << "Command line arguments parsed." << ENDL;
    TRACE << "\tServername: " << hostname << ENDL;
    TRACE << "\tPort number: " << portNum << ENDL;
    TRACE << "\tDebug Level: " << LOG_LEVEL << ENDL;
    TRACE << "\tOutput file name: " << inputFilename << ENDL;

    // *********************************
    // * Open the input file
    // *********************************

    try {

        // ***************************************************************
        // * Initialize your timer, window and the unreliableTransport etc.
        // **************************************************************


        // ***************************************************************
        // * Send the file one datagram at a time until they have all been
        // * acknowledged
        // **************************************************************
        bool allSent(false);
        bool allAcked(false);
        while ((!allSent) && (!allAcked)) {
	
		// Is there space in the window? If so, read some data from the file and send it.

                // Call udt_recieve() to see if there is an acknowledgment.  If there is, process it.
 
                // Check to see if the timer has expired.

        }

        // cleanup and close the file and network.

    } catch (std::exception &e) {
        FATAL<< "Error: " << e.what() << ENDL;
        exit(1);
    }
    return 0;
}

//
// Created by Phillip Romig on 7/16/24.
//
#include <algorithm>
#include <array>
#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>

#include "datagram.h"
#include "logging.h"
#include "timerC.h"
#include "unreliableTransport.h"

#define WINDOW_SIZE 10
int main(int argc, char *argv[]) {

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
        std::cout << "Usage: " << argv[0]
                  << " -f filename -h hostname [-p port] [-d debug_level]"
                  << std::endl;
        break;
      }
    }
  } catch (std::exception &e) {
    std::cout << "Usage: " << argv[0]
              << " -f filename -h hostname [-p port] [-d debug_level]"
              << std::endl;
    FATAL << "Invalid command line arguments: " << e.what() << ENDL;
    return (-1);
  }

  if (requiredArgumentCount != 2) {
    std::cout << "Usage: " << argv[0]
              << " -f filename -h hostname [-p port] [-d debug_level]"
              << std::endl;
    std::cerr << "hostname and filename are required." << std::endl;
    return (-1);
  }

  TRACE << "Command line arguments parsed." << ENDL;
  TRACE << "\tServername: " << hostname << ENDL;
  TRACE << "\tPort number: " << portNum << ENDL;
  TRACE << "\tDebug Level: " << LOG_LEVEL << ENDL;
  TRACE << "\tOutput file name: " << inputFilename << ENDL;

  // *********************************
  // * Open the input file
  // *********************************
  std::ifstream file(inputFilename, std::ios::binary);
  if (!file.is_open()) {
    FATAL << "input file failed to open: " << inputFilename << ENDL;
    return -1;
  }

  try {

    // ***************************************************************
    // * Initialize your timer, window and the unreliableTransport etc.
    // **************************************************************
    timerC timer{100};
    unreliableTransportC client{hostname, portNum};
    std::array<datagramS, WINDOW_SIZE> window;
    uint16_t nextseqnum = 1;
    uint16_t base = 1;

    // ***************************************************************
    // * Send the file one datagram at a time until they have all been
    // * acknowledged
    // **************************************************************
    bool allSent{false};
    bool allAcked{false};
    while (!allSent || !allAcked) {
      // Is there space in the window? If so, read some data from the file and
      // send it.
      if (nextseqnum < base + WINDOW_SIZE) {

        std::array<char, MAX_PAYLOAD_LENGTH> buffer;
        file.read(buffer.data(), buffer.size());
        std::streamsize bytes_read = file.gcount();
        datagramS packet{
            nextseqnum, 0, 0, static_cast<uint8_t>(bytes_read), {}};
        if (bytes_read > 0) {
          std::copy(buffer.cbegin(), buffer.cbegin() + bytes_read, packet.data);

        } else {
          INFO << "Sending end packet" << ENDL;
          allSent = true;
        }

        packet.checksum = computeChecksum(packet);
        client.udt_send(packet);
        window[nextseqnum % WINDOW_SIZE] = packet;
        if (base == nextseqnum) {
          timer.start();
        }
        nextseqnum++;
      }

      // Call udt_recieve() to see if there is an acknowledgment.  If there is,
      // process it.
      datagramS ackpacket;
      const auto bytes_received = client.udt_receive(ackpacket);
      if (bytes_received > 0) {
        INFO << "received " << bytes_received << " bytes." << ENDL;
        if (validateChecksum(ackpacket)) {
          DEBUG << "Valid ACK for seqNum: " << ackpacket.ackNum << ENDL;

          if (ackpacket.ackNum >= base) {
            base = ackpacket.ackNum + 1;

            if (base == nextseqnum) {
              timer.stop();
              if (allSent) {
                allAcked = true;
              }
            } else {
              timer.start();
            }
          }
        } else {
          WARNING << "ACK received with wrong checksum" << ENDL;
        }
      } else {
        TRACE << "0 bytes received. Potentially could be a loss during "
                 "transmission"
              << ENDL;
      }

      // Check to see if the timer has expired.
      if (timer.timeout()) {
        WARNING << "Timeout occured, retrying tranmission from base: " << base
                << ENDL;

        for (uint16_t i = base; i < nextseqnum; i++) {
          client.udt_send(window[i % WINDOW_SIZE]);
          DEBUG << "Retranmitted packet (seq#: " << i << ")" << ENDL;
        }

        timer.start();
      }
    }

    INFO << "File transmission completed" << ENDL;

    // cleanup and close the file and network.
    file.close();
  } catch (std::exception &e) {
    FATAL << "Error: " << e.what() << ENDL;
    file.close();
    exit(1);
  }
  return 0;
}

#ifndef SERIAL_PORT_PROTOCOLS_H
#define SERIAL_PORT_PROTOCOLS_H

// All needed headers here
#include <windows.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

typedef struct {
	HANDLE	serialHandle;
	char	portName[64];
	bool	isOpen;
	DWORD	readTimeoutMilliseconds;
	DWORD	writeTimeoutMilliseconds;
} SerialPort;

void sleepMilliseconds(unsigned int milliseconds);

bool openSerialPort(SerialPort* selectedPort, const char* portName, DWORD baudRate);

void closeSerialPort(SerialPort* selectedPort);

bool writeBytes(SerialPort* selectedPort, const void* dataToWrite, size_t lengthToWrite);

int readBytes(SerialPort* selectedPort, void* dataToRead, size_t lengthToRead);

void flushInput(SerialPort* selectedPort);

#endif
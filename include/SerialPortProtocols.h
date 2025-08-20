#ifndef SERIAL_PORT_PROTOCOLS_H
#define SERIAL_PORT_PROTOCOLS_H

#include "framework.h"

typedef struct {
	HANDLE	serialHandle;
	char	portName[64];
	bool	isOpen;
	DWORD	readTimeoutMilliseconds;
	DWORD	writeTimeoutMilliseconds;
} SerialPort;

bool openSerialPort(SerialPort* selectedPort, const char* portName, DWORD baudRate);

void closeSerialPort(SerialPort* selectedPort);

bool writeBytes(SerialPort* selectedPort, const void* dataToWrite, size_t lengthToWrite);

int readBytes(SerialPort* selectedPort, void* dataToRead, size_t lengthToRead);

void flushInput(SerialPort* selectedPort);

#endif
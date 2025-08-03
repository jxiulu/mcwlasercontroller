#ifndef SERIAL_PORT_PROTOCOLS_H
#define SERIAL_PORT_PROTOCOLS_H

#include <windows.h>

typedef struct {
	HANDLE	serialHandle;
	char	portName[64];
	bool	isOpen;
	DWORD	readTimeoutMilliseconds;
	DWORD	writeTimeoutMilliseconds;
} SerialPort;

void sleepMilliseconds(unsigned int milliseconds);

void sleepMilliseconds(unsigned int milliseconds);

bool setDeviceControlBook(DCB* deviceControlBook, SerialPort* selectedPort, DWORD baudRate);

void setDeviceTimeouts(COMMTIMEOUTS* timeOut, SerialPort* selectedPort, DWORD baudRate);

bool openSerialPort(SerialPort* selectedPort, const char* portName, DWORD baudRate);

void closeSerialPort(SerialPort* selectedPort);

bool writeBytes(SerialPort* selectedPort, const void* dataToWrite, size_t lengthToWrite);

int readBytes(SerialPort* selectedPort, void* dataToRead, size_t lengthToRead);

void flushInput(SerialPort* selectedPort);

#endif
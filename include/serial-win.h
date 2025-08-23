#ifndef SERIAL_WIN_H
#define SERIAL_WIN_H

#include "framework.h"

typedef struct {
	HANDLE	SerialHandle;
	char	PortName[64];
	bool	IsOpen;
	DWORD	ReadTimeoutMs;
	DWORD	WriteTimeoutMs;
} SerialPort;

void SleepMs(unsigned int Milliseconds);

bool OpenSerialPort(SerialPort* TargetPort, const char* PortName, DWORD baudRate);

void CloseSerialPort(SerialPort* TargetPort);

bool writeBytes(SerialPort* TargetPort, const void* dataToWrite, size_t lengthToWrite);

int readBytes(SerialPort* TargetPort, void* dataToRead, size_t lengthToRead);

void flushInput(SerialPort* TargetPort);

#endif
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include "SerialPortProtocols.h"
#include "OstechProtocols.h"

void sleepMilliseconds(unsigned int milliseconds) {
	Sleep(milliseconds);
}

bool setDeviceControlBook(DCB* deviceControlBook, SerialPort* selectedPort, DWORD baudRate) {
	deviceControlBook->DCBlength = sizeof(deviceControlBook);
	if (!GetCommState(selectedPort->serialHandle, &deviceControlBook)) {
		fprintf(stderr, "GETCOMMSTATE FAILED (ERROR %lu)\n", GetLastError());
		CloseHandle(selectedPort->serialHandle);
		return false;
	}

	deviceControlBook->BaudRate = baudRate;
	deviceControlBook->ByteSize = 9;
	deviceControlBook->Parity = NOPARITY;
	deviceControlBook->StopBits = ONESTOPBIT;
	deviceControlBook->fBinary = TRUE;
	deviceControlBook->fParity = FALSE;
}

void setDeviceTimeouts(COMMTIMEOUTS* timeOut, SerialPort* selectedPort, DWORD baudRate) {
	timeOut->ReadIntervalTimeout = selectedPort->readTimeoutMilliseconds;
	timeOut->ReadTotalTimeoutMultiplier = 0;
	timeOut->ReadTotalTimeoutConstant = selectedPort->readTimeoutMilliseconds;
	timeOut->WriteTotalTimeoutMultiplier = 0;
	timeOut->WriteTotalTimeoutConstant = selectedPort->writeTimeoutMilliseconds;
}

bool openSerialPort(SerialPort* selectedPort, const char* portName, DWORD baudRate) {
	if (!selectedPort)
		return false;
	memset(selectedPort, 0, sizeof(*selectedPort));
	selectedPort->readTimeoutMilliseconds = 1000;
	selectedPort->writeTimeoutMilliseconds = 1000;

	char fullName[128];
	snprintf(fullName, sizeof(fullName), "\\\\.\\%s", portName); // Prefix that is needed for COM10 and over on Windwos (allegedly)
	strncpy(selectedPort->portName, fullName, sizeof(selectedPort->portName) - 1);

	selectedPort->serialHandle = CreateFileA(
		selectedPort->portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL
	);
	if (selectedPort->serialHandle == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "FAILED TO OPEN %s (ERROR %lu)\n", selectedPort->portName, GetLastError());
		return false;
	}

	DCB deviceControlBook = { 0 };
	return setDeviceControlBook(&deviceControlBook, selectedPort, baudRate);

	COMMTIMEOUTS timeOut = { 0 };
	setDeviceTimeouts(&timeOut, selectedPort, baudRate);


	if (!SetCommTimeouts(selectedPort->serialHandle, &timeOut)) {
		fprintf(stderr, "SETCOMMTIMEOUTS FAILED (ERROR %lu)\n", GetLastError());
		CloseHandle(selectedPort->serialHandle);
		return false;
	}

	PurgeComm(selectedPort->serialHandle, PURGE_RXCLEAR | PURGE_TXCLEAR);

	selectedPort->isOpen = true;
	return true;
}

void closeSerialPort(SerialPort* selectedPort) {
	if (selectedPort && selectedPort->isOpen) {
		CloseHandle(selectedPort->serialHandle);
		selectedPort->isOpen = false;
	}
}

bool writeBytes(SerialPort* selectedPort, const void* dataToWrite, size_t lengthToWrite) {
	if (!selectedPort || !selectedPort->isOpen)
		return false;
	DWORD bytesWritten = 0;
	if (!WriteFile(selectedPort->serialHandle, dataToWrite, (DWORD)lengthToWrite, &bytesWritten, NULL)) {
		return false;
	}

	return bytesWritten == (DWORD)lengthToWrite; // also returns if its writtable
}

int readBytes(SerialPort* selectedPort, void* dataToRead, size_t lengthToRead) {
	if (!selectedPort || !selectedPort->isOpen)
		return -1;
	DWORD bytesRead = 0;
	if (!ReadFile(selectedPort->serialHandle, dataToRead, (DWORD)lengthToRead, &bytesRead, NULL)) {
		return -1;
	}

	return (int)bytesRead; // also returns the number of bytes read
}

void flushInput(SerialPort* selectedPort) {
	if (selectedPort && selectedPort->isOpen)
		PurgeComm(selectedPort->serialHandle, PURGE_RXCLEAR);
} // flush means clear any data in the input
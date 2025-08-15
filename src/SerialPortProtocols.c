#define _CRT_SECURE_NO_WARNINGS

#include "SerialPortProtocols.h"

void sleepMilliseconds(unsigned int milliseconds) {
	Sleep(milliseconds);
}

static bool setDeviceControlBook(DCB* deviceControlBook, SerialPort* selectedPort, DWORD baudRate) {
	ZeroMemory(deviceControlBook, sizeof(*deviceControlBook));
	deviceControlBook->DCBlength = sizeof(*deviceControlBook);

	if (!GetCommState(selectedPort->serialHandle, deviceControlBook)) {
		fprintf(stderr, "GETCOMMSTATE FAILED (ERROR %lu)\n", GetLastError());
		CloseHandle(selectedPort->serialHandle);
		return false;
	}

	deviceControlBook->BaudRate = baudRate;
	deviceControlBook->ByteSize = 8; // 8N1
	deviceControlBook->Parity = NOPARITY;
	deviceControlBook->StopBits = ONESTOPBIT;
	deviceControlBook->fBinary = TRUE;
	deviceControlBook->fParity = FALSE;

	if (!SetCommState(selectedPort->serialHandle, deviceControlBook)) {
		fprintf(stderr, "SETCOMMSTATE FAILED (ERROR %lu)\n", GetLastError());
		CloseHandle(selectedPort->serialHandle);
		return false;
	}
	return true;
}

static void setDeviceTimeouts(COMMTIMEOUTS* timeOut, SerialPort* selectedPort, DWORD baudRate) {
	ZeroMemory(timeOut, sizeof(*timeOut));
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
	snprintf(fullName, sizeof(fullName), "\\\\.\\%s", portName); // Prefix that is needed for COM10 and over on Windwos (allegedly). should be fine for COM1 through COM9
	strncpy(selectedPort->portName, fullName, sizeof(selectedPort->portName) - 1);

	selectedPort->serialHandle = CreateFileA(
		selectedPort->portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL
	);
	if (selectedPort->serialHandle == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "FAILED TO OPEN %s (ERROR %lu)\n", selectedPort->portName, GetLastError());
		return false;
	}

	DCB deviceControlBook = { 0 };
	if (!setDeviceControlBook(&deviceControlBook, selectedPort, baudRate)) {
		CloseHandle(selectedPort->serialHandle);
		return false;
	}

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
}
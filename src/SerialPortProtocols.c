#define _CRT_SECURE_NO_WARNINGS

#include "SerialPortProtocols.h"

void SleepMs(unsigned int milliseconds) {
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

bool OpenSerialPort(SerialPort* TargetPort, const char* PortName, DWORD BaudRate) {
	if (!TargetPort)
		return false;
	memset(TargetPort, 0, sizeof(*TargetPort));
	TargetPort->readTimeoutMilliseconds = 1000;
	TargetPort->writeTimeoutMilliseconds = 1000;

	char fullName[128];
	snprintf(fullName, sizeof(fullName), "\\\\.\\%s", PortName); // Prefix that is needed for COM10 and over on Windwos (allegedly). should be fine for COM1 through COM9
	strncpy(TargetPort->portName, fullName, sizeof(TargetPort->portName) - 1);

	TargetPort->serialHandle = CreateFileA(
		TargetPort->portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL
	);
	if (TargetPort->serialHandle == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "FAILED TO OPEN %s (ERROR %lu)\n", TargetPort->portName, GetLastError());
		return false;
	}

	DCB deviceControlBook = { 0 };
	if (!setDeviceControlBook(&deviceControlBook, TargetPort, BaudRate))
		return false;

	COMMTIMEOUTS timeOut = { 0 };
	setDeviceTimeouts(&timeOut, TargetPort, BaudRate);
	if (!SetCommTimeouts(TargetPort->serialHandle, &timeOut)) {
		fprintf(stderr, "SETCOMMTIMEOUTS FAILED (ERROR %lu)\n", GetLastError());
		CloseHandle(TargetPort->serialHandle);
		return false;
	}

	PurgeComm(TargetPort->serialHandle, PURGE_RXCLEAR | PURGE_TXCLEAR);
	TargetPort->isOpen = true;
	return true;
}

void CloseSerialPort(SerialPort* TargetPort) {
	if (TargetPort && TargetPort->isOpen) {
		CloseHandle(TargetPort->serialHandle);
		TargetPort->isOpen = false;
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
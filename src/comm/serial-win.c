#define _CRT_SECURE_NO_WARNINGS

#include "serial-win.h"

void SleepMs(unsigned int milliseconds) {
	Sleep(milliseconds);
}

static inline bool setDeviceControlBook(DCB* deviceControlBook, SerialPort* selectedPort, DWORD baudRate) {
	ZeroMemory(deviceControlBook, sizeof(*deviceControlBook));
	deviceControlBook->DCBlength = sizeof(*deviceControlBook);

	if (!GetCommState(selectedPort->SerialHandle, deviceControlBook)) {
		fprintf(stderr, "GETCOMMSTATE FAILED (ERROR %lu)\n", GetLastError());
		CloseHandle(selectedPort->SerialHandle);
		return false;
	}

	deviceControlBook->BaudRate = baudRate;
	deviceControlBook->ByteSize = 8; // 8N1
	deviceControlBook->Parity = NOPARITY;
	deviceControlBook->StopBits = ONESTOPBIT;
	deviceControlBook->fBinary = TRUE;
	deviceControlBook->fParity = FALSE;

	if (!SetCommState(selectedPort->SerialHandle, deviceControlBook)) {
		fprintf(stderr, "SETCOMMSTATE FAILED (ERROR %lu)\n", GetLastError());
		CloseHandle(selectedPort->SerialHandle);
		return false;
	}
	return true;
}

static inline void setDeviceTimeouts(COMMTIMEOUTS* timeOut, SerialPort* selectedPort, DWORD baudRate) {
	ZeroMemory(timeOut, sizeof(*timeOut));
	timeOut->ReadIntervalTimeout = selectedPort->ReadTimeoutMs;
	timeOut->ReadTotalTimeoutMultiplier = 0;
	timeOut->ReadTotalTimeoutConstant = selectedPort->ReadTimeoutMs;
	timeOut->WriteTotalTimeoutMultiplier = 0;
	timeOut->WriteTotalTimeoutConstant = selectedPort->WriteTimeoutMs;
}

bool OpenSerialPort(SerialPort* TargetPort, const char* PortName, DWORD BaudRate) {
	if (!TargetPort)
		return false;
	memset(TargetPort, 0, sizeof(*TargetPort));
	TargetPort->ReadTimeoutMs = 1000;
	TargetPort->WriteTimeoutMs = 1000;

	char fullName[128];
	snprintf(fullName, sizeof(fullName), "\\\\.\\%s", PortName); // Prefix that is needed for COM10 and over on Windwos (allegedly). should be fine for COM1 through COM9
	strncpy(TargetPort->PortName, fullName, sizeof(TargetPort->PortName) - 1);

	TargetPort->SerialHandle = CreateFileA(
		TargetPort->PortName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL
	);
	if (TargetPort->SerialHandle == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "FAILED TO OPEN %s (ERROR %lu)\n", TargetPort->PortName, GetLastError());
		return false;
	}

	DCB deviceControlBook = { 0 };
	if (!setDeviceControlBook(&deviceControlBook, TargetPort, BaudRate))
		return false;

	COMMTIMEOUTS timeOut = { 0 };
	setDeviceTimeouts(&timeOut, TargetPort, BaudRate);
	if (!SetCommTimeouts(TargetPort->SerialHandle, &timeOut)) {
		fprintf(stderr, "SETCOMMTIMEOUTS FAILED (ERROR %lu)\n", GetLastError());
		CloseHandle(TargetPort->SerialHandle);
		return false;
	}

	PurgeComm(TargetPort->SerialHandle, PURGE_RXCLEAR | PURGE_TXCLEAR);
	TargetPort->IsOpen = true;
	return true;
}

void CloseSerialPort(SerialPort* TargetPort) {
	if (TargetPort && TargetPort->IsOpen) {
		CloseHandle(TargetPort->SerialHandle);
		TargetPort->IsOpen = false;
	}
}

bool writeBytes(SerialPort* selectedPort, const void* dataToWrite, size_t lengthToWrite) {
	if (!selectedPort || !selectedPort->IsOpen)
		return false;
	DWORD bytesWritten = 0;
	if (!WriteFile(selectedPort->SerialHandle, dataToWrite, (DWORD)lengthToWrite, &bytesWritten, NULL)) {
		return false;
	}

	return bytesWritten == (DWORD)lengthToWrite; // also returns if its writtable
}

int readBytes(SerialPort* selectedPort, void* dataToRead, size_t lengthToRead) {
	if (!selectedPort || !selectedPort->IsOpen)
		return -1;
	DWORD bytesRead = 0;
	if (!ReadFile(selectedPort->SerialHandle, dataToRead, (DWORD)lengthToRead, &bytesRead, NULL)) {
		return -1;
	}

	return (int)bytesRead; // also returns the number of bytes read
}

void flushInput(SerialPort* selectedPort) {
	if (selectedPort && selectedPort->IsOpen)
		PurgeComm(selectedPort->SerialHandle, PURGE_RXCLEAR);
}
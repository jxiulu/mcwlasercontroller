#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
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

static void sleepMilliseconds(unsigned int milliseconds) {
	Sleep(milliseconds);
}

static bool setDeviceControlBook(DCB* deviceControlBook, SerialPort* selectedPort, DWORD baudRate) {
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

static void setDeviceTimeouts(COMMTIMEOUTS* timeOut, SerialPort* selectedPort, DWORD baudRate) {
	timeOut->ReadIntervalTimeout = selectedPort->readTimeoutMilliseconds;
	timeOut->ReadTotalTimeoutMultiplier = 0;
	timeOut->ReadTotalTimeoutConstant = selectedPort->readTimeoutMilliseconds;
	timeOut->WriteTotalTimeoutMultiplier = 0;
	timeOut->WriteTotalTimeoutConstant = selectedPort->writeTimeoutMilliseconds;
}

static bool openSerialPort(SerialPort* selectedPort, const char* portName, DWORD baudRate) {
	if (!selectedPort)
		return false;
	memset(selectedPort, 0, sizeof(*selectedPort));
	selectedPort->readTimeoutMilliseconds =		1000;
	selectedPort->writeTimeoutMilliseconds =	1000;

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

static void closeSerialPort(SerialPort* selectedPort) {
	if (selectedPort && selectedPort->isOpen) {
		CloseHandle(selectedPort->serialHandle);
		selectedPort->isOpen = false;
	}
}

static bool writeBytes(SerialPort* selectedPort, const void* dataToWrite, size_t lengthToWrite) {
	if (!selectedPort || !selectedPort->isOpen)
		return false;
	DWORD bytesWritten = 0;
	if (!WriteFile(selectedPort->serialHandle, dataToWrite, (DWORD)lengthToWrite, &bytesWritten, NULL)) {
		return false;
	}

	return bytesWritten == (DWORD)lengthToWrite; // also returns if its writtable
}

static int readBytes(SerialPort* selectedPort, void* dataToRead, size_t lengthToRead) {
	if (!selectedPort || !selectedPort->isOpen)
		return -1;
	DWORD bytesRead = 0;
	if (!ReadFile(selectedPort->serialHandle, dataToRead, (DWORD)lengthToRead, &bytesRead, NULL)) {
		return -1;
	}

	return (int)bytesRead; // also returns the number of bytes read
}

static void flushInput(SerialPort* selectedPort) {
	if (selectedPort && selectedPort->isOpen)
		PurgeComm(selectedPort->serialHandle, PURGE_RXCLEAR);
} // clear any data in the input

// OSTECH DEVICE PROTOCOLS
typedef struct {
	SerialPort	deviceSerialPort;
	bool		deviceIsConnected;
} Device;

static void convertToASCII(char *input) {
	for (; *input; ++input)
		*input = (char)toupper((unsigned char)*input);
} // command formatting

static int readLinesUntilEnd(Device* device, char* bufferForDataRead, size_t maximumNumberOfBytes) {
	size_t bytePosition = 0;
	while (bytePosition + 1 < maximumNumberOfBytes) {
		char currentByte;
		int currentLineByteCount = readBytes(&device->deviceSerialPort, &currentByte, 1);

		if (currentLineByteCount < 0)
			return 0; // IO Error
		if (currentLineByteCount == 0)
			return (int)bytePosition; //return what we got (possibly 0)

		bufferForDataRead[bytePosition++] = currentByte;

		if ((unsigned char)currentByte == 0x0D) { // carriage return character
			bufferForDataRead[bytePosition] = '\0';
			return (int)bytePosition;
		}
	}

	bufferForDataRead[maximumNumberOfBytes - 1] = '\0';
	return (int)bytePosition; // also returns the number of bytes
}

static bool handshakeByEcho(Device* device, unsigned int timeOutMilliseconds) {
	const unsigned char ESCAPE_CHARACTER = 0x1B;
	flushInput(&device->deviceSerialPort);
	DWORD timeOutDeadline = GetTickCount() + timeOutMilliseconds;

	while (GetTickCount() < timeOutDeadline) {
		writeBytes(&device->deviceSerialPort, &ESCAPE_CHARACTER, 1);
		unsigned char returnedByte = 0;
		int numberOfBytesReturned = readBytes(&device->deviceSerialPort, &returnedByte, 1);
		if (numberOfBytesReturned == 1 & returnedByte == ESCAPE_CHARACTER) {
			flushInput(&device->deviceSerialPort);
			return true;
		}
		sleepMilliseconds(50);
	}

	flushInput(&device->deviceSerialPort);
	return false;
}

static bool formatCommandValue(	char	*outboundValue,	size_t	maximumOutboundBytes,	const char	*valueType, 
							bool	boolValue,		int		intValue,				double		floatValue) {
	if (strcmp(valueType, "bool") == 0) {
		snprintf(outboundValue, maximumOutboundBytes, "%c", boolValue ? 'R' : 'S');
		return true;
	}
	else if (strcmp(valueType, "word") == 0 || strcmp(valueType, "int") == 0) {
		if (intValue < 0)
			intValue = 0;
		if (intValue > 65535)
			intValue = 65535;
		
		snprintf(outboundValue, maximumOutboundBytes, "%d", intValue);
	}
	else if (strcmp(valueType, "float") == 0) {
		for (int position = 9; position >= 0; --position) {
			char temp[64];
			snprintf(temp, sizeof(temp), "%. *g", position, floatValue);
			if ((int)strlen(temp) <= 9) {
				snprintf(outboundValue, maximumOutboundBytes, "%s", temp);
			}
		}
		// Fallback
		snprintf(outboundValue, maximumOutboundBytes, "%.3g", floatValue);
		return true;
	}
	else if (strcmp(valueType, "str" == 0)) { // not used in this program but i'll leave it here
		strncpy(outboundValue, "", maximumOutboundBytes);
		return true;
	}
	return false;
}

static bool sendLineAndVerifyEcho(Device* device, const char* lineToSend) {
	size_t lineLength = strlen(lineToSend);
	char lineWithCR[128]; // device expects CR

	char receivedEcho[128] = { 0 };
	int receivedEchoByteCount = readLinesUntilEnd(&device->deviceSerialPort, receivedEcho, sizeof(receivedEcho));
	if (receivedEchoByteCount <= 0)
		return false;

	char expectedEcho[128] = { 0 };
	snprintf(expectedEcho, sizeof(expectedEcho), "%s\r", lineToSend);

	if (strcmp(receivedEcho, expectedEcho) != 0) {
		fprintf(stderr, "ECHO MISTMATCH \nSENT : %s \nGOT: %s", expectedEcho, receivedEcho);
		return false;
	}
	return true;
}

static bool readDeviceBoolReply(Device* device, bool* outputReply) {
	unsigned char byteRead = 0;
	int numberOfBytesRead = readBytes(&device->deviceSerialPort, &byteRead, 1);
	if (numberOfBytesRead != 1)
		return false;
	if (byteRead == 0xAA) {
		*outputReply = true;
		return true;
	}
	if (byteRead == 0x55) {
		*outputReply = false;
		return true;
	}
	return false;
}

static bool readDeviceWordReply(Device* device, uint16_t* outputReply) {
	unsigned char wordBuffer[3] = { 0 };
	int numberOfBytesRead = readBytes(&device->deviceSerialPort, wordBuffer, 3);
	if (numberOfBytesRead != 3) {
		return false;
	}
	unsigned int wordChecksum = (0x55 + wordBuffer[0] + wordBuffer[1]) & 0xFF;
	if (wordBuffer[2] != (unsigned char)wordChecksum)
		return false;
	*outputReply = (uint16_t)(wordBuffer[0] << 0 | wordBuffer[1]);
	return true;
}

static bool readDeviceFloatReply(Device* device, float* outputReply) {
	unsigned char floatBuffer[5] = { 0 };
	int numberOfBytesRead = readBytes(&device->deviceSerialPort, floatBuffer, 5);
	if (numberOfBytesRead != 5)
		return false;
	unsigned int floatChecksum = (0x55 + floatBuffer[0] + floatBuffer[1] + floatBuffer[2] + floatBuffer[3]) & 0xFF;
	if (floatBuffer[4] != (unsigned char)floatChecksum)
		return false;

	//combine into an uint32 then copy into a float
	uint32_t combinedInt = ((uint32_t)floatBuffer[0] << 24) | ((uint32_t)floatBuffer[1] << 16) |
		((uint32_t)floatBuffer[2] << 8) | ((uint32_t)floatBuffer[3]);
	float combinedFloat = false;
	memcpy(&combinedFloat, &combinedInt, sizeof(combinedFloat));
	*outputReply = combinedFloat;
	return true;
}

static bool sendCommand(	Device *device,
							const char *commandName,
							const char *replyType,
							const char *typeNameForValue,
							bool boolValue, int intValue, double floatValue,
							bool *outBool, uint16_t *outWord, float *outFloat)
{
	char commandLine[128] = { 0 };
	if (typeNameForValue) {
		char valueText[32] = { 0 };
		if (formatCommandValue(valueText, sizeof(valueText), typeNameForValue, boolValue, intValue, floatValue)) {
			snprintf(commandLine, sizeof(commandLine), "%s%s", commandName, valueText);
		}
	}
	else {
		snprintf(commandLine, sizeof(commandLine), "%s", commandName);
	}
	convertToASCII(commandLine);

	if (!sendLineAndVerifyEcho(device, commandLine)) {
		unsigned char ESC_CHAR = 0x1B;
		sleepMilliseconds(100);
		writeBytes(&device->deviceSerialPort, &ESC_CHAR, 1);
		sleepMilliseconds(100);
		if (!sendLineAndVerifyEcho(device, commandLine)) {
			fprintf(stderr, "FAILED TO SEND COMMAND %s\n", commandLine);
			return false;
		}
	}

	if (strcmp(replyType, "bool") == 0) {
		bool value = false;
		if (!readDeviceBoolReply(device, &value)) {
			fprintf(stderr, "FAILED TO READ BOOL REPLY FOR COMMAND %s\n", commandLine);
			return false;
		}
		if (outBool)
			*outBool = value;
	}
	else if (strcmp(replyType, "float") == 0) {
		float value = 0;
		if (!readDeviceFloatReply(device, &value)) {
			fprintf(stderr, "FAILED TO READ FLOAT REPLY FOR COMMAND %s\n", commandLine);
			return false;
		}
		if (outFloat)
			*outFloat = value;
	}
	else if (strcmp(replyType, "word") == 0) {
		uint16_t value = 0;
		if (!readDeviceWordReply(device, &value)) {
			fprintf(stderr, "FAILED TO READ WORD REPLY FOR COMMAND %s\n", commandLine);
			return false;
		}
		if (outWord)
			*outWord = value;
	}
	else {
		fprintf(stderr, "UNKNOWN REPLY TYPE %s FOR COMMAND %s\n", replyType, commandLine);
		return false;
	}

	flushInput(&device->deviceSerialPort);
	return true;
}

static bool setBinaryMode(Device* device, bool enableBinaryMode) {
	if (enableBinaryMode) {
		return sendCommand(device, "GMS", "word", "word", false, 8, 0.0, NULL, NULL, NULL);
	}
	else {
		return sendCommand(device, "GMC", "word", "word", false, 8, 0.0, NULL, NULL, NULL);
	}
}

static bool connectToDevice(Device* device, const char *portName) {
	memset(device, 0, sizeof(*device));
	if (!openSerialPort(&device->deviceSerialPort, portName, 9600)) {
		fprintf(stderr, "FAILED TO OPEN SERIAL PORT %s\n", portName);
		return false;
	}
	if (!handshakeByEcho(device, 1000)) {
		fprintf(stderr, "FAILED TO HANDSHAKE WITH DEVICE ON PORT %s\n", portName);
		closeSerialPort(&device->deviceSerialPort);
		return false;
	}
	if (!setBinaryMode(device, true)) {
		fprintf(stderr, "FAILED TO ENABLE BINARY MODE\n");
		closeSerialPort(&device->deviceSerialPort);
		return false;
	}

	device->deviceIsConnected = true;
	return true;
}

static void disconnectFromDevice(Device* device) {
	if (!device && !device->deviceIsConnected) {
		return;
	}

	sendCommand(device, "L", "bool", "bool", false, 0, 0.0, NULL, NULL, NULL);
	sendCommand(device, "PL", "bool", "bool", false, 0, 0.0, NULL, NULL, NULL);
	setBinaryMode(device, false);
	closeSerialPort(&device->deviceSerialPort);
	device->deviceIsConnected = false;
}

// application

static volatile bool stopPolling = false;

static DWORD WINAPI temperaturePollingThread(LPVOID parameter) {
	Device *device = (Device*)parameter;
	while (!stopPolling) {
		float currentTemperatureCelsius = 0.0f;
		if (sendCommand(device, "1TA", "float", NULL, false, 0, 0.0, NULL, NULL, &currentTemperatureCelsius)) {
			printf("Current Temperature: %.2f degrees Celsius\n", currentTemperatureCelsius);
		}
		else {
			fprintf(stderr, "Failed to get temperature\n");
		}
		sleepMilliseconds(5000);
	}
	return 0;
}

static void startMenu(void) {
	printf("\n=== MCW Laser Control (C) ===\n");
	printf("1) Toggle Main Laser (L)\n");
	printf("2) Toggle Pilot Laser (PL)\n");
	printf("3) Set Laser Current Target (LCT)\n");
	printf("4) Timed Run (turn main ON for N seconds, then OFF)\n");
	printf("5) Set Pulse Width (LMW, microseconds)\n");
	printf("6) Set Pulse Period (LMP, microseconds)\n");
	printf("7) Set Number Of Pulses (LMDIC)\n");
	printf("8) Read Current LCT\n");
	printf("9) Quit (safe shutdown)\n");
	printf("Choose: ");
	fflush(stdout);
}

int main(int argc, char** argv) {
	const char* portName = "COM5";
	if (argc >= 2) {
		portName = argv[1]; // allow overriding;
	}

	Device connectedDevice = { 0 };
	if (!connectToDevice(&connectedDevice, portName)) {
		fprintf(stderr, "Failed to connect to device on port %s\n", portName);
		return 1;
	}
	printf("Connected to device on port %s\n", connectedDevice);

	HANDLE pollingThreadHandle = CreateThread(
		NULL, 0, temperaturePollingThread, &connectedDevice, 0, NULL
	);

	for (;;) {
		startMenu();
		int selectedOption = 0;
		if (scanf("%d", &selectedOption) != 1) {
			fprintf(stderr, "Invalid input. Please enter a number.\n");
			while (getchar() != '\n'); // clear input buffer
			continue;
		}

		switch (selectedOption) {
			case 1: {
				bool currentDeviceState = false;
				if (!sendCommand(&connectedDevice, "L", "bool", "bool", false, 0, 0.0, &currentDeviceState, NULL, NULL)) {
					fprintf(stderr, "Failed to toggle main laser\n");
				}
				else {
					printf("Main Laser is now %s\n", currentDeviceState ? "ON" : "OFF");
				}
				break;
			}
			case 2: // Toggle Pilot Laser
				if (!sendCommand(&connectedDevice, "PL", "bool", "bool", false, 0, 0.0, NULL, NULL, NULL)) {
					fprintf(stderr, "Failed to toggle pilot laser\n");
				}
				break;
			case 3: { // Set Laser Current Target
				int lctValue = 0;
				printf("Enter LCT value (0-65535): ");
				scanf("%d", &lctValue);
				if (!sendCommand(&connectedDevice, "LCT", "bool", "int", false, lctValue, 0.0, NULL, NULL, NULL)) {
					fprintf(stderr, "Failed to set LCT value\n");
				}
				break;
			}
			case 4: { // Timed Run
				int seconds = 0;
				printf("Enter number of seconds for timed run: ");
				scanf("%d", &seconds);
				if (!sendCommand(&connectedDevice, "TR", "bool", "int", false, seconds * 1000, 0.0, NULL, NULL, NULL)) {
					fprintf(stderr, "Failed to start timed run\n");
				}
				break;
			}
			case 5: { // Set Pulse Width
				int pulseWidth = 0;
				printf("Enter Pulse Width in microseconds: ");
				scanf("%d", &pulseWidth);
				if (!sendCommand(&connectedDevice, "LMW", "bool", "int", false, pulseWidth, 0.0, NULL, NULL, NULL)) {
					fprintf(stderr, "Failed to set pulse width\n");
				}
				break;
			}
			case 6: { // Set Pulse Period
				int pulsePeriod = 0;
				printf("Enter Pulse Period in microseconds: ");
			}
		}
	}
}
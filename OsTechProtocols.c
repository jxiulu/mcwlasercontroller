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

void convertToASCII(char* input) {
	for (; *input; ++input)
		*input = (char)toupper((unsigned char)*input);
} // command formatting

int readLinesUntilEnd(Device* device, char* bufferForDataRead, size_t maximumNumberOfBytes) {
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

bool handshakeByEcho(Device* device, unsigned int timeOutMilliseconds) {
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

bool formatCommandValue(char* outboundValue, size_t	maximumOutboundBytes, const char* valueType,
	bool	boolValue, int		intValue, double		floatValue) {
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

bool sendLineAndVerifyEcho(Device* device, const char* lineToSend) {
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

bool readDeviceBoolReply(Device* device, bool* outputReply) {
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

bool readDeviceWordReply(Device* device, uint16_t* outputReply) {
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

bool readDeviceFloatReply(Device* device, float* outputReply) {
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

bool sendCommand(Device* device,
	const char* commandName,
	const char* replyType,
	const char* typeNameForValue,
	bool boolValue, int intValue, double floatValue,
	bool* outBool, uint16_t* outWord, float* outFloat)
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

bool setBinaryMode(Device* device, bool enableBinaryMode) {
	if (enableBinaryMode) {
		return sendCommand(device, "GMS", "word", "word", false, 8, 0.0, NULL, NULL, NULL);
	}
	else {
		return sendCommand(device, "GMC", "word", "word", false, 8, 0.0, NULL, NULL, NULL);
	}
}

bool connectToDevice(Device* device, const char* portName) {
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

void disconnectFromDevice(Device* device) {
	if (!device && !device->deviceIsConnected) {
		return;
	}

	sendCommand(device, "L", "bool", "bool", false, 0, 0.0, NULL, NULL, NULL);
	sendCommand(device, "PL", "bool", "bool", false, 0, 0.0, NULL, NULL, NULL);
	setBinaryMode(device, false);
	closeSerialPort(&device->deviceSerialPort);
	device->deviceIsConnected = false;
}
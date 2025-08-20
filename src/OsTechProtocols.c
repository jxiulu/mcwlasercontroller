#define _CRT_SECURE_NO_WARNINGS

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
		if (numberOfBytesReturned == 1 && returnedByte == ESCAPE_CHARACTER) {
			flushInput(&device->deviceSerialPort);
			return true;
		}
		SleepMs(50);
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
			snprintf(temp, sizeof(temp), "%.*g", position, floatValue);
			if ((int)strlen(temp) <= 9) { // keeping it to 9 characters
				snprintf(outboundValue, maximumOutboundBytes, "%s", temp);
				return true;
			}
		}
		// Fallback
		snprintf(outboundValue, maximumOutboundBytes, "%.3g", floatValue);
		return true;
	}
	else if (strcmp(valueType, "str") == 0) { // not used but i'll leave it here
		strncpy(outboundValue, "", maximumOutboundBytes);
		return true;
	}
	return false;
}

static void hexdumpOnFail(const char *label, const unsigned char *buffer, int number) {
	fprintf(stderr, "%s", label);
	for (int i = 0; i < number; i++) {
		fprintf(stderr, " %02X", buffer[i]);
	}
	fprintf(stderr, "\n");
}

bool sendLineAndVerifyEcho(Device* device, const char *lineToSend) {
	size_t lineLength = strlen(lineToSend); // line to send should already by capitalized
	char lineWithCR[128]; // device expects CR
	if (lineLength + 1 >= sizeof(lineWithCR)) {
		fprintf(stderr, "LINE TOO LONG TO SEND d: %s\n", lineToSend);
		return false;
	}
	snprintf(lineWithCR, sizeof(lineWithCR), "%s\r", lineToSend); // append CR

	flushInput(&device->deviceSerialPort);
	if (!writeBytes(&device->deviceSerialPort, lineWithCR, strlen(lineWithCR))) {
		fprintf(stderr, "FAILED TO WRITE COMMAND LINE %s\n", lineToSend);
		return false;
	}

	char receivedEcho[128] = { 0 };
	int receivedEchoByteCount = readLinesUntilEnd(device, receivedEcho, sizeof(receivedEcho));
	if (receivedEchoByteCount <= 0) {
		fprintf(stderr, "FAILED TO READ ECHO FOR LINE %s\n", lineToSend);
		return false;
	}

	char expectedEcho[128] = { 0 };
	snprintf(expectedEcho, sizeof(expectedEcho), "%s\r", lineToSend);

	if (strcmp(receivedEcho, expectedEcho) != 0) {
		fprintf(stderr, "ECHO MISMATCH \n SENT : %s \n GOT: %s\n", expectedEcho, receivedEcho);
		hexdumpOnFail("SENT HEX", expectedEcho, (int)strlen((char *)expectedEcho));
		hexdumpOnFail("GOT HEX", receivedEcho, (int)strlen((char *)receivedEcho));
		return false;
	}
	return true;
}

bool readDeviceBoolReply(Device* device, bool* boolReplyOutput) {
	unsigned char byteRead = 0;
	int numberOfBytesRead = readBytes(&device->deviceSerialPort, &byteRead, 1);
	if (numberOfBytesRead != 1)
		return false;
	if (byteRead == 0xAA) {
		*boolReplyOutput = true;
		return true;
	}
	if (byteRead == 0x55) {
		*boolReplyOutput = false;
		return true;
	}
	return false;
}

bool readDeviceWordReply(Device* device, uint16_t* wordReplyOutput) {
	unsigned char wordBuffer[3] = { 0 };
	int numberOfBytesRead = readBytes(&device->deviceSerialPort, wordBuffer, 3);
	if (numberOfBytesRead != 3) {
		return false;
	}
	unsigned int wordChecksum = (0x55 + wordBuffer[0] + wordBuffer[1]) & 0xFF;
	if (wordBuffer[2] != (unsigned char)wordChecksum)
		return false;
	*wordReplyOutput = (uint16_t)((wordBuffer[0] << 8) | wordBuffer[1]);
	return true;
}

bool readDeviceFloatReply(Device* device, float* floatReplyOutput) {
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
	float combinedFloat = 0.0f;
	memcpy(&combinedFloat, &combinedInt, sizeof(combinedFloat));
	*floatReplyOutput = combinedFloat;
	return true;
}

void formatCommandLine(const char* commandName, const char* commandVariableType, bool boolInput, int wordInput, double floatInput, char* commandLine) {
	if (commandVariableType) {
		char commandInputValue[32] = { 0 };
		if (formatCommandValue(commandInputValue, sizeof(commandInputValue), commandVariableType, boolInput, wordInput, floatInput)) {
			snprintf(commandLine, sizeof(commandLine), "%s%s", commandName, commandInputValue);
		}
	}
	else {
		snprintf(commandLine, sizeof(commandLine), "%s", commandName);
	}
	convertToASCII(commandLine);
}

bool sendFormattedCommandLine(Device* targetDevice, char* commandLine) {
	if (!sendLineAndVerifyEcho(targetDevice, commandLine)) { // attempt the command twice
		unsigned char CANCEL_BUFFERED_COMMANDS = 0x1B; // ESC character

		SleepMs(100);
		writeBytes(&targetDevice->deviceSerialPort, &CANCEL_BUFFERED_COMMANDS, 1);
		SleepMs(100);

		if (!sendLineAndVerifyEcho(targetDevice, commandLine)) {
			fprintf(stderr, "FAILED TO SEND COMMAND %s", commandLine);
			return false;
		}
	}
	return true;	
}

bool sendCommand(
	Device *device,
	const char *commandName,
	const char *replyVariableType,
	const char *commandVariableType,
	bool boolInput, int wordInput, double floatInput,
	bool *boolReplyOutput, uint16_t *wordReplyOutput, float *floatReplyOutput
) {

	char commandLine[128] = { 0 };
	formatCommandLine(commandName, commandVariableType, boolInput, wordInput, floatInput, commandLine);
	if (!sendFormattedCommandLine(device, commandLine)) {
		return false;
	}

	// Read the command
	// This function is a placeholder master function for now
	if (strcmp(replyVariableType, "bool") == 0) {
		bool receivedBoolReply = false;
		if (!readDeviceBoolReply(device, &receivedBoolReply)) {
			fprintf(stderr, "FAILED TO READ BOOL REPLY FOR COMMAND %s", commandLine);
			return false;
		}
		if (boolReplyOutput)
			*boolReplyOutput = receivedBoolReply;
	}
	else if (strcmp(replyVariableType, "float") == 0) {
		float receivedFloatReply = 0;
		if (!readDeviceFloatReply(device, &receivedFloatReply)) {
			fprintf(stderr, "FAILED TO READ FLOAT REPLY FOR COMMAND %s", commandLine);
			return false;
		}
		if (floatReplyOutput)
			*floatReplyOutput = receivedFloatReply;
	}
	else if (strcmp(replyVariableType, "word") == 0) {
		uint16_t receivedWordReply = 0;
		if (!readDeviceWordReply(device, &receivedWordReply)) {
			fprintf(stderr, "FAILED TO READ WORD REPLY FOR COMMAND %s", commandLine);
			return false;
		}
		if (wordReplyOutput)
			*wordReplyOutput = receivedWordReply;
	}
	else {
		fprintf(stderr, "UNKNOWN REPLY TYPE %s FOR COMMAND %s", replyVariableType, commandLine);
		return false;
	}

	flushInput(&device->deviceSerialPort);
	return true;
}

bool setBinaryMode(Device* device, bool binaryModeEnabled) {
	/*
	if (binaryModeEnabled) {
		return sendCommand(device, "GMS8", "word", "word", false, 8, 0.0, NULL, NULL, NULL);
	}
	else {
		return sendCommand(device, "GMC8", "word", "word", false, 8, 0.0, NULL, NULL, NULL);
	}
	*/
	const char *command = binaryModeEnabled ? "GMS8" : "GMC8";
	return sendLineAndVerifyEcho(device, command);
}

bool connectToDevice(Device* device, const char* portName) {
	memset(device, 0, sizeof(*device));
	if (!openSerialPort(&device->deviceSerialPort, portName, 9600)) {
		fprintf(stderr, "FAILED TO OPEN SERIAL PORT %s\n", portName);
		goto fail;
	}
	if (!handshakeByEcho(device, 1000)) {
		fprintf(stderr, "FAILED TO HANDSHAKE WITH DEVICE ON PORT %s\n", portName);
		goto fail;
	}
	if (!sendLineAndVerifyEcho(device, "GX R")) {
		fprintf(stderr, "FAILED TO ENTER EXTERNAL CONTROL\n");
		goto fail;
	}
	if (!setBinaryMode(device, true)) {
		fprintf(stderr, "FAILED TO ENABLE BINARY MODE\n");
		goto fail;
	}

	device->deviceIsConnected = true;
	return true;

	fail:
		closeSerialPort(&device->deviceSerialPort);
		return false;
}

void disconnectFromDevice(Device* device) {
	if (!device || !device->deviceIsConnected) {
		return;
	}

	sendCommand(device, "L", "bool", "bool", false, 0, 0.0, NULL, NULL, NULL);
	sendCommand(device, "PL", "bool", "bool", false, 0, 0.0, NULL, NULL, NULL);
	setBinaryMode(device, false);
	closeSerialPort(&device->deviceSerialPort);
	device->deviceIsConnected = false;
}

// Individual Variable Type Send Functions

bool SendBoolCommand(bool ReadOnly, Device* TargetDevice, char* CommandString, bool SetBool, bool* GetBool) {
	char *commandType = "bool";
	if (ReadOnly) commandType = NULL;

	char commandLine[128] = { 0 };
	formatCommandLine(CommandString, commandType, SetBool, 0, 0.0f, commandLine);
	sendFormattedCommandLine(TargetDevice, commandLine);

	bool deviceReply = false;
	if (!readDeviceBoolReply(TargetDevice, &deviceReply)) {
		fprintf(stderr, "FAILED TO READ BOOL REPLY FOR COMMAND %s", commandLine);
		return false;
	}

	*GetBool = deviceReply;
	return true;
}

bool SendFloatCommand(bool ReadOnly, Device* TargetDevice, char* CommandString, float SetFloat, float* GetFloat) {
	char *commandType = "float";
	if (ReadOnly) commandType = NULL;
	
	char commandLine[128] = { 0 };
	formatCommandLine(CommandString, commandType, false, 0, SetFloat, commandLine);
	if (!sendFormattedCommandLine(TargetDevice, commandLine)) return false;

	float deviceReply = 0.0f;
	if (!readDeviceFloatReply(TargetDevice, &deviceReply)) {
		fprintf(stderr, "FAILED TO READ FLOAT REPLY FOR COMMAND %s", commandLine);
		return false;
	}

	*GetFloat = deviceReply;
	return true;
}

bool SendWordCommand(bool ReadOnly, Device* TargetDevice, char* CommandString, int SetWord, uint16_t* GetWord) {
	char *commandType = "word";
	if (ReadOnly) commandType = NULL;
	
	char commandLine[128] = { 0 };
	formatCommandLine(CommandString, commandType, false, SetWord, 0.0f, commandLine);
	if (!sendFormattedCommandLine(TargetDevice, commandLine)) return false;

	uint16_t deviceReply = 0;
	if (!readDeviceWordReply(TargetDevice, &deviceReply)) {
		fprintf(stderr, "FAILED TO READ WORD REPLY FOR COMMAND %s", commandLine);
		return false;
	}

	*GetWord = deviceReply;
	return true;
}
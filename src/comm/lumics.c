#define _CRT_SECURE_NO_WARNINGS

#include "framework.h"
#include "serial-win.h"
#include "lumicscomm.h"

// Lumics OsTech Protocols 

void convertToASCII(char* input) {
	for (; *input; ++input)
		*input = (char)toupper((unsigned char)*input);
} // command formatting

int readLinesUntilEnd(SerialPort* DeviceSerialPort, char* bufferForDataRead, size_t maximumNumberOfBytes) {
	size_t bytePosition = 0;
	while (bytePosition + 1 < maximumNumberOfBytes) {
		char currentByte;
		int currentLineByteCount = readBytes(DeviceSerialPort, &currentByte, 1);

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

bool handshakeByEcho(SerialPort* DeviceSerialPort, unsigned int timeOutMilliseconds) {
	const unsigned char ESCAPE_CHARACTER = 0x1B;
	flushInput(DeviceSerialPort);
	DWORD timeOutDeadline = GetTickCount() + timeOutMilliseconds;

	while (GetTickCount() < timeOutDeadline) {
		writeBytes(DeviceSerialPort, &ESCAPE_CHARACTER, 1);
		unsigned char returnedByte = 0;
		int numberOfBytesReturned = readBytes(DeviceSerialPort, &returnedByte, 1);
		if (numberOfBytesReturned == 1 && returnedByte == ESCAPE_CHARACTER) {
			flushInput(DeviceSerialPort);
			return true;
		}
		SleepMs(50);
	}

	flushInput(DeviceSerialPort);
	return false;
}

bool formatCommandValue(char* formattedValue, size_t	maximumOutputBytes, CommandType ValueType,
	bool	boolValue, int		intValue, double		floatValue) {
	switch (ValueType) {
		case LUMICS_BOOL:
			snprintf(formattedValue, maximumOutputBytes, "%c", boolValue ? 'R' : 'S');
			return true;
		case LUMICS_WORD:
			if (intValue < 0)
				intValue = 0;
			if (intValue > 65535)
				intValue = 65535;
			snprintf(formattedValue, maximumOutputBytes, "%d", intValue);
			return true;
		case LUMICS_FLOAT:
			for (int position = 9; position >= 0; --position) {
			char temp[64];
			snprintf(temp, sizeof(temp), "%.*g", position, floatValue);
			if ((int)strlen(temp) <= 9) { // keeping it to 9 characters
				snprintf(formattedValue, maximumOutputBytes, "%s", temp);
				return true;
			}
			}
			// Fallback
			snprintf(formattedValue, maximumOutputBytes, "%.3g", floatValue);
			return true;
		default:
			fprintf(stderr, "Error: invalid command type\n");
			return false;
	}
}

static void hexdumpOnFail(const char *label, const unsigned char *buffer, int number) {
	fprintf(stderr, "%s", label);
	for (int i = 0; i < number; i++) {
		fprintf(stderr, " %02X", buffer[i]);
	}
	fprintf(stderr, "\n");
}

bool sendLineAndVerifyEcho(SerialPort* DeviceSerialPort, const char *lineToSend) {
	size_t lineLength = strlen(lineToSend); // line to send should already by capitalized
	char lineWithCR[128]; // TargetDevice expects CR
	if (lineLength + 1 >= sizeof(lineWithCR)) {
		fprintf(stderr, "LINE TOO LONG TO SEND %s\n", lineToSend);
		return false;
	}
	snprintf(lineWithCR, sizeof(lineWithCR), "%s\r", lineToSend); // append CR

	flushInput(DeviceSerialPort);
	if (!writeBytes(DeviceSerialPort, lineWithCR, strlen(lineWithCR))) {
		fprintf(stderr, "FAILED TO WRITE COMMAND LINE %s\n", lineToSend);
		return false;
	}

	char receivedEcho[128] = { 0 };
	int receivedEchoByteCount = readLinesUntilEnd(DeviceSerialPort, receivedEcho, sizeof(receivedEcho));
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

bool readDeviceBoolReply(SerialPort* DeviceSerialPort, bool* boolReplyOutput) {
	unsigned char byteRead = 0;
	int numberOfBytesRead = readBytes(DeviceSerialPort, &byteRead, 1);
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

bool readDeviceWordReply(SerialPort* DeviceSerialPort, uint16_t* wordReplyOutput) {
	unsigned char wordBuffer[3] = { 0 };
	int numberOfBytesRead = readBytes(DeviceSerialPort, wordBuffer, 3);
	if (numberOfBytesRead != 3) {
		return false;
	}
	unsigned int wordChecksum = (0x55 + wordBuffer[0] + wordBuffer[1]) & 0xFF;
	if (wordBuffer[2] != (unsigned char)wordChecksum)
		return false;
	*wordReplyOutput = (uint16_t)((wordBuffer[0] << 8) | wordBuffer[1]);
	return true;
}

bool readDeviceFloatReply(SerialPort* DeviceSerialPort, float* floatReplyOutput) {
	unsigned char floatBuffer[5] = { 0 };
	int numberOfBytesRead = readBytes(DeviceSerialPort, floatBuffer, 5);
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

inline void formatCommandLine(const char* commandString, CommandType ValueType, bool boolInput, int wordInput, double floatInput, char* commandLine) {
	if (ValueType == NOCOMMAND) {
		char commandInputValue[32] = { 0 };
		if (formatCommandValue(commandInputValue, sizeof(commandInputValue), ValueType, boolInput, wordInput, floatInput)) {
			snprintf(commandLine, sizeof(commandLine), "%s%s", commandString, commandInputValue);
		}
	}
	else {
		snprintf(commandLine, sizeof(commandLine), "%s", commandString);
	}
	convertToASCII(commandLine);
}

inline bool sendCommandLine(SerialPort* DeviceSerialPort, char* commandLine) {
	if (!sendLineAndVerifyEcho(DeviceSerialPort, commandLine)) { // attempt the command twice
		unsigned char CANCEL_BUFFERED_COMMANDS = 0x1B; // ESC

		SleepMs(100);
		writeBytes(DeviceSerialPort, &CANCEL_BUFFERED_COMMANDS, 1);
		SleepMs(100);

		if (!sendLineAndVerifyEcho(DeviceSerialPort, commandLine)) {
			return false;
		}
	}
	return true;	
}

bool setBinaryMode(SerialPort* DeviceSerialPort, bool binaryModeEnabled) {
	const char *setBinaryCommand = binaryModeEnabled ? "GMS8" : "GMC8";
	return sendLineAndVerifyEcho(DeviceSerialPort, setBinaryCommand);
}

inline bool getReply(SerialPort* DeviceSerialPort, CommandType ReplyType, bool *GetBool, uint16_t *GetWord, float *GetFloat, char *CommandLine) {
	switch (ReplyType) {
		case LUMICS_BOOL:
			bool receivedBoolReply = false;
			if (!readDeviceBoolReply(DeviceSerialPort, &receivedBoolReply)) {
				fprintf(stderr, "FAILED TO READ BOOL REPLY FOR COMMAND %s\n", CommandLine);
				return false;
			}
			if (GetBool)
				*GetBool = receivedBoolReply;
			break;
		case LUMICS_WORD:
			uint16_t receivedWordReply = 0;
			if (!readDeviceWordReply(DeviceSerialPort, &receivedWordReply)) {
				fprintf(stderr, "FAILED TO READ WORD REPLY FOR COMMAND %s\n", CommandLine);
				return false;
			}
			if (GetWord)
				*GetWord = receivedWordReply;
			break;
		case LUMICS_FLOAT:
			float receivedFloatReply = 0;
			if (!readDeviceFloatReply(DeviceSerialPort, &receivedFloatReply)) {
				fprintf(stderr, "FAILED TO READ FLOAT REPLY FOR COMMAND %s\n", CommandLine);
				return false;
			}
			if (GetFloat)
				*GetFloat = receivedFloatReply;
			break;
		default:
			fprintf(stderr, "Error: UNKNOWN REPLY TYPE FOR COMMAND %s\n", CommandLine);
			return false;
		}
	return true;
}


// Send Functions

bool SendGeneralCommand(
	SerialPort* DeviceSerialPort,
	const char *CommandString,
	CommandType ReplyType,
	CommandType InputType,
	bool SetBool, int SetWord, double SetFloat,
	bool *GetBool, uint16_t *GetWord, float *GetFloat
) {
	char commandLine[128] = { 0 };

	formatCommandLine(CommandString, InputType, SetBool, SetWord, SetFloat, commandLine);
	if (!sendCommandLine(DeviceSerialPort, commandLine))
		return false;
	if (!getReply(DeviceSerialPort, ReplyType, GetBool, GetWord, GetFloat, commandLine))
		return false;

	flushInput(DeviceSerialPort);
	return true;
}

bool SendBoolCommand(InstructionType Instruction, SerialPort* DeviceSerialPort, const char* CommandString, bool SetBool, bool* GetBool) {
	CommandType valueType = LUMICS_BOOL;
	if (Instruction == READ) {
		valueType = NOCOMMAND;
	}

	char commandLine[128] = { 0 };
	formatCommandLine(CommandString, valueType, SetBool, 0, 0.0f, commandLine);
	if (!sendCommandLine(DeviceSerialPort, commandLine))
		return false;
	if (!getReply(DeviceSerialPort, valueType, GetBool, NULL, NULL, commandLine))
		return false;

	return true;
}

bool SendWordCommand(InstructionType Instruction, SerialPort* DeviceSerialPort, const char* CommandString, int SetWord, uint16_t* GetWord) {
	CommandType valueType = LUMICS_WORD;
	if (Instruction == READ) {
		valueType = NOCOMMAND;
	}
	
	char commandLine[128] = { 0 };
	formatCommandLine(CommandString, valueType, false, SetWord, 0.0f, commandLine);
	if (!sendCommandLine(DeviceSerialPort, commandLine))
		return false;
	if (!getReply(DeviceSerialPort, valueType, NULL, GetWord, NULL, commandLine))
		return false;
	return true;
}

bool SendFloatCommand(InstructionType Instruction, SerialPort* DeviceSerialPort, const char* CommandString, float SetFloat, float* GetFloat) {
	CommandType valueType = LUMICS_FLOAT;
	if (Instruction == READ) {
		valueType = NOCOMMAND;
	}
	
	char commandLine[128] = { 0 };
	formatCommandLine(CommandString, valueType, false, 0, SetFloat, commandLine);
	if (!sendCommandLine(DeviceSerialPort, commandLine))
		return false;
	if (!getReply(DeviceSerialPort, valueType, NULL, NULL, GetFloat, commandLine))
		return false;
	return true;
}

// Connect and Disconnect

bool ConnectDevice(SerialPort* DeviceSerialPort, const char* PortName, bool *ReceiveConnectedState) {
	#ifndef GUI
	memset(DeviceSerialPort, 0, sizeof(*DeviceSerialPort));
	#endif

	if (!OpenSerialPort(DeviceSerialPort, PortName, 9600)) {
		fprintf(stderr, "FAILED TO OPEN SERIAL PORT %s\n", PortName);
		goto fail;
	}
	if (!handshakeByEcho(DeviceSerialPort, 1000)) {
		fprintf(stderr, "FAILED TO HANDSHAKE WITH DEVICE ON PORT %s\n", PortName);
		goto fail;
	}
	if (!sendLineAndVerifyEcho(DeviceSerialPort, "GX R")) {
		fprintf(stderr, "FAILED TO ENTER EXTERNAL CONTROL\n");
		goto fail;
	}
	if (!setBinaryMode(DeviceSerialPort, true)) {
		fprintf(stderr, "FAILED TO ENABLE BINARY MODE\n");
		goto fail;
	}

	*ReceiveConnectedState = true;
	return true;

	fail:
		CloseSerialPort(DeviceSerialPort);
		return false;
}

bool DisconnectDevice(SerialPort* DeviceSerialPort, bool ConnectedState, bool *ReceiveConnectedState) {
	if (!ConnectedState) {
		return true;
	}

	bool safeToDisconnect = false;
	if (!SendBoolCommand(WRITE, DeviceSerialPort, "PL", false, NULL)) {
		printf("Warning: failed to disable pilot laser. Program will exit if main laser is disabled.\n");
		safeToDisconnect = true;
	}
	if (!SendBoolCommand(WRITE, DeviceSerialPort, "L", false, NULL)) {
		printf("Warning: failed to disable main laser.\n");
		safeToDisconnect = false;
	}

	setBinaryMode(DeviceSerialPort, false);
	CloseSerialPort(DeviceSerialPort);
	*ReceiveConnectedState = false;

	safeToDisconnect = true;
	return safeToDisconnect;
}
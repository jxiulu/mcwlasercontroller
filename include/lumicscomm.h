#ifndef LUMICSCOMM_H
#define LUMICSCOMM_H

#define _CRT_SECURE_NO_WARNINGS

#include "serial-win.h"

#ifdef _cplusplus
extern "C" {
using namespace ser;
#endif

typedef enum {
	NOCOMMAND = 0,
	LUMICS_BOOL,
	LUMICS_WORD,
	LUMICS_FLOAT,
} CommandType;

typedef enum {
	READ = 0,
	WRITE,
} InstructionType;

#ifndef GUI
typedef struct {
	SerialPort			DeviceSerialPort;
	bool				deviceIsConnected;
} Device;
#endif

// Lumics OsTech Protocols 

void convertToASCII(char* input);

int readLinesUntilEnd(SerialPort* DeviceSerialPort, char* bufferForDataRead, size_t maximumNumberOfBytes);

bool handshakeByEcho(SerialPort* DeviceSerialPort, unsigned int timeOutMilliseconds);

bool formatCommandValue(char* formattedValue, size_t	maximumOutputBytes, CommandType ValueType,
	bool	boolValue, int		intValue, double		floatValue);

static void hexdumpOnFail(const char *label, const unsigned char *buffer, int number);

bool sendLineAndVerifyEcho(SerialPort* DeviceSerialPort, const char *lineToSend);

bool readDeviceBoolReply(SerialPort* DeviceSerialPort, bool* boolReplyOutput);

bool readDeviceWordReply(SerialPort* DeviceSerialPort, uint16_t* wordReplyOutput);

bool readDeviceFloatReply(SerialPort* DeviceSerialPort, float* floatReplyOutput);

inline void formatCommandLine(const char* commandString, CommandType ValueType, bool boolInput, int wordInput, double floatInput, char* commandLine);

inline bool sendCommandLine(SerialPort* DeviceSerialPort, char* commandLine);

bool setBinaryMode(SerialPort* DeviceSerialPort, bool binaryModeEnabled);

inline bool getReply(SerialPort* DeviceSerialPort, CommandType ReplyType, bool *GetBool, uint16_t *GetWord, float *GetFloat, char *CommandLine);


// Send Functions

bool SendGeneralCommand(
	SerialPort* DeviceSerialPort,
	const char *CommandString,
	CommandType ReplyType,
	CommandType InputType,
	bool SetBool, int SetWord, double SetFloat,
	bool *GetBool, uint16_t *GetWord, float *GetFloat
);

bool SendBoolCommand(InstructionType Instruction, SerialPort* DeviceSerialPort, const char* CommandString, bool SetBool, bool* GetBool);

bool SendWordCommand(InstructionType Instruction, SerialPort* DeviceSerialPort, const char* CommandString, int SetWord, uint16_t* GetWord);

bool SendFloatCommand(InstructionType Instruction, SerialPort* DeviceSerialPort, const char* CommandString, float SetFloat, float* GetFloat);

// Connect and Disconnect

bool ConnectDevice(SerialPort* DeviceSerialPort, const char* PortName, bool *ReceiveConnectedState);

bool DisconnectDevice(SerialPort* DeviceSerialPort, bool ConnectedState, bool *ReceiveConnectedState);

#ifdef _cplusplus
}
#endif

#endif
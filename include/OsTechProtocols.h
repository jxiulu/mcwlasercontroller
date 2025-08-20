//	OsTechProtocols.h

#ifndef OSTECH_PROTOCOLS_H
#define OSTECH_PROTOCOLS_H

#include "SerialPortProtocols.h"

typedef struct {
	SerialPort			deviceSerialPort;
	bool				deviceIsConnected;
} Device;

void convertToASCII(char* input);

int readLinesUntilEnd(Device* device, char* bufferForDataRead, size_t maximumNumberOfBytes);

bool handshakeByEcho(Device* device, unsigned int timeOutMilliseconds);

bool formatCommandValue(char* outboundValue, size_t	maximumOutboundBytes, const char* valueType,
	bool	boolValue, int		intValue, double		floatValue);

bool sendLineAndVerifyEcho(Device* device, const char* lineToSend);

bool readDeviceBoolReply(Device* device, bool* outputReply);

bool readDeviceWordReply(Device* device, uint16_t* outputReply);

bool readDeviceFloatReply(Device* device, float* outputReply);

bool sendCommand(
	Device *device,
	const char *commandName,
	const char *replyVariableType,
	const char *commandVariableType,
	bool boolInput, int wordInput, double floatInput,
	bool *boolReplyOutput, uint16_t *wordReplyOutput, float *floatReplyOutput);

bool setBinaryMode(Device* device, bool binaryModeEnabled);

bool connectToDevice(Device* device, const char* portName);

void disconnectFromDevice(Device* device);

// Type-specific command functions
bool SendBoolCommand(bool ReadOnly, Device* TargetDevice, char* CommandString, bool SetBool, bool* GetBool);

bool SendFloatCommand(bool ReadOnly, Device* TargetDevice, char* CommandString, float SetFloat, float* GetFloat);

bool SendWordCommand(bool ReadOnly, Device* TargetDevice, char* CommandString, int SetWord, uint16_t* GetWord);

#endif
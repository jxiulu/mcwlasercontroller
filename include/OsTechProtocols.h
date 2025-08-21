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

bool setBinaryMode(Device* device, bool binaryModeEnabled);

bool ConnectDevice(Device* device, const char* portName);

bool DisconnectDevice(Device* device);

// Send Command Functions

bool SendGeneralCommand(
	Device *TargetDevice,
	const char *CommandString,
	const char *ReplyVariableType,
	const char *InputVariableType,
	bool SetBool, int SetWord, double SetFloat,
	bool *GetBool, uint16_t *GetWord, float *GetFloat
);

bool SendBoolCommand(bool ReadOnly, Device* TargetDevice, char* CommandString, bool SetBool, bool* GetBool);

bool SendFloatCommand(bool ReadOnly, Device* TargetDevice, char* CommandString, float SetFloat, float* GetFloat);

bool SendWordCommand(bool ReadOnly, Device* TargetDevice, char* CommandString, int SetWord, uint16_t* GetWord);

#endif
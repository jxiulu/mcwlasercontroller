#ifndef OSTECH_PROTOCOLS_H
#define OSTECH_PROTOCOLS_H

#include "SerialPortProtocols.h"
#include "OsTechProtocols.h"
#include <stdint.h>

typedef struct {
	SerialPort	deviceSerialPort;
	bool		deviceIsConnected;
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

bool sendCommand(Device* device,
	const char* commandName,
	const char* replyType,
	const char* typeNameForValue,
	bool boolValue, int intValue, double floatValue,
	bool* outBool, uint16_t* outWord, float* outFloat);

bool setBinaryMode(Device* device, bool enableBinaryMode);

bool connectToDevice(Device* device, const char* portName);

void disconnectFromDevice(Device* device);

#endif
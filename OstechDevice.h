#ifndef OSTECH_DEVICE_H
#define OSTECH_DEVICE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	OSTECH_NONE = 0,
	OSTECH_BOOL,
	OSTECH_WORD,
	OSTECH_FLOAT,
} OstechType;

typedef struct OstechDevice OstechDevice;

OstechDevice*	deviceOpen(const char* port, int baudRate);
bool			deviceConnect(OstechDevice* d);
void			deviceClose(OstechDevice* d);
bool			deviceIsConnected(const OstechDevice* d);

bool deviceSendCommand(	OstechDevice*	d,
						const char*		commandASCII,
						OstechType		rType,
						const void*		newValue,
						void*			outValue		);

bool 

}
#endif
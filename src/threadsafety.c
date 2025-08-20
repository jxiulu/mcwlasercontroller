#include "framework.h"

// wip -- only needed for gui
// if we dont want thread interlacing, a critical section is needed

typedef struct {
	bool MainLaserOn;
	bool PilotLaserOn;
	bool GatedModeOn;
	float CurrentTempCelsius;
} PollingStates;

static PollingStates 	DeviceStates;
static CRITICAL_SECTION DeviceCriticalSection;
static bool 			DeviceCriticalSectionInitialized;

static void initiateStateLock(void) {
    if(!DeviceCriticalSectionInitialized) {
        InitializeCriticalSection(&DeviceCriticalSection);
        DeviceCriticalSectionInitialized = true;
    }
}

static void deleteStateLock(void) {
    if (DeviceCriticalSectionInitialized) {
        DeleteCriticalSection(&DeviceCriticalSection);
        DeviceCriticalSectionInitialized = false;
    }
}

// get shared states

static void SetMainLaser(bool SetBool) {
    EnterCriticalSection(&DeviceCriticalSection);
    DeviceStates.MainLaserOn = SetBool;
    LeaveCriticalSection(&DeviceCriticalSection);
}
static bool GetMainLaser(void) {
    bool mainLaserOn = false;
    EnterCriticalSection(&DeviceCriticalSection);
    mainLaserOn = DeviceStates.MainLaserOn;
    LeaveCriticalSection(&DeviceCriticalSection);
    return mainLaserOn;
}
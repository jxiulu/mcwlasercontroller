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

    printf("Connected to device on port %s\n", connectedDevice.deviceSerialPort.portName);

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
				bool mainLaserStatus = false;
				if (!sendCommand(&connectedDevice, "L", "bool", "bool", false, 0, 0.0, &mainLaserStatus, NULL, NULL)) {
					fprintf(stderr, "Failed to toggle main laser\n");
				}
				else {
					printf("Main Laser is now %s\n", mainLaserStatus ? "ON" : "OFF");
				}
				break;
			}
			case 2: {// Toggle Pilot Laser 
				bool pilotLaserStatus = false;
				if (!sendCommand(&connectedDevice, "PL", "bool", "bool", false, 0, 0.0, pilotLaserStatus, NULL, NULL)) {
					fprintf(stderr, "Failed to toggle pilot laser\n");
				}
				else {
					printf("Pilot Laser is now %s\n", pilotLaserStatus ? "ON" : "OFF");
				}
				break;
			}
			case 3: { // Set Laser Current Target
				//
			}
			case 4: { // Timed Run
				//
			}
			case 5: { // Set Pulse Width
				//
			}
			case 6: { // Set Pulse Period
				//
			}
		}
	}
}
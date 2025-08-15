#define _CRT_SECURE_NO_WARNINGS

#include "SerialPortProtocols.h"
#include "OsTechProtocols.h"

// application

static volatile bool stopPolling = false;

static DWORD WINAPI temperaturePollingThread(LPVOID parameter) {
	sleepMilliseconds(100); // Sleep so we don't get to close to the main thread
	Device *device = (Device*)parameter;

	while (!stopPolling) {
		float currentTemperatureCelsius = 0.0f;
		if (sendCommand(device, "1TA", "float", NULL, false, 0, 0.0, NULL, NULL, &currentTemperatureCelsius)) {
			printf("Current Temperature: %.2f degrees Celsius\n", currentTemperatureCelsius);
		}
		else {
			fprintf(stderr, "FAILED TO GET TEMPERATURE\n");
		}
		sleepMilliseconds(1000);
	}

	return 0;
}

static void startMenu(void) {
	printf("\nLaser Controller Menu\n");
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

	// Initial check
	bool mainLaserOn = false;
	bool pilotLaserOn = false;
	if (!sendCommand(&connectedDevice, "L", "bool", NULL, false, 0, 0.0, &mainLaserOn, NULL, NULL))
		fprintf(stderr, "FAILED TO READ MAIN LASER STATE\n");
	if (!sendCommand(&connectedDevice, "PL", "bool", NULL, false, 0, 0.0, &pilotLaserOn, NULL, NULL))
		fprintf(stderr, "FAILED TO READ PILOT LASER STATEr\n");

	HANDLE pollingThreadHandle = CreateThread(NULL, 0, temperaturePollingThread, &connectedDevice, 0, NULL);
	// Apparently creating this thread before running the initial check causes interlacing and an echo mismatch
	// replies from the initial check are buffered into the handshake echo

	while (true) {
		startMenu();
		int selectedOption = 0;
		if (scanf("%d", &selectedOption) != 1) {
			fprintf(stderr, "Invalid input. Please enter a number.\n");
			while (getchar() != '\n'); // clear input buffer
			continue;
		}

		switch (selectedOption) {
			case 1: {
				printf("Main laser was %s\n", mainLaserOn ? "ON" : "OFF");

				bool mainLaserDesiredState = !mainLaserOn;
				if (!sendCommand(&connectedDevice, "L", "bool", "bool", mainLaserDesiredState, 0, 0.0, &mainLaserOn, NULL, NULL)) {
					fprintf(stderr, "FAILED TO TOGGLE MAIN LASER STATE\n");
				}
				else {
					printf("Main Laser is now %s\n", mainLaserDesiredState ? "ON" : "OFF");
				}
				break;
			}
			case 2: {
				printf("Pilot laser was %s\n", pilotLaserOn ? "ON" : "OFF");

				bool pilotLaserDesiredState = !pilotLaserOn;
				if (!sendCommand(&connectedDevice, "PL", "bool", "bool", pilotLaserDesiredState, 0, 0.0, &pilotLaserOn, NULL, NULL)) {
					fprintf(stderr, "FAILED TO TOGGLE MAIN LASER STATE\n");
				}
				else {
					printf("Pilot Laser is now %s\n", pilotLaserDesiredState ? "ON" : "OFF");
				}
				break;
			}
			case 3: { // Set Laser Current Target
				// TODO
				continue;
			}
			case 4: { // Timed Run
				// TODO
				continue;
			}
			case 5: { // Set Pulse Width
				// TODO
				continue;
			}
			case 6: { // Set Pulse Period
				// TODO
				continue;
			}
		}
	}
}
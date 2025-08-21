#define _CRT_SECURE_NO_WARNINGS

#include "SerialPortProtocols.h"
#include "OsTechProtocols.h"

// application

static volatile bool polling = true;

static DWORD WINAPI pollingThread(LPVOID parameter) {
	SleepMs(100); // Sleep so we don't get to close to the main thread
	Device *device = (Device*)parameter;

	if (polling) {
		if (!SendFloatCommand(true, device, "1TA", 0.0, NULL)) {
			fprintf(stderr, "Warning: failed to poll temperature\n");
		}
	}

	while (polling) {
		float currentTemperatureCelsius = 0.0f;

		SendFloatCommand(true, device, "1TA", 0.0, &currentTemperatureCelsius);
		printf("Current Temperature: %.2f degrees Celsius\n", currentTemperatureCelsius);

		SleepMs(1000);
	}

	return 0;
}

static void printStartMenu(void) {
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
	printf("10) Quit (force shutdown)\n");
	printf("Choose: ");

	fflush(stdout);
}

int main(int argc, char** argv) {
	const char* PORT = "COM5";
	
	if (argc >= 2) {
		PORT = argv[1]; // allow overriding;
	}

	Device connectedDevice = { 0 };
	if (!ConnectDevice(&connectedDevice, PORT)) {
		fprintf(stderr, "Failed to connect to device on port %s\n", PORT);

		char failedToConnectOption = 0;
		printf("Press enter to exit, or + then enter to ignore.\n");
		scanf("%c", &failedToConnectOption);
		if (failedToConnectOption != '+') {
			return 1;
		}
		printf("Warning: lots of errors will be thrown\n");
	}
	else {
 		printf("Connected to device on port %s\n", connectedDevice.deviceSerialPort.portName);
	}

	// Initial check
	bool initialCheckPassed = false;
	bool mainLaserOn = false;
	bool pilotLaserOn = false;
	if (!SendBoolCommand(true, &connectedDevice, "L", false, &mainLaserOn))
		initialCheckPassed = false;
	if (!SendBoolCommand(true, &connectedDevice, "PL", false, &pilotLaserOn))
		initialCheckPassed = false;
	if (!initialCheckPassed) {
		fprintf(stderr, "Error in initial check; possible thread interlacing\n");
	}

	HANDLE pollingThreadHandle = CreateThread(NULL, 0, pollingThread, &connectedDevice, 0, NULL);
	// Apparently creating this thread before running the initial check causes interlacing and an echo mismatch
	// replies from the initial check are buffered into the handshake echo

	bool running = true;
	while (running) {
		printStartMenu();
		int selectedOption = 0;
		if (scanf("%d", &selectedOption) != 1) {
			fprintf(stderr, "Invalid input. Please enter a number.\n");
			while (getchar() != '\n'); // clear input buffer
			continue;
		}

		switch (selectedOption) {
			case 1: {
				bool mainLaserDesiredState = !mainLaserOn;
				
				printf("Main laser was %s\n", mainLaserOn ? "ON" : "OFF");
				if (!SendBoolCommand(false, &connectedDevice, "L", mainLaserDesiredState, &mainLaserOn))
					break;
				printf("Main Laser is now %s\n", mainLaserDesiredState ? "ON" : "OFF");

				break;
			}
			case 2: {
				bool pilotLaserDesiredState = !pilotLaserOn;

				printf("Pilot laser was %s\n", pilotLaserOn ? "ON" : "OFF");
				if (!SendBoolCommand(false, &connectedDevice, "PL", pilotLaserDesiredState, &pilotLaserOn))
					break;
				printf("Pilot Laser is now %s\n", pilotLaserDesiredState ? "ON" : "OFF");
				
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
			case 9: { // Exit (safe shutdown)
				if (!DisconnectDevice(&connectedDevice)) {
					printf("Couldn't safely disconnect from device.\n");
					break;
				}
				
				running = false;
			}

			// force shutdown -- WIP
			/* case 10: {
				if (!DisconnectDevice(&connectedDevice)) {
					printf("Proceed? y/n: ");

					char *forcedDisconnectChoice = "";
					bool proceedForcedDisconnect = false;
					if (scanf("%s", &forcedDisconnectChoice) != 1) {
						fprintf(stderr, "Invalid input.\n");
						while (getchar() != '\n');
						continue;
					}

					if (strcmp(forcedDisconnectChoice, "y") == 0) {
						proceedForcedDisconnect = true;
					}
					else if (strcmp(forcedDisconnectChoice, "n") == 0) {
						proceedForcedDisconnect = false;
					}
					else {
						continue;
					}
				}
			} */
		}
	}
}
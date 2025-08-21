// todo: gui in dear imgui

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>

namespace Lumics {
    extern "C" {
        #include "OsTechProtocols.h"
    }
}

namespace Serial {
    extern "C" {
        #include "SerialPortProtocols.h"
    }
}

/*
namespace Global {
    struct TemperatureSensors_struct {
        std::atomic<float> Sensor1 = 0.0f;
        std::atomic<float> Sensor2 = 0.0f;
        //TOOD rest of the sensors 
    };
    typedef TemperatureSensors_struct TemperatureSensors;

    // read by 1TA, 2TA, etc. as per manual

    struct structDeviceState {
        Laser::Device       Link; // todo: fix
        std::mutex          MutexLock;
        std::atomic<bool>   Polling = false;
        std::atomic<bool>   MainLaserOn;
        std::atomic<bool>   PilotLaserOn;
        TemperatureSensors  Temperature;
    };
    typedef structDeviceState DeviceState;
}
*/

class Laser {
    public:
    struct Temperature {
        float Sensor1 = 0.0f;
        float Sensor2 = 0.0f;
        // Rest of the sensors read by 1TA, 2TA, etc.
    };
    struct PulseSettings {
        uint16_t    PulseWidth = 1000;
        uint16_t    PulsePeriod = 1000;
        uint16_t    PulseNumber = 1000;
    };
    struct Control {
        bool MainLaserOn = false;
        bool PilotLaserOn = false;
        bool GatedMode = false;
    };
};

// TODO: try to import necessary functions from /src for global polling thread

int main() {
    std::cout << "test" << std::endl;
    std::cin.get();
    return 0;
}

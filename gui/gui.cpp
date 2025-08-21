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

namespace Laser {
    extern "C" {
        #include "OsTechProtocols.h"
    }
}

namespace Serial {
    extern "C" {
        #include "SerialPortProtocols.h"
    }
}

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

// TODO: try to import necessary functions from /src for global polling thread

Global::DeviceState CurrentDevice = {};

int main() {
    std::cout << "Hello world!" << std::endl;
    std::cin;
    return 0;
}

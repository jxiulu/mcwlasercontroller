// todo: gui in dear imgui

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <iostream>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>

#define GUI

extern "C" {
    #include "framework.h"
}

namespace ser {
    extern "C" {
        #include "serial-win.h"
    }
}

namespace lum {
    using namespace ser;
    extern "C" {
        #include "lumicscomm.h"
    }
}

class Laser {
private:
    struct temperature {
        float Sensor1 = 0.0f;
        float Sensor2 = 0.0f;
        // Rest of the sensors read by 1TA, 2TA, etc.
    };
    struct pulse {
        uint16_t    Width = 1000;
        uint16_t    Period = 1000;
        uint16_t    Number = 1000;
    };
    struct controls {
        bool MainLaserOn = false;
        bool PilotLaserOn = false;
        bool GatedMode = false;
    };
    struct states {
        bool IsConnected = false;
    };

public:
    struct temperature Temperature {};
    struct pulse Pulse {};
    struct controls Controls {};
    struct states States {};
    ser::SerialPort DevicePort;

    bool Command(lum::InstructionType Instruction, lum::CommandType ValueType, const std::string CommandName, std::optional<bool> SetValue, bool *GetValue) {
        if (ValueType != lum::LUMICS_BOOL) {
            std::cerr << "Error: Command type mismatch, inputs expect type BOOL";
            return false;
        }
        if (Instruction == lum::READ && !GetValue) {
            std::cerr << "Error: No output pointer for device reply for instruction type READ\n";
            return false;
        }
        if (Instruction == lum::WRITE && !SetValue.has_value()) {
            std::cerr << "Error: No value to set for instruction type WRITE";
            return false;
        }
 
        return lum::SendBoolCommand(Instruction, &DevicePort, CommandName.c_str(), SetValue.value_or(false), GetValue);
    }

    bool Command(lum::InstructionType Instruction, lum::CommandType ValueType, const std::string CommandName, std::optional<int> SetValue, uint16_t *GetValue) {
        if (ValueType != lum::LUMICS_WORD) {
            std::cerr << "Error: Command type mismatch, inputs expect type WORD";
            return false;
        }
        if (Instruction == lum::READ && !GetValue) {
            std::cerr << "Error: No output pointer for device reply for instruction type READ\n";
            return false;
        }
        if (Instruction == lum::WRITE && !SetValue.has_value()) {
            std::cerr << "Error: No value to set for instruction type WRITE";
            return false;
        }
 
        return lum::SendWordCommand(Instruction, &DevicePort, CommandName.c_str(), SetValue.value_or(0), GetValue);
    }

    bool Command(lum::InstructionType Instruction, lum::CommandType ValueType, const std::string CommandName, std::optional<float> SetValue, float *GetValue) {
        if (ValueType != lum::LUMICS_FLOAT) {
            std::cerr << "Error: Command type mismatch, inputs expect type FLOAT";
            return false;
        }
        if (Instruction == lum::READ && !GetValue) {
            std::cerr << "Error: No output pointer for device reply for instruction type READ\n";
            return false;
        }
        if (Instruction == lum::WRITE && !SetValue.has_value()) {
            std::cerr << "Error: No value to set for instruction type WRITE";
            return false;
        }
 
        return lum::SendFloatCommand(Instruction, &DevicePort, CommandName.c_str(), SetValue.value_or(0.0f), GetValue);
    }

    bool ConnectToPort(const std::string PortName) {
        // memset(this, 0, sizeof(*this));
        return lum::ConnectDevice(&this->DevicePort, PortName.c_str(), &this->States.IsConnected);
    }
    bool DisconnectDevice() {
        //returns safe to disconnect
        return lum::DisconnectDevice(&this->DevicePort, this->States.IsConnected, &this->States.IsConnected);
    }
};

// TODO: try to import necessary functions from /src for global polling thread

int main() {
    const std::string CurrentPort = "COM5";
    
    Laser CurrentDevice;
    if (!CurrentDevice.ConnectToPort(CurrentPort)) {
        std::cerr << "Error: failed to connect to device on port " << CurrentPort << "\n";
    }

    //WIP
    std::cin.get();
    return 0;
}

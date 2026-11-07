#pragma once
#include <driver/i2c_master.h>

// A missed status update is optional telemetry, not a fatal device failure.
inline bool ReadWaveshareBatteryStatus(i2c_master_dev_handle_t device, int& level, bool& charging,
                                       bool& discharging) {
    uint8_t reg = 0x01, status = 0, percent = 0;
    if (i2c_master_transmit_receive(device, &reg, 1, &status, 1, 50) != ESP_OK)
        return false;
    reg = 0xA4;
    if (i2c_master_transmit_receive(device, &reg, 1, &percent, 1, 50) != ESP_OK)
        return false;
    if (percent > 100)
        return false;
    level = percent;
    charging = ((status >> 5) & 3) == 1;
    discharging = ((status >> 5) & 3) == 2;
    return true;
}

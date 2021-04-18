#include <unistd.h>
#include <cmath>

#include <wiringPiI2C.h>

#include "pca9685.h"
#include "pca9685_constants.h"

PCA9685::PCA9685(int i2c_bus, int device_address) :
        i2c_bus_(i2c_bus), device_address_(device_address) {}

PCA9685::PCA9685(int device_address) :
        i2c_bus_(1), device_address_(device_address) {}

int PCA9685::connect() {
    file_descriptor_ = wiringPiI2CSetup(device_address_);
    // TODO: Can still return a positive fd if device is not connected
    if (file_descriptor_ < 0) {
        return -1;
    }
    set_all_pwm(0, 0);
    // Set mode2
    wiringPiI2CWriteReg8(file_descriptor_, MODE2, OUTDRV);
    // Set mode1
    wiringPiI2CWriteReg8(file_descriptor_, MODE1, ALLCALL);
    // Wait for oscillator. Takes 500 microseconds
    usleep(500);
    // Read mode1
    int mode1 = wiringPiI2CReadReg8(file_descriptor_, MODE1);
    mode1 = mode1 & ~SLEEP;
    // Write mode1
    wiringPiI2CWriteReg8(file_descriptor_, MODE1, mode1);
    // Wait for oscillator. Takes 500 microseconds
    usleep(500);
    return 1;
}

void PCA9685::set_pwm_freq(float frequency_hz) {
    frequency_hz_ = frequency_hz;
    const float oscillator_clock_freq_hz = 25000000.0f;

    // Refer to the PCA9685 documentation for details on the prescale value.
    int prescale_val = static_cast<int>(std::round(oscillator_clock_freq_hz / (PWM_RESOLUTION * frequency_hz)) - 1.0f);

    int current_mode = wiringPiI2CReadReg8(file_descriptor_, MODE1);
    int new_mode = (current_mode & 0x7F) | SLEEP;

    wiringPiI2CWriteReg8(file_descriptor_, MODE1, new_mode);
    wiringPiI2CWriteReg8(file_descriptor_, PRESCALE, prescale_val);
    wiringPiI2CWriteReg8(file_descriptor_, MODE1, current_mode);
    // Wait for oscillator. Takes 500 microseconds
    usleep(500);
    wiringPiI2CWriteReg8(file_descriptor_, MODE1, current_mode | RESTART);
}

void PCA9685::set_pwm(int channel, uint16_t on, uint16_t off) {
    wiringPiI2CWriteReg8(file_descriptor_, LED0_ON_L + 4 * channel, on & 0xFF);
    wiringPiI2CWriteReg8(file_descriptor_, LED0_ON_H + 4 * channel, on >> 8);
    wiringPiI2CWriteReg8(file_descriptor_, LED0_OFF_L + 4 * channel, off & 0xFF);
    wiringPiI2CWriteReg8(file_descriptor_, LED0_OFF_H + 4 * channel, off >> 8);
}

void PCA9685::set_pwm_ms(int channel, float ms) {
    float period_ms = 1000.0 / frequency_hz_;
    int bits_per_ms = PWM_RESOLUTION / period_ms;
    int bits = ms * bits_per_ms;
    set_pwm(channel, 0, bits);
}

void PCA9685::set_all_pwm(uint16_t on, uint16_t off) {
    wiringPiI2CWriteReg8(file_descriptor_, ALL_LED_ON_L, on & 0xFF);
    wiringPiI2CWriteReg8(file_descriptor_, ALL_LED_ON_H, on >> 8);
    wiringPiI2CWriteReg8(file_descriptor_, ALL_LED_OFF_L, off & 0xFF);
    wiringPiI2CWriteReg8(file_descriptor_, ALL_LED_OFF_H, off >> 8);
}

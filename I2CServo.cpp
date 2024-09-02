#include <Adafruit_PWMServoDriver.h>

#include "I2CServo.h"

static Adafruit_PWMServoDriver pwm;

bool I2CServo::is_attached[MAX_SERVOS] = {false};

bool I2CServo::begin()
{
    pwm.begin();
    pwm.setOscillatorFrequency(25000000);
    pwm.setPWMFreq(REFRESH_FREQ);
}

I2CServo::I2CServo() : pin(INVALID_SERVO), pulse_width(DEFAULT_PULSE_WIDTH)
{
}

uint8_t I2CServo::attach(int pin)
{
    return this->attach(pin, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
}

uint8_t I2CServo::attach(int pin, int min, int max)
{
    if (pin < 0 || pin >= MAX_SERVOS || is_attached[pin]) {
        return INVALID_SERVO;
    }

    is_attached[pin] = true;
    this->pin = pin;
    this->min = min;
    this->max = max;

    pwm.writeMicroseconds(this->pin, this->pulse_width);

    return pin;
}

void I2CServo::detach()
{
    if (this->pin == INVALID_SERVO) {
        return;
    }
    is_attached[this->pin] = false;
    pwm.setPin(this->pin, 0);
}

void I2CServo::write(int value)
{
    if (value < MIN_PULSE_WIDTH) { // treat values less than 544 as angles in degrees (valid values in microseconds are handled as microseconds)
        if (value < 0)
            value = 0;
        if (value > 180)
            value = 180;
        value = map(value, 0, 180, this->min, this->max);
    }
    this->writeMicroseconds(value);
}

void I2CServo::writeMicroseconds(int value)
{
    if (this->pin == INVALID_SERVO || !is_attached[this->pin]) {
        return;
    }

    if (value < this->min) { // ensure pulse width is valid
        value = this->min;
    } else if (value > this->max) {
        value = this->max;
    }
    pwm.writeMicroseconds(this->pin, value);
}

int I2CServo::read() // return the value as degrees
{
    return map(this->readMicroseconds(), this->min, this->max, 0, 180);
}

int I2CServo::readMicroseconds()
{
    if (this->pin == INVALID_SERVO || !is_attached[this->pin]) {
        return 0;
    }
    return this->pulse_width;
}

bool I2CServo::attached()
{
    if (this->pin == INVALID_SERVO) {
        return false;
    }
    return is_attached[this->pin];
}

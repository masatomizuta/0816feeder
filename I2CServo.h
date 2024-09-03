#ifndef _I2C_SERVO_H
#define _I2C_SERVO_H

#include <inttypes.h>

#define MIN_PULSE_WIDTH     544  // the shortest pulse sent to a servo
#define MAX_PULSE_WIDTH     2400 // the longest pulse sent to a servo
#define DEFAULT_PULSE_WIDTH 1500 // default pulse width when servo is attached
#define REFRESH_FREQ        50   // refresh frequency in Hz

#define MAX_SERVOS 16 // the maximum number of servos controlled by one controller

#define INVALID_SERVO 255 // flag indicating an invalid servo index

class I2CServo
{
  public:
    I2CServo();
    uint8_t attach(int pin);                   // attach the given pin to the next free channel, sets pinMode, returns channel number or INVALID_SERVO if failure
    uint8_t attach(int pin, int min, int max); // as above but also sets min and max values for writes.
    void detach();
    void write(int value);             // if value is < 200 it's treated as an angle, otherwise as pulse width in microseconds
    void writeMicroseconds(int value); // Write pulse width in microseconds
    int read();                        // returns current pulse width as an angle between 0 and 180 degrees
    int readMicroseconds();            // returns current pulse width in microseconds for this servo (was read_us() in first release)
    bool attached();                   // return true if this servo is attached, otherwise false

    static bool begin(bool enableExtclk = true);

  private:
    uint8_t pin; // pin number for this servo
    uint16_t pulse_width;
    uint16_t min; // minimum pulse width in us at 0deg
    uint16_t max; // maximum pulse width in us at 180deg

    static bool is_attached[MAX_SERVOS];
};

#endif // _I2C_SERVO_H

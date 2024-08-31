/*
 * capabilities
 */
#define NUMBER_OF_FEEDER 18
#define NO_ENABLE_PIN
#define NO_FEEDBACKLINES
#define NO_ANALOG_IN
#define NO_POWER_OUTPUTS

/*
 * feederPinMap: Map IO-pins to specific feeder. First feeder is at index 0 (N0). Last feeder is NUMBER_OF_FEEDER-1
 */
const static uint8_t feederPinMap[NUMBER_OF_FEEDER] = {
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    10,
    11,
    12,
    13,
    A0,
    A1,
    A2,
    A3,
    A4,
    A5,
};

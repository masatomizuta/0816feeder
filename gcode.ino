#include "Feeder.h"
#include "config.h"
#include "shield.h"

String inputBuffer = ""; // Buffer for incoming G-Code lines

/**
 * Look for character /code/ in the inputBuffer and read the float that immediately follows it.
 * @return the value found.  If nothing is found, /defaultVal/ is returned.
 * @input code the character to look for.
 * @input defaultVal the return value if /code/ is not found.
 **/
float parseParameter(char code, float defaultVal)
{
    int codePosition = inputBuffer.indexOf(code);
    if (codePosition != -1) {
        // code found in buffer

        // find end of number (separated by " " (space))
        int delimiterPosition = inputBuffer.indexOf(" ", codePosition + 1);

        float parsedNumber = inputBuffer.substring(codePosition + 1, delimiterPosition).toFloat();

        return parsedNumber;
    } else {
        return defaultVal;
    }
}

void setupGCodeProc()
{
    inputBuffer.reserve(MAX_BUFFFER_MCODE_LINE);
}

void sendAnswer(uint8_t error, String message)
{
    if (error == 0)
        Serial.print(F("ok "));
    else
        Serial.print(F("error "));

    Serial.println(message);
}

bool validFeederNo(int8_t signedFeederNo, uint8_t feederNoMandatory = 0)
{
    return (signedFeederNo == -1 && feederNoMandatory == 0) || (signedFeederNo >= 0 && signedFeederNo < NUMBER_OF_FEEDER);
}

/**
 * Read the input buffer and find any recognized commands.  One G or M command per line.
 */
void processCommand()
{
    // get the command, default -1 if no command found
    int cmd = parseParameter('M', -1);

#ifdef DEBUG
    Serial.print("command found: M");
    Serial.println(cmd);
#endif

    switch (cmd) {

        /*
        FEEDER-CODES
        */

    case MCODE_SET_FEEDER_ENABLE: {

        int8_t _feederEnabled = parseParameter('S', -1);
        if ((_feederEnabled == 0 || _feederEnabled == 1)) {

            if ((uint8_t)_feederEnabled == 1) {
#ifdef HAS_ENABLE_PIN
                digitalWrite(FEEDER_ENABLE_PIN, HIGH);
#endif
                feederEnabled = ENABLED;

                executeCommandOnAllFeeder(cmdEnable);

                sendAnswer(0, F("Feeder set enabled and operational"));
            } else {
#ifdef HAS_ENABLE_PIN
                digitalWrite(FEEDER_ENABLE_PIN, LOW);
#endif
                feederEnabled = DISABLED;

                executeCommandOnAllFeeder(cmdDisable);

                sendAnswer(0, F("Feeder set disabled"));
            }
        } else if (_feederEnabled == -1) {
            sendAnswer(0, ("current powerState: ") + String(feederEnabled));
        } else {
            sendAnswer(1, F("Invalid parameters"));
        }

        break;
    }

    case MCODE_ADVANCE: {
        // 1st to check: are feeder enabled?
        if (feederEnabled != ENABLED) {
            sendAnswer(1, String(String("Enable feeder first! M") + String(MCODE_SET_FEEDER_ENABLE) + String(" S1")));
            break;
        }

        int8_t signedFeederNo = (int)parseParameter('N', -1);
        int8_t overrideErrorRaw = (int)parseParameter('X', -1);
        bool overrideError = false;
        if (overrideErrorRaw >= 1) {
            overrideError = true;
#ifdef DEBUG
            Serial.println("Argument X1 found, feedbackline/error will be ignored");
#endif
        }

        // check for presence of a mandatory FeederNo
        if (!validFeederNo(signedFeederNo, 1)) {
            sendAnswer(1, F("feederNo missing or invalid"));
            break;
        }

        // determine feedLength
        uint8_t feedLength;
        // get feedLength if given, otherwise go for default configured feed_length
        feedLength = (uint8_t)parseParameter('F', feeders[(uint8_t)signedFeederNo].feederSettings.feed_length);

        if (((feedLength % 2) != 0) || feedLength > 24) {
            // advancing is only possible for multiples of 2mm and 24mm max
            sendAnswer(1, F("Invalid feedLength"));
            break;
        }
#ifdef DEBUG
        Serial.print("Determined feedLength ");
        Serial.print(feedLength);
        Serial.println();
#endif

        // start feeding
        bool triggerFeedOK = feeders[(uint8_t)signedFeederNo].advance(feedLength, overrideError);
        if (!triggerFeedOK) {
            // report error to host at once, tape was not advanced...
            sendAnswer(1, F("feeder not OK (not activated, no tape or tension of cover tape not OK)"));
            break;
        }

        // store last used feederNo for M400 command
        FeederClass::completionAnswerFeederNo = signedFeederNo;

        sendAnswer(0, F(""));

        break;
    }

    case MCODE_RETRACT_POST_PICK: {
        // 1st to check: are feeder enabled?
        if (feederEnabled != ENABLED) {
            sendAnswer(1, String(String("Enable feeder first! M") + String(MCODE_SET_FEEDER_ENABLE) + String(" S1")));
            break;
        }

        int8_t signedFeederNo = (int)parseParameter('N', -1);

        // check for presence of FeederNo
        if (!validFeederNo(signedFeederNo, 1)) {
            sendAnswer(1, F("feederNo missing or invalid"));
            break;
        }

        feeders[(uint8_t)signedFeederNo].gotoPostPickPosition();

        sendAnswer(0, F(""));

        break;
    }

    case MCODE_FEEDER_IS_OK: {
        int8_t signedFeederNo = (int)parseParameter('N', -1);

        // check for presence of FeederNo
        if (!validFeederNo(signedFeederNo, 1)) {
            sendAnswer(1, F("feederNo missing or invalid"));
            break;
        }

        sendAnswer(0, feeders[(uint8_t)signedFeederNo].reportFeederErrorState());

        break;
    }

    case MCODE_SERVO_SET_ANGLE: {
        // 1st to check: are feeder enabled?
        if (feederEnabled != ENABLED) {
            sendAnswer(1, String(String("Enable feeder first! M") + String(MCODE_SET_FEEDER_ENABLE) + String(" S1")));
            break;
        }

        // FeederNo
        int8_t signedFeederNo = (int)parseParameter('N', -1);
        if (!validFeederNo(signedFeederNo, 1)) {
            sendAnswer(1, F("feederNo missing or invalid"));
            break;
        }

        // Angle
        uint8_t angle = (int)parseParameter('A', -1);
        if (angle == -1) {
            angle = (int)parseParameter('B', -1);
        } else if (angle == -1) {
            angle = (int)parseParameter('C', -1);
        }

        if (angle < 0 || angle > 180) {
            sendAnswer(1, F("angle missing or invalid"));
            break;
        }

        // Min & Max pulse width
        int min = (int)parseParameter('V', -1);
        int max = (int)parseParameter('W', -1);

        feeders[(uint8_t)signedFeederNo].gotoAngle(angle, min, max);

        sendAnswer(0, F("angle set"));

        break;
    }

    case MCODE_UPDATE_FEEDER_CONFIG: {
        int8_t signedFeederNo = (int)parseParameter('N', -1);

        if (signedFeederNo == -1) {
            executeCommandOnAllFeeder(cmdOutputCurrentSettings);
            sendAnswer(0, F("All feeders config printed."));
            break;
        }

        // check for presence of FeederNo
        if (!validFeederNo(signedFeederNo, 1)) {
            sendAnswer(1, F("feederNo missing or invalid"));
            break;
        }

        if (inputBuffer.indexOf('A') == -1 &&
            inputBuffer.indexOf('B') == -1 &&
            inputBuffer.indexOf('C') == -1 &&
            inputBuffer.indexOf('F') == -1 &&
            inputBuffer.indexOf('U') == -1 &&
            inputBuffer.indexOf('V') == -1 &&
            inputBuffer.indexOf('W') == -1 &&
            inputBuffer.indexOf('X') == -1) {
            feeders[(uint8_t)signedFeederNo].outputCurrentSettings();
            sendAnswer(0, F("Feeder config printed."));
            break;
        }

        // merge given parameters to old settings
        FeederClass::sFeederSettings oldFeederSettings = feeders[(uint8_t)signedFeederNo].getSettings();
        FeederClass::sFeederSettings updatedFeederSettings;
        updatedFeederSettings.full_advanced_angle = parseParameter('A', oldFeederSettings.full_advanced_angle);
        updatedFeederSettings.half_advanced_angle = parseParameter('B', oldFeederSettings.half_advanced_angle);
        updatedFeederSettings.retract_angle = parseParameter('C', oldFeederSettings.retract_angle);
        updatedFeederSettings.feed_length = parseParameter('F', oldFeederSettings.feed_length);
        updatedFeederSettings.time_to_settle = parseParameter('U', oldFeederSettings.time_to_settle);
        updatedFeederSettings.motor_min_pulsewidth = parseParameter('V', oldFeederSettings.motor_min_pulsewidth);
        updatedFeederSettings.motor_max_pulsewidth = parseParameter('W', oldFeederSettings.motor_max_pulsewidth);
#ifdef HAS_FEEDBACKLINES
        updatedFeederSettings.ignore_feedback = parseParameter('X', oldFeederSettings.ignore_feedback);
#endif

        // set to feeder
        feeders[(uint8_t)signedFeederNo].setSettings(updatedFeederSettings);

        // save to eeprom
        feeders[(uint8_t)signedFeederNo].saveFeederSettings();

        // reattach servo with new settings
        feeders[(uint8_t)signedFeederNo].setup();

        // confirm
        feeders[(uint8_t)signedFeederNo].outputCurrentSettings();
        sendAnswer(0, F("Feeders config updated."));

        break;
    }

    case MCODE_WAIT_ADVANCE: {
        if (feederEnabled != ENABLED) {
            sendAnswer(0, F(""));
            return;
        }

        if (FeederClass::completionAnswerRequested) {
            sendAnswer(1, F("already requested"));
            return;
        }

        int8_t signedFeederNo = (int)parseParameter('N', -1);
        if (!validFeederNo(signedFeederNo, 0)) {
            sendAnswer(1, F("feederNo invalid"));
            return;
        }

        if (signedFeederNo != -1) {
            // Request completion answer for specific feeder if it is not idle
            if (feeders[signedFeederNo].feederState != FeederClass::sFeederState::sIDLE) {
                FeederClass::completionAnswerRequested = true;
                FeederClass::completionAnswerFeederNo = signedFeederNo;
                return;
            }
        } else {
            // Request completion answer for last feeder advanced if it is not idle
            if (feeders[FeederClass::completionAnswerFeederNo].feederState != FeederClass::sFeederState::sIDLE) {
                FeederClass::completionAnswerRequested = true;
                return;
            }
        }

        // No need to wait
        sendAnswer(0, F(""));

        break;
    }

    /*
    CODES to Control ADC
    */
#ifdef HAS_ANALOG_IN
    case MCODE_GET_ADC_RAW: {
        // answer to host
        int8_t channel = parseParameter('A', -1);
        if (channel >= 0 && channel < 8) {

            // send value in first line of answer, so it can be parsed by OpenPnP correctly
            Serial.println(String("value:") + String(adcRawValues[(uint8_t)channel]));

            // common answer
            sendAnswer(0, "value sent");
        } else {
            sendAnswer(1, F("invalid adc channel (0...7)"));
        }

        break;
    }
    case MCODE_GET_ADC_SCALED: {
        // answer to host
        int8_t channel = parseParameter('A', -1);
        if (channel >= 0 && channel < 8) {

            // send value in first line of answer, so it can be parsed by OpenPnP correctly
            Serial.println(String("value:") + String(adcScaledValues[(uint8_t)channel], 4));

            // common answer
            sendAnswer(0, "value sent");
        } else {
            sendAnswer(1, F("invalid adc channel (0...7)"));
        }

        break;
    }
    case MCODE_SET_SCALING: {

        int8_t channel = parseParameter('A', -1);

        // check for valid parameters
        if (channel >= 0 && channel < 8) {
            commonSettings.adc_scaling_values[(uint8_t)channel][0] = parseParameter('S', commonSettings.adc_scaling_values[(uint8_t)channel][0]);
            commonSettings.adc_scaling_values[(uint8_t)channel][1] = parseParameter('O', commonSettings.adc_scaling_values[(uint8_t)channel][1]);

            EEPROM.put(EEPROM_COMMON_SETTINGS_ADDRESS_OFFSET, commonSettings);

            sendAnswer(0, (F("scaling set and stored to eeprom")));
        } else {
            sendAnswer(1, F("invalid adc channel (0...7)"));
        }

        break;
    }
#endif

#ifdef HAS_POWER_OUTPUTS
    case MCODE_SET_POWER_OUTPUT: {
        // answer to host
        int8_t powerPin = parseParameter('D', -1);
        int8_t powerState = parseParameter('S', -1);
        if ((powerPin >= 0 && powerPin < NUMBER_OF_POWER_OUTPUT) && (powerState == 0 || powerState == 1)) {
            digitalWrite(pwrOutputPinMap[(uint8_t)powerPin], (uint8_t)powerState);
            sendAnswer(0, F("Output set"));
        } else {
            sendAnswer(1, F("Invalid Parameters"));
        }

        break;
    }

#endif

    case MCODE_FACTORY_RESET: {
        commonSettings.version[0] = commonSettings.version[0] + 1;

        EEPROM.put(EEPROM_COMMON_SETTINGS_ADDRESS_OFFSET, commonSettings);

        sendAnswer(0, F("EEPROM invalidated, defaults will be loaded on next restart. Please restart now."));

        break;
    }

    default:
        sendAnswer(0, F("unknown or empty command ignored"));

        break;
    }
}

void listenToSerialStream()
{
    while (Serial.available()) {

        // get the received byte, convert to char for adding to buffer
        char receivedChar = (char)Serial.read();

        // print back for debugging
        // #ifdef DEBUG
        Serial.print(receivedChar);
        // #endif

        // add to buffer
        inputBuffer += receivedChar;

        // if the received character is a newline, processCommand
        if (receivedChar == '\n') {

            // remove comments
            inputBuffer.remove(inputBuffer.indexOf(";"));
            inputBuffer.trim();

            processCommand();

            // clear buffer
            inputBuffer = "";
        }
    }
}

/**
 * Wac-A-Mole
*/

#include <hackerpet.h>
const char PlayerName[] = "Luna";

int currentLevel = 1;                          // starting and current level
const int MAX_LEVEL = 2;                       // Maximum number of levels
const int HISTORY_LENGTH = 20;                 // Number of previous interactions to look at for
                                               // performance
const int ENOUGH_SUCCESSES = 18;               // if num successes >= ENOUGH_SUCCESSES level-up
const unsigned long FOODTREAT_DURATION = 4000; // (ms) how long to present foodtreat
const int FLASHING = 0;
const int FLASHING_DUTY_CYCLE = 99;
const unsigned long TIMEOUT_MS = 900; // (ms) how long to wait until restarting
                                      // the interaction
const int NUM_PADS = 1;               // Choose number of lit-up pads at a time (1, 2 or 3)

const unsigned long SOUND_FOODTREAT_DELAY = 1200; // (ms) delay for reward sound
const unsigned long SOUND_TOUCHPAD_DELAY = 300;   // (ms) delay for touchpad sound

bool performance[HISTORY_LENGTH] = {0}; // store the progress in this challenge
int perfPos = 0;                        // to keep our position in the performance array
int perfDepth = 0;                      // to keep the size of the number of perf numbers to
                                        // consider

// Use primary serial over USB interface for logging output (9600)
// Choose logging level here (ERROR, WARN, INFO)
SerialLogHandler logHandler(LOG_LEVEL_INFO, {
                                                // Logging level for all messages
                                                {"app.hackerpet", LOG_LEVEL_ERROR}, // Logging level for library messages
                                                {"app", LOG_LEVEL_INFO}             // Logging level for application messages
                                            });

HubInterface hub;
SYSTEM_THREAD(ENABLED);

/**
 * Helper functions
 * ----------------
 */

/// return the number of successful interactions in performance history for current level
unsigned int countSuccesses()
{
    unsigned int total = 0;
    for (unsigned char i = 0; i <= perfDepth - 1; i++)
        if (performance[i] == 1)
            total++;
    return total;
}

/// return the number of misses in performance history for current level
unsigned int countMisses()
{
    unsigned int total = 0;
    for (unsigned char i = 0; i <= perfDepth - 1; i++)
        if (performance[i] == 0)
            total++;
    return total;
}

/// reset performance history to 0
void resetPerformanceHistory()
{
    for (unsigned char i = 0; i < HISTORY_LENGTH; i++)
        performance[i] = 0;
    perfPos = 0;
    perfDepth = 0;
}

/// add an interaction result to the performance history
void addResultToPerformanceHistory(bool entry)
{
    // Log.info("Adding %u", entry);
    performance[perfPos] = entry;
    perfPos++;
    if (perfDepth < HISTORY_LENGTH)
        perfDepth++;
    if (perfPos > (HISTORY_LENGTH - 1))
    { // make our performance array circular
        perfPos = 0;
    }
    // Log.info("perfPos %u, perfDepth %u", perfPos, perfDepth);
    Log.info("New successes: %u, misses: %u", countSuccesses(),
             countMisses());
}

/// print the performance history for debugging
void printPerformanceArray()
{
    Serial.printf("performance: {");
    for (unsigned char i = 0; i < perfDepth; i++)
    {
        Serial.printf("%u", performance[i]);
        if ((i + 1) == perfPos)
            Serial.printf("|");
    }
    Serial.printf("}\n");
}

/// converts a bitfield of pressed touchpads to letters
/// multiple consecutive touches are possible and will be reported L -> M - > R
/// @returns String
String convertBitfieldToLetter(unsigned char pad)
{
    String letters = "";
    if (pad & hub.BUTTON_LEFT)
        letters += 'L';
    if (pad & hub.BUTTON_MIDDLE)
        letters += 'M';
    if (pad & hub.BUTTON_RIGHT)
        letters += 'R';
    return letters;
}

/// The actual AvoidingUnlitTouchpads challenge. This function needs to be called in a loop.
bool playAvoidingUnlitTouchpads()
{
    yield_begin();

    static unsigned long gameStartTime, timestampBefore, activityDuration = 0;
    static unsigned char target = 0;      // bitfield, holds target touchpad
    static unsigned char retryTarget = 0; // should not be re-initialized
    static int foodfoodtreatState = 99;
    static unsigned char pressed = 0; // bitfield, holds pressed touchpad
    static unsigned long timestampTouchpad = 0;
    static bool foodtreatWasEaten = false; // store if foodtreat was eaten
    static bool accurate = false;
    static bool timeout = false;
    static bool challengeComplete = false; // do not re-initialize
    static int yellow = 60;                // touchpad color, is randomized
    static int blue = 60;                  // touchpad color, is randomized

    // Static variables and constants are only initialized once, and need to be re-initialized
    // on subsequent calls
    gameStartTime = 0;
    timestampBefore = 0;
    activityDuration = 0;
    target = 0; // bitfield, holds target touchpad
    foodfoodtreatState = 99;
    pressed = 0; // bitfield, holds pressed touchpad
    timestampTouchpad = 0;
    foodtreatWasEaten = false; // store if foodtreat was eaten
    accurate = false;
    timeout = false;
    yellow = 60; // touchpad color, is randomized
    blue = 60;   // touchpad color, is randomized

    gameStartTime = Time.now();
    hub.SetButtonAudioEnabled(false);

    // before starting interaction, wait until:
    //  1. device layer is ready (in a good state)
    //  2. foodmachine is "idle", meaning it is not spinning or dispensing
    //      and tray is retracted (see FOODMACHINE_... constants)
    //  3. no touchpad is currently pressed
    yield_wait_for((hub.IsReady() &&
                    hub.FoodmachineState() == hub.FOODMACHINE_IDLE &&
                    !hub.AnyButtonPressed()),
                   false);

    // DI reset occurs if, for example, device  layer detects that touchpads need
    // re-calibration
    hub.SetDIResetLock(true);

    // Record start timestamp for performance logging
    timestampBefore = millis();

    yellow = random(20, 90); // pick a yellow for interaction
    blue = random(20, 90);   // pick a blue for interaction

    if (retryTarget != 0)
    {
        Log.info("We're doing a retry interaction");
        target = retryTarget;
        hub.SetLights(target, yellow, blue, FLASHING, FLASHING_DUTY_CYCLE);
    }
    else
    {
        // choose some target lights, and store which targets were randomly chosen
        target = hub.SetRandomButtonLights(NUM_PADS, yellow, blue, FLASHING,
                                           FLASHING_DUTY_CYCLE);
    }

    // progress to next state
    timestampTouchpad = millis();

    do
    {
        // detect any touchpads currently pressed
        pressed = hub.AnyButtonPressed();
        yield(false); // use yields statements any time the hub is pausing or waiting
    } while (
        (pressed != hub.BUTTON_LEFT // we want it to just be a single touchpad
         && pressed != hub.BUTTON_MIDDLE && pressed != hub.BUTTON_RIGHT) &&
        millis() < timestampTouchpad + TIMEOUT_MS);

    // record time period for performance logging
    activityDuration = millis() - timestampBefore;

    hub.SetLights(hub.LIGHT_BTNS, 0, 0, 0); // turn off lights

    // Check touchpads and accuracy
    if (pressed == 0)
    {
        Log.info("No touchpad pressed, we have a timeout");
        timeout = true;
        accurate = false;
    }
    else
    {
        timeout = false;
        accurate = !((pressed & target) == 0); // will be zero if and only if the
    }                                          // wrong touchpad was touched

    foodtreatWasEaten = false;

    // Check result and consequences
    if (accurate)
    {
        Log.info("Correct touchpad pressed, dispensing foodtreat");
        hub.PlayAudio(hub.AUDIO_POSITIVE, 20);
        do
        {
            foodfoodtreatState =
                hub.PresentAndCheckFoodtreat(FOODTREAT_DURATION); // time pres (ms)
            yield(false);
        } while (foodfoodtreatState != hub.PACT_RESPONSE_FOODTREAT_NOT_TAKEN &&
                 foodfoodtreatState != hub.PACT_RESPONSE_FOODTREAT_TAKEN);

        // Check if foodtreat was eaten
        if (foodfoodtreatState == hub.PACT_RESPONSE_FOODTREAT_TAKEN)
        {
            Log.info("Foodtreat was eaten");
            foodtreatWasEaten = true;
        }
        else
        {
            Log.info("Foodtreat was not eaten");
            foodtreatWasEaten = false;
        }
    }
    else
    {
        if (!timeout) // don't play any audio if there's a timeout (no response -> no
                      // consequence)
        {
            Log.info("Wrong touchpad pressed");
            hub.PlayAudio(hub.AUDIO_NEGATIVE, 10);
            yield_sleep_ms(
                SOUND_FOODTREAT_DELAY,
                false); // give the Hub a moment to finish playing the sound
        }
    }

    // Check if we're ready for next challenge
    if (currentLevel == MAX_LEVEL)
    {
        addResultToPerformanceHistory(accurate);
        if (countSuccesses() >= ENOUGH_SUCCESSES)
        {
            Log.info("At MAX level! %u", currentLevel);
            challengeComplete = true;
            resetPerformanceHistory();
        }
    }
    else
    {
        // Increase level if foodtreat eaten and good performance in this level
        addResultToPerformanceHistory(accurate);
        if (countSuccesses() >= ENOUGH_SUCCESSES)
        {
            if (currentLevel < MAX_LEVEL)
            {
                currentLevel++;
                Log.info("Leveling UP %u", currentLevel);
                resetPerformanceHistory();
            }
        }
    }

    if (!timeout)
    {
        // Send report
        Log.info("Sending report");

        String extra = "{\"targets\":\"";
        extra += convertBitfieldToLetter(target);
        extra += "\",\"pressed\":\"";
        extra += convertBitfieldToLetter(pressed);
        extra += String::format("\",\"retryGame\":\"%c\"", retryTarget ? '1' : '0');
        if (challengeComplete)
        {
            extra += ",\"challengeComplete\":1";
        }
        extra += "}";

        hub.Report(Time.format(gameStartTime,
                               TIME_FORMAT_ISO8601_FULL), // play_start_time
                   PlayerName,                            // player
                   currentLevel,                          // level
                   String(accurate),                      // result
                   activityDuration,                      // duration -> linked to level and includes
                                                          // tray movement
                   accurate,                              // foodtreat_presented
                   foodtreatWasEaten,                     // foodtreatWasEaten
                   extra                                  // extra field
        );
    }

    if (accurate)
    {
        retryTarget = 0;
    }
    else if (!timeout && (currentLevel > 1))
    {
        retryTarget = target;
    }


    // between interaction wait time
    yield_sleep_ms((unsigned long)random(4000, 40000), false);

    hub.SetDIResetLock(false); // allow DI board to reset if needed between interactions
    yield_finish();
    return true;
}


void setup()
{
    hub.Initialize(__FILE__);
}

void loop()
{
    bool gameIsComplete = false;

    // Advance the device layer state machine, but with 20 ms max time
    // spent per loop cycle.
    hub.Run(20);

    // Play 1 interaction of the playAvoidingUnlitTouchpads challenge
    gameIsComplete = playAvoidingUnlitTouchpads(); // Returns true if level is done

    if (gameIsComplete)
    {
        // Interaction end
        return;
    }
}

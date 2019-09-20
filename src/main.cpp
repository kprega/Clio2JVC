#include <SerialCommand.h>
#include <commands_dfs.h>
#include <JvcRadio.h>
#include <Clio.h>
#include <mcp_can.h>

#define DEBUG true

// Pinout for Arduino
const byte csPin = 9;
const byte interruptPin = 2;
const byte displaySwitchPin = 4;
const byte signalPin = 8;

JvcRadio carRadio;
Clio clio(csPin, interruptPin, displaySwitchPin);
SerialCommand commandLine;

// Variables used for car's speed calculation
int pulseDuration;
int speed = 0;
double distance = 105; // milimeters

// Variables used to handle time-based events
unsigned long currentTime;
unsigned long newTime;
unsigned long refreshTime;
unsigned long syncTime0;
unsigned long syncTime1;

// Time constants
const unsigned long timeout = 5000; // miliseconds
const unsigned long syncRate = 750; // miliseconds
const unsigned long refreshRate = 2 * syncRate;

// Variables used for automatic volume adjustment
int addedVolume = 0;
const int addedVolumeLimit = 8;     // volume can be increased up to 8 points
const int activationThreshold = 50; // km/h
const int changeThreshold = 10;     // km/h

// Handler for command that isn't matched
void Unrecognized(const char *command)
{
    Serial.println("Command not recognized.");
}

// Sends to JVC unit command read from serial
void SendToCarRadio()
{
    int aNumber;
    char *arg;

    arg = commandLine.next();
    if (arg != NULL)
    {
        aNumber = atoi(arg); // Converts a char string to an integer
        InterpreteRadioCommand(aNumber);
    }
    else
    {
        Serial.println("No arguments");
    }
}

// This method interpretes command to be sent to car radio unit
// Returns true if command has been successfully interpreted (i.e. commandIndex was in range from 1 to 14), false otherwise
bool InterpreteRadioCommand(int commandIndex)
{
    bool result = true;
    if (DEBUG)
    {
        switch (commandIndex)
        {
        case 1: // 1: mute button - short - mute/unmute speakers
            Serial.println("Sending MUTE");
            break;
        case 2: // 2: mute button - long - mute/unmute speakers
            Serial.println("Sending MUTE");
            break;
        case 3: // 3: volume up - short - increases volume by 1
            Serial.println("Sending VOL_UP");
            break;
        case 4: // 4: volume up - long - increases volume by 2
            Serial.println("Sending VOL_UP twice");
            break;
        case 5: // 5: volume down - short - decreases volume by 1
            Serial.println("Sending VOL_DOWN");
            break;
        case 6: // 6: volume down - long - decreases volume by 2
            Serial.println("Sending VOL_DOWN twice");
            break;
        case 7: // 7: source button - short - toggle input source
            Serial.println("Sending SOURCE");
            break;
        case 8: // 8: source button - long - activate voice control
            Serial.println("Sending VOICE_CONTROL");
            break;
        case 9: // 9: track left - short - track backward
            Serial.println("Sending TRACK_BACK");
            break;
        case 10: // 10: track left - long - toggle equalizer
            Serial.println("Sending EQUALIZER");
            break;
        case 11: // 11: track right - short - track forward
            Serial.println("Sending TRACK_FORW");
            break;
        case 12: // 12: track right - long - unused
            Serial.println("Index not in use");
            break;
        case 13: // 13: roll down - folder backward
            Serial.println("Sending FOLDER_FORW");
            break;
        case 14: // 14: roll up - folder forward
            Serial.println("Sending FOLDER_BACK");
            break;
        }
    }

    switch (commandIndex)
    {
    case 1: // 1: mute button - short - mute/unmute speakers
        carRadio.Action(MUTE);
        clio.PrintDisplay("MUTE");
        break;
    case 2: // 2: mute button - long - mute/unmute speakers
        carRadio.Action(MUTE);
        clio.PrintDisplay("MUTE");
        break;
    case 3: // 3: volume up - short - increases volume by 1
        carRadio.Action(VOL_UP);
        clio.PrintDisplay("VOL+");
        break;
    case 4: // 4: volume up - long - increases volume by 2
        carRadio.Action(VOL_UP);
        carRadio.Action(VOL_UP);
        clio.PrintDisplay("VOL++");
        break;
    case 5: // 5: volume down - short - decreases volume by 1
        carRadio.Action(VOL_DOWN);
        clio.PrintDisplay("VOL-");
        break;
    case 6: // 6: volume down - long - decreases volume by 2
        carRadio.Action(VOL_DOWN);
        carRadio.Action(VOL_DOWN);
        clio.PrintDisplay("VOL--");
        break;
    case 7: // 7: source button - short - toggle input source
        carRadio.Action(SOURCE);
        clio.PrintDisplay("SOURCE");
        break;
    case 8: // 8: source button - long - activate voice control
        carRadio.Action(VOICE_CONTROL);
        clio.PrintDisplay("VOICE");
        break;
    case 9: // 9: track left - short - track backward
        carRadio.Action(TRACK_BACK);
        clio.PrintDisplay("TR BACK");
        break;
    case 10: // 10: track left - long - toggle equalizer
        carRadio.Action(EQUALIZER);
        clio.PrintDisplay("EQ");
        break;
    case 11: // 11: track right - short - track forward
        carRadio.Action(TRACK_FORW);
        clio.PrintDisplay("TR FORW");
        break;
    case 12: // 12: track right - long - unused
        clio.PrintDisplay("NONE");
        break;
    case 13: // 13: roll down - folder backward
        carRadio.Action(FOLDER_FORW);
        clio.PrintDisplay("FOL FORW");
        break;
    case 14: // 14: roll up - folder forward
        carRadio.Action(FOLDER_BACK);
        clio.PrintDisplay("FOL BACK");
        break;
    default:
        result = false;
        break;
    }
    return result;
}

void SetupCommandLine()
{
    if (DEBUG)
    {
        // radio [commandIndex] - sends command with specified index to radio device
        commandLine.addCommand("radio", SendToCarRadio);
        // display [text] - writes text to Clio's display
        commandLine.addCommand("display", PrintToDisplay);
        // speed - turns on speed signal measurement for 10 seconds
        commandLine.addCommand("speed", SpeedSignalAnalysis);
        // distance [value] - sets distance for speed signal measurement analysis
        commandLine.addCommand("distance", SetDistance);
        // disp_on - turns Clio's display on
        commandLine.addCommand("disp_on", DisplayOn);
        // disp_on - turns Clio's display off
        commandLine.addCommand("disp_off", DisplayOff);
        // Handler for command that isn't matched
        commandLine.setDefaultHandler(Unrecognized);
    }
}

// Turns Clio's display on
void DisplayOn()
{
    clio.DisplayOn();
}

// Turns Clio's display off
void DisplayOff()
{
    clio.DisplayOff();
}

// Writes text to Clio's display
void PrintToDisplay()
{
    char *arg;
    arg = commandLine.next();
    String str(arg);
    // str += arg;
    clio.PrintDisplay(str);
}

// Sets distance for speed signal measurement analysis
void SetDistance()
{
    char *arg;
    arg = commandLine.next();

    if (arg != NULL)
    {
        distance = atoi(arg);
        Serial.print("Distance set to ");
        Serial.println(distance);
    }
}

// turns on speed signal measurement for 10 seconds
void SpeedSignalAnalysis()
{
    currentTime = millis();
    do
    {
        pulseDuration = pulseIn(signalPin, LOW);
        delay(500);
        speed = (distance * 3600) / pulseDuration;
        Serial.print("Pulse duration: ");
        Serial.print(pulseDuration);
        Serial.print("   Estimated speed: ");
        Serial.print(speed);
        Serial.println("km/h");
        newTime = millis();
    } while (newTime - currentTime < 10000UL);
}

// Reads car speed from the sensor
int GetSpeed()
{
    pulseDuration = pulseIn(signalPin, LOW);
    speed = (distance /*mm*/ * 3600) / pulseDuration /*μs*/; // result in km/h 
    return round(speed) < 0 ? 0 : round(speed);
}

void AdjustVolume()
{
    if (speed > activationThreshold)
    {
        // calculate expected value of added volume
        int expectedValue = floor((speed - activationThreshold) / changeThreshold);
        // adjust volume if added volume level differs from expected
        if (addedVolume < expectedValue && addedVolume < addedVolumeLimit)
        {
            carRadio.Action(VOL_UP);
            addedVolume++;

            if (DEBUG)
            {
                Serial.println("Auto volume up");
            }
        }
        if (addedVolume > expectedValue)
        {
            carRadio.Action(VOL_DOWN);
            addedVolume--;

            if (DEBUG)
            {
                Serial.println("Auto volume down");
            }
        }
    }
}

void setup()
{
    // Await serial monitor open
    if (DEBUG)
    {
        while (!Serial)
            ;
        Serial.begin(9600);
    }

    // Prepare to read speed signal info on pin 8
    pinMode(signalPin, INPUT_PULLUP);

    // Set up car radio to work with pin 4
    carRadio.SetupRemote(4);

    // Initialize display
    clio.DisplayOn();

    // Prepare command line
    SetupCommandLine();

    // Store time
    syncTime0 = millis();
    currentTime = syncTime0;
    refreshTime = syncTime0;

    if (DEBUG)
    {
        Serial.println("Ready");
    }
}

void loop()
{
    if (DEBUG)
    {
        commandLine.readSerial(); // We don't do much, just process serial commands
    }

    // Check if a command has been sent from remote or serial
    if (InterpreteRadioCommand(clio.ReceiveFromRemote()))
    {
        // If a command has been sent we'll wait few seconds before switching display back to speed mode
        // Store current time to calculate message timeout below
        currentTime = millis();
    }

    // Synchronize display every 750 miliseconds
    syncTime1 = millis();
    if (syncTime1 - syncTime0 > syncRate)
    {
        // Synchronize with display
        clio.Sync();
        // Reset time zero
        syncTime0 = millis();
    }

    // Check if timeout from remote and refresh time rate have passed, if yes, display speed
    newTime = millis();
    speed = GetSpeed();
    Serial.println(speed);
    if (newTime - currentTime > timeout && newTime - refreshTime > refreshRate)
    {
        // Prepare string
        String spd("SPD ");
        spd += speed;
        // Display message
        clio.PrintDisplay(spd);
        // Store refresh time
        refreshTime = millis();
    }

    // Automatically adjust volume level basing on speed
    AdjustVolume();

    // Display CAN packets in serial monitor
    if (DEBUG)
    {
        clio.ShowPacketData();
    }
}
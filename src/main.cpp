#include <SerialCommand.h>
#include <commands_dfs.h>
#include <JvcRadio.h>
#include <Clio.h>
#include <mcp_can.h>

byte csPin = 9;
byte interruptPin = 2;
byte displaySwitchPin = 4;

JvcRadio carRadio;
Clio clio(csPin, interruptPin, displaySwitchPin);
SerialCommand commandLine;

int pulseDuration;
int signalPin = 8;
double speed = 0;
double distance = 105; // milimeters
unsigned long currentTime;
unsigned long newTime;
unsigned long refreshTime;
unsigned long syncTime0;
unsigned long syncTime1;
unsigned long timeout = 5000;             // miliseconds
unsigned long syncRate = 750;             // miliseconds
unsigned long refreshRate = 2 * syncRate; // miliseconds
int addedVolume = 0;

// Prints out list of available commands
void ShowHelp()
{
    // List of available commands:
    // radio [commandIndex] - sends command with specified index to radio device
    // display [text] - writes text to Clio's display
    // speed - turns on speed signal measurement for 10 seconds
    // distance [value] - sets distance for speed signal measurement analysis
}

// Handler for command that isn't matched
void Unrecognized(const char *command)
{
    Serial.println("Command not recognized.");
}

void ShowRadioCommands()
{
    // List of available radio commands:
    // 1: mute button - short - mute/unmute speakers
    // 2: mute button - long - mute/unmute speakers
    // 3: volume up - short - increases volume by 1
    // 4: volume up - long - increases volume by 2
    // 5: volume down - short - decreases volume by 1
    // 6: volume down - long - decreases volume by 2
    // 7: source button - short - toggle input source
    // 8: source button - long - activate voice control
    // 9: track left - short - track backward
    // 10: track left - long - toggle equalizer
    // 11: track right - short - track forward
    // 12: track right - long - unused
    // 13: roll down - folder backward
    // 14: roll up - folder forward
}

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
//
bool InterpreteRadioCommand(int commandIndex)
{
    bool result = true;
    switch (commandIndex)
    {
    case 1:
        Serial.println("Sending MUTE");
        carRadio.Action(MUTE);
        clio.PrintDisplay("MUTE");
        break;
    case 2:
        Serial.println("Sending MUTE");
        carRadio.Action(MUTE);
        clio.PrintDisplay("MUTE");
        break;
    case 3:
        Serial.println("Sending VOL_UP");
        carRadio.Action(VOL_UP);
        clio.PrintDisplay("VOL+");
        break;
    case 4:
        Serial.println("Sending VOL_UP twice");
        carRadio.Action(VOL_UP);
        carRadio.Action(VOL_UP);
        clio.PrintDisplay("VOL++");
        break;
    case 5:
        Serial.println("Sending VOL_DOWN");
        carRadio.Action(VOL_DOWN);
        clio.PrintDisplay("VOL-");
        break;
    case 6:
        Serial.println("Sending VOL_DOWN twice");
        carRadio.Action(VOL_DOWN);
        carRadio.Action(VOL_DOWN);
        clio.PrintDisplay("VOL--");
        break;
    case 7:
        Serial.println("Sending SOURCE");
        carRadio.Action(SOURCE);
        clio.PrintDisplay("SOURCE");
        break;
    case 8:
        Serial.println("Sending VOICE_CONTROL");
        carRadio.Action(VOICE_CONTROL);
        clio.PrintDisplay("VOICE");
        break;
    case 9:
        Serial.println("Sending TRACK_BACK");
        carRadio.Action(TRACK_BACK);
        clio.PrintDisplay("TR BACK");
        break;
    case 10:
        Serial.println("Sending EQUALIZER");
        carRadio.Action(EQUALIZER);
        clio.PrintDisplay("EQ");
        break;
    case 11:
        Serial.println("Sending TRACK_FORW");
        carRadio.Action(TRACK_FORW);
        clio.PrintDisplay("TR FORW");
        break;
    case 12:
        Serial.println("Index not in use");
        clio.PrintDisplay("NONE");
        break;
    case 13:
        Serial.println("Sending FOLDER_FORW");
        carRadio.Action(FOLDER_FORW);
        clio.PrintDisplay("FOL FORW");
        break;
    case 14:
        Serial.println("Sending FOLDER_BACK");
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
    commandLine.addCommand("radio", SendToCarRadio);
    commandLine.addCommand("display", PrintToDisplay);
    commandLine.addCommand("speed", SpeedSignalAnalysis);
    commandLine.addCommand("distance", SetDistance);
    commandLine.addCommand("disp_on", DisplayOn);
    commandLine.addCommand("disp_off", DisplayOff);
    commandLine.setDefaultHandler(Unrecognized);
}

void DisplayOn()
{
    clio.DisplayOn();
}

void DisplayOff()
{
    clio.DisplayOff();
}

void PrintToDisplay()
{
    char *arg;
    arg = commandLine.next();
    String str(arg);
    // str += arg;
    clio.PrintDisplay(str);
}

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

double GetSpeed()
{
    pulseDuration = pulseIn(signalPin, LOW);
    return (distance /*mm*/ * 3600) / pulseDuration /*μs*/; // result in km/h
}

void AdjustVolume()
{
    //acceleration from below level
    if (speed > addedVolume * 10 + 50 && addedVolume <= 7)
    {
        carRadio.Action(VOL_UP);
        addedVolume++;
    }

    //deceleration from above level
    if (speed < (addedVolume - 1) * 10 + 50 && addedVolume >= 1)
    {
        carRadio.Action(VOL_DOWN);
        addedVolume--;
    }
}

void setup()
{
    // Await serial monitor open
    while (!Serial)
        ;
    Serial.begin(9600);

    // Prepare to read speed signal info on pin 8
    pinMode(signalPin, INPUT_PULLUP);

    // Set up car radio to work with pin 4
    carRadio.SetupRemote(4);

    // Initialize display
    clio.SetupDisplay();

    // Prepare command line
    SetupCommandLine();

    // Store time
    syncTime0 = millis();
    currentTime = syncTime0;
    refreshTime = syncTime0;

    Serial.println("Ready");
}

void loop()
{
    commandLine.readSerial(); // We don't do much, just process serial commands

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
    if (newTime - currentTime > timeout && newTime - refreshTime > refreshRate)
    {
        // Prepare string
        String speed("SPD ");
        speed += GetSpeed();
        // Display message
        clio.PrintDisplay(speed);
        // Store refresh time
        refreshTime = millis();
    }

    // Automatically adjust volume level basing on speed
    AdjustVolume();
}
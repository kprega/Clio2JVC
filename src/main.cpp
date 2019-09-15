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
double distance = 105;
unsigned long currentTime;
unsigned long newTime;
unsigned long timeZero;
unsigned long timeOne;

// Prints out list of available commands
void ShowHelp()
{
    Serial.println("List of available commands:");
    Serial.println("help - displays this message");
    Serial.println("radio [commandIndex] - sends command with specified index to radio device");
    Serial.println("radio ? - displays available radio commands");
    Serial.println("display [text] - writes text to Clio's display");
    Serial.println("speed - turns on speed signal measurement for 10 seconds");
    Serial.println("distance [value] - sets distance for speed signal measurement analysis");
}

// Handler for command that isn't matched
void Unrecognized(const char *command)
{
    Serial.println("Command not recognized. Type 'help' to display available commands.");
}

void ShowRadioCommands()
{
    Serial.println("List of available radio commands:");
    Serial.println("1: increase volume level");
    Serial.println("2: decrease volume level");
    Serial.println("3: change input source");
    Serial.println("4: change equalizer preset(?)");
    Serial.println("5: mute/unmute speakers");
    Serial.println("6: track forward");
    Serial.println("7: track backward");
    Serial.println("8: switch to next folder");
    Serial.println("9: switch to previous folder");
    Serial.println("10: enables voice control");
}

void SendToCarRadio()
{
    int aNumber;
    char *arg;

    arg = commandLine.next();
    if (arg != NULL)
    {
        aNumber = atoi(arg); // Converts a char string to an integer

        switch (aNumber)
        {
        case 1:
            Serial.println("Sending VOL_UP");
            carRadio.Action(VOL_UP);
            break;
        case 2:
            Serial.println("Sending VOL_DOWN");
            carRadio.Action(VOL_DOWN);
            break;
        case 3:
            Serial.println("Sending SOURCE");
            carRadio.Action(SOURCE);
            break;
        case 4:
            Serial.println("Sending EQUALIZER");
            carRadio.Action(EQUALIZER);
            break;
        case 5:
            Serial.println("Sending MUTE");
            carRadio.Action(MUTE);
            break;
        case 6:
            Serial.println("Sending TRACK_FORW");
            carRadio.Action(TRACK_FORW);
            break;
        case 7:
            Serial.println("Sending TRACK_BACK");
            carRadio.Action(TRACK_BACK);
            break;
        case 8:
            Serial.println("Sending FOLDER_FORW");
            carRadio.Action(FOLDER_FORW);
            break;
        case 9:
            Serial.println("Sending FOLDER_BACK");
            carRadio.Action(FOLDER_BACK);
            break;
        case 10:
            Serial.println("Sending VOICE_CONTROL");
            carRadio.Action(VOICE_CONTROL);
            break;
        default:
            Serial.println("Given command index not supported.");
            ShowRadioCommands();
            break;
        }
    }
    else
    {
        Serial.println("No arguments");
    }
}

void SetupCommandLine()
{
    commandLine.addCommand("help", ShowHelp);
    commandLine.addCommand("radio", SendToCarRadio);
    commandLine.addCommand("display", PrintToDisplay);
    commandLine.addCommand("speed", SpeedSignalAnalysis);
    commandLine.addCommand("distance", SetDistance);
    commandLine.setDefaultHandler(Unrecognized);
}

void PrintToDisplay()
{
    char *arg;
    arg = commandLine.next();
    clio.PrintDisplay(*arg);
    clio.DisplayString(arg);
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
    while (newTime - currentTime < 10000UL)
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
    timeZero = millis();

    Serial.println("Ready");
}

void loop()
{
    commandLine.readSerial(); // We don't do much, just process serial commands
    timeOne = millis();
    if (timeOne - timeZero > 750UL )
    {
        // Synchronize with display
        clio.Sync();
        // Reset time zero
        timeZero = millis();
    }
}
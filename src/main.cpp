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
    Serial.println("1: mute button - short - mute/unmute speakers");
    Serial.println("2: mute button - long - mute/unmute speakers");
    Serial.println("3: volume up - short - increases volume by 1");
    Serial.println("4: volume up - long - increases volume by 2");
    Serial.println("5: volume down - short - decreases volume by 1");
    Serial.println("6: volume down - long - decreases volume by 2");
    Serial.println("7: source button - short - toggle input source");
    Serial.println("8: source button - long - activate voice control");
    Serial.println("9: track left - short - track backward");
    Serial.println("10: track left - long - toggle equalizer");
    Serial.println("11: track right - short - track forward");
    Serial.println("12: track right - long - unused");
    Serial.println("13: roll down - folder backward");
    Serial.println("14: roll up - folder forward");
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

void InterpreteRadioCommand(int commandIndex)
{
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
        //Serial.println("Sending EQUALIZER");
        //carRadio.Action(EQUALIZER);
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
        Serial.println("Given command index not supported.");
        clio.PrintDisplay("ERROR");
        ShowRadioCommands();
        break;
    }
}

void SetupCommandLine()
{
    commandLine.addCommand("help", ShowHelp);
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
    String str;
    str += arg;
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
        String spd;
        spd += "SPD ";
        spd += speed;
        clio.PrintDisplay(spd);
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

    clio.PrintDisplay("WELCOME");
    Serial.println("Ready");
}

void loop()
{
    commandLine.readSerial(); // We don't do much, just process serial commands
    InterpreteRadioCommand(clio.ReceiveFromRemote());

    timeOne = millis();
    if (timeOne - timeZero > 750UL)
    {
        // Synchronize with display
        clio.Sync();
        // Reset time zero
        timeZero = millis();
    }
}
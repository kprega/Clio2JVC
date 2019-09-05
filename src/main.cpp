#include <SerialCommand.h>
#include <commands_dfs.h>
#include <JVC.h>

JVC carRadio;
SerialCommand command;

// Prints out list of available commands
void ShowHelp()
{
    Serial.println("List of available commands:");
    Serial.println("help - displays this message");
    Serial.println("radio [commandIndex] - sends command with specified index to radio device");
    Serial.println("radio ? - displays available radio commands");
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
    Serial.println("11: answer BT call(?)");
    Serial.println("12: turn on/off(?)");
}

void SendToCarRadio()
{
    int aNumber;
    char *arg;

    arg = command.next();
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
            Serial.println("Sending VOL_DONW");
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
    command.addCommand("help", ShowHelp);
    command.addCommand("radio", SendToCarRadio);
    command.setDefaultHandler(Unrecognized);
}

void setup()
{
    // Await serial monitor open
    while (!Serial)
        ;
    Serial.begin(9600);

    // Set up car radio to work with pin 4
    carRadio.SetupRemote(4);
    SetupCommandLine();

    Serial.println("Ready");
}

void loop()
{
    command.readSerial(); // We don't do much, just process serial commands
}
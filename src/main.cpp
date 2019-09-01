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
    Serial.println("  4: increase volume level");
    Serial.println("  5: decrease volume level");
    Serial.println("  8: change input source");
    Serial.println(" 13: change equalizer preset(?)");
    Serial.println(" 14: mute/unmute speakers");
    Serial.println(" 18: track forward");
    Serial.println(" 19: track backward");
    Serial.println(" 20: switch to next folder");
    Serial.println(" 21: switch to previous folder");
    Serial.println(" 26: enables voice control");
    Serial.println(" 27: answer BT call(?)");
    Serial.println("128: turn on/off(?)");
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
        case 4:
        case 5:
        case 8:
        case 13:
        case 14:
        case 18:
        case 19:
        case 20:
        case 21:
        case 26:
        case 27:
        case 128:
            carRadio.SendCommand(aNumber);
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
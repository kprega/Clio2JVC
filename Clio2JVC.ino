#include "SPI.h"
#include "mcp_can.h" // https://github.com/coryjfowler/MCP_CAN_lib
#include "mcp_can_dfs.h"
#include "SimpleTimer.h" // https://github.com/schinken/SimpleTimer
#include "JvcRadio.h"
#include "EEPROM.h"
#include <commands_dfs.h>
#include <messages_dfs.h>

// Objects
SimpleTimer timer;
JvcRadio carRadio;

// Variables for remote message fade-out
unsigned long timeKeyPressed = 0;
unsigned long refreshTime = 0;
unsigned long timeout = 2000;     // ms
unsigned long refreshRate = 1000; // ms

// Variables used for automatic volume adjustment
int addedVolume = 0;
bool isMuted = false;

// Variables used for speed calculation
const double distance = 104.5; // mm
double pulseDuration;
double velocity;

// Variables used for voltage calculation
float r1 = 100000.0; // 100k Ohms
float r2 = 10000.0;  // 10k Ohms
float volIn = 0.0;
float volOut = 0.0;
int pinReading = 0;

// Variables for trip distance calculation
double dist = 0;

// Pinout for Arduino
const int radioPin = 5;
const int signalPin = 8;
const int interruptPin = 3;
const int csPin = 9;
const int voltagePin = A3;

// Display modes
enum DisplayModeEnum {SpeedMode, VoltageMode, DistanceMode};
DisplayModeEnum displayMode = SpeedMode;

// Variables for CAN communication
MCP_CAN canBus(csPin); // Set CS pin
unsigned char canMsgLength = 0;
unsigned char canReceivedMsg[8];
long unsigned int canFrameID;
byte canSendResult;

// Settings
byte autoVolume = 1;
byte initSpeed = 50;
byte speedStep = 10;
byte defaultDisplayMode = 0;
byte displayedSettingIndex = 0;
byte settings[4] = {autoVolume, initSpeed, speedStep, defaultDisplayMode};
String labels[4] = {"AVLM", "INIT", "STEP", "DISP"};
bool inMenu = false;

// Messages
const unsigned char RemoteMessages[14][8] =
    {
        {0x03, 0x89, 0x00, 0x03, 0xA2, 0xA2, 0xA2, 0xA2}, // Volume up
        {0x03, 0x89, 0x00, 0x43, 0xA2, 0xA2, 0xA2, 0xA2}, // Volume up long
        {0x03, 0x89, 0x00, 0x04, 0xA2, 0xA2, 0xA2, 0xA2}, // Volume down
        {0x03, 0x89, 0x00, 0x44, 0xA2, 0xA2, 0xA2, 0xA2}, // Volume down long
        {0x03, 0x89, 0x00, 0x05, 0xA2, 0xA2, 0xA2, 0xA2}, // Pause
        {0x03, 0x89, 0x00, 0x85, 0xA2, 0xA2, 0xA2, 0xA2}, // Pause long
        {0x03, 0x89, 0x00, 0x01, 0xA2, 0xA2, 0xA2, 0xA2}, // Source right
        {0x03, 0x89, 0x00, 0x81, 0xA2, 0xA2, 0xA2, 0xA2}, // Source right long
        {0x03, 0x89, 0x00, 0x02, 0xA2, 0xA2, 0xA2, 0xA2}, // Source left
        {0x03, 0x89, 0x00, 0x82, 0xA2, 0xA2, 0xA2, 0xA2}, // Source left long
        {0x03, 0x89, 0x00, 0x00, 0xA2, 0xA2, 0xA2, 0xA2}, // Select
        {0x03, 0x89, 0x00, 0x80, 0xA2, 0xA2, 0xA2, 0xA2}, // Select long
        {0x03, 0x89, 0x01, 0x01, 0xA2, 0xA2, 0xA2, 0xA2}, // Roll up
        {0x03, 0x89, 0x01, 0x41, 0xA2, 0xA2, 0xA2, 0xA2}  // Roll down
};

const unsigned char keepAlive[8]     = {0x79, 0x00, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81};
const unsigned char keepAliveAck[8]  = {0x69, 0x00, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2};
const unsigned char pongMsg[8]       = {0x74, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81};
const unsigned char displayMsgOk[8]  = {0x30, 0x01, 0x00, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2};
const unsigned char startSyncMsg[8]  = {0x7A, 0x01, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81};
const unsigned char syncDispMsg[8]   = {0x70, 0x1A, 0x11, 0x00, 0x00, 0x00, 0x00, 0x01};
const unsigned char initDispMsg[8]   = {0x70, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81};
const unsigned char enableDispMsg[8] = {0x04, 0x52, 0x02, 0xFF, 0xFF, 0x81, 0x81, 0x81};

void setup()
{
    // Wait for 2s to allow car power up all systems
    delay(2000);

    ReadSettings();
    displayMode = (DisplayModeEnum)settings[3];
    
    // CAN 11 bits 500kbauds
    canBus.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ);
    //    if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) // Baud rates defined in mcp_can_dfs.h
    //        Serial.println("CAN Init OK.");
    //    else
    //        Serial.println("CAN Init Failed.");
    canBus.setMode(MCP_NORMAL);
    pinMode(interruptPin, INPUT); // start interrupt
    
    // Set up car radio to work with pin 5
    carRadio.SetupRemote(radioPin);

    // Read speed on pin 8
    pinMode(signalPin, INPUT_PULLUP);

    // Read voltage on pin A3
    pinMode(voltagePin, INPUT);

    // Initialize display
    canBus.sendMsgBuf(DISPLAY_SYNC_FRAME_ID, 0, 8, (byte*)startSyncMsg);
    delay(1);
    canBus.sendMsgBuf(DISPLAY_SYNC_FRAME_ID, 0, 8, (byte*)keepAlive);
    delay(1);
    canBus.sendMsgBuf(DISPLAY_SYNC_FRAME_ID, 0, 8, (byte*)syncDispMsg);
    delay(10);
    canBus.sendMsgBuf(PONG_MSG_FRAME_ID, 0, 8, (byte*)pongMsg);
    canBus.sendMsgBuf(REMOTE_OUT_MSG_FRAME_ID, 0, 8, (byte*)pongMsg);
    canBus.sendMsgBuf(DISPLAY_CONTENT_FRAME_ID, 0, 8, (byte*)initDispMsg);
    delay(1);
    canBus.sendMsgBuf(DISPLAY_ENABLE_FRAME_ID, 0, 8, (byte*)initDispMsg);
    delay(1);
    canBus.sendMsgBuf(DISPLAY_ENABLE_FRAME_ID, 0, 8, (byte*)enableDispMsg);
    delay(50 + 10);
    timer.setInterval(700, CLIO_CAN_syncOK);
}

void loop()
{
    // receive CAN
    if (!digitalRead(interruptPin))
    {
        canBus.readMsgBuf(&canFrameID, &canMsgLength, canReceivedMsg); // read data,  canMsgLength: data length, buf: data buf
        if (canFrameID == KEEPALIVE_FRAME_ID && memcmp(canReceivedMsg, keepAliveAck, 8) == 0)
        {
            // Keep alive reception confirmation received
        }
        else if (canFrameID == DISPLAY_RECEPTION_FRAME_ID)
        {
            // Display confirms reception of new content
        }
        else if (canFrameID == PING_MSG_FRAME_ID)
        {
            // Ping - pong
            canBus.sendMsgBuf(PONG_MSG_FRAME_ID, 0, 8, (byte*)pongMsg);
        }
        // Receiving from the steering wheel remote
        else if (canFrameID == REMOTE_IN_MSG_FRAME_ID)
        {
            canSendResult = canBus.sendMsgBuf(REMOTE_OUT_MSG_FRAME_ID, 0, 8, (byte*)pongMsg);

            // store key pressed time
            timeKeyPressed = millis();

            // Determine key pressed and mode - iterate over collection of messages
            int i = 0;
            int messagesCount = 14;
            while (i < messagesCount && memcmp(canReceivedMsg, RemoteMessages[i], 8) != 0)
            {
                i++;
            }
            ExecuteRemoteInput(i);
        }
    }

    timer.run();

    if (millis() - timeKeyPressed > timeout && millis() - refreshTime > refreshRate)
    {
        CalculateVelocity();
        CalculateVoltage();
        CalculateDistance();

        if (!inMenu)
        {
            // Handle display mode here
            switch (displayMode)
            {
            case SpeedMode:
                DisplaySpeed();
                break;
            case VoltageMode:
                DisplayVoltage();
                break;
            case DistanceMode:
                DisplayDistance();
                break;
            default:
                break;
            }
        }
        refreshTime = millis();

        if (!isMuted && settings[0] == 1)
        {
            AdjustVolume();
        }
    }
}

void ExecuteRemoteInput(int input)
{
    if (inMenu)
    {
        switch (input)
        {
        case 0:
        case 1:
            //Serial.println("Volume up pressed");
            ChangeSettingValue(1);
            break;
        case 2:
        case 3:
            //Serial.println("Volume down pressed");
            ChangeSettingValue(-1);
            break;
        case 4:
        case 5:
            break;
        case 6:
        case 7:
            ToggleSetting(1);
            break;
        case 8:
        case 9:
            ToggleSetting(-1);
            break;
        case 11:
            //Serial.println("Select long pressed");
            UpdateSettings();
            PrintDisplay("SAVED");
            inMenu = !inMenu;
            delay(refreshRate);
            break;
        case 10:
        case 12:
        case 13:
        default:
            break;
        }
    }
    else
    {
        switch (input)
        {
        case 0:
            //Serial.println("Volume up pressed");
            PrintDisplay("VOL+");
            carRadio.Action(VOL_UP);
            break;
        case 1:
            //Serial.println("Volume up long pressed");
            PrintDisplay("VOL++");
            carRadio.Action(VOL_UP);
            carRadio.Action(VOL_UP);
            break;
        case 2:
            //Serial.println("Volume down pressed");
            PrintDisplay("VOL-");
            carRadio.Action(VOL_DOWN);
            break;
        case 3:
            //Serial.println("Volume down long pressed");
            PrintDisplay("VOL--");
            carRadio.Action(VOL_DOWN);
            carRadio.Action(VOL_DOWN);
            break;
        case 4:
            //Serial.println("Pause pressed");
            PrintDisplay("MUTE");
            isMuted = !isMuted;
            carRadio.Action(MUTE);
            break;
        case 5:
            //Serial.println("Pause long pressed");
            PrintDisplay("VOICE");
            carRadio.Action(VOICE_CONTROL);
            break;
        case 6:
            //Serial.println("Source right pressed");
            PrintDisplay("TR FORW");
            carRadio.Action(TRACK_FORW);
            break;
        case 7:
            //Serial.println("Source right long pressed");
            PrintDisplay("FOL FORW");
            carRadio.Action(FOLDER_FORW);
            break;
        case 8:
            //Serial.println("Source left pressed");
            PrintDisplay("TR BACK");
            carRadio.Action(TRACK_BACK);
            break;
        case 9:
            //Serial.println("Source left long pressed");
            PrintDisplay("FOL BACK");
            carRadio.Action(FOLDER_BACK);
            break;
        case 10:
            //Serial.println("Select pressed");
            PrintDisplay("SOURCE");
            carRadio.Action(SOURCE);
            break;
        case 11:
            //Serial.println("Select long pressed");
            PrintDisplay("SETTINGS");
            inMenu = !inMenu;
            delay(refreshRate);
            DisplaySetting();
            break;
        case 12:
            //Serial.println("Roll up pressed");
            PrintDisplay("NXT MODE");
            ToggleMode(1);
            break;
        case 13:
            //Serial.println("Roll down pressed");
            PrintDisplay("PRV MODE");
            ToggleMode(-1);
            break;
        default:
            //Serial.println("Index out of supported range");
            break;
        }
    }
}

void CLIO_CAN_syncOK()
{
    canSendResult = canBus.sendMsgBuf(DISPLAY_SYNC_FRAME_ID, 0, 8, (byte*)keepAlive);
}

void PrintDisplay(String s)
{
    // Fill string to 8 characters with whitespaces
    while (s.length() < 8)
    {
        s += " ";
    }

    // Trim if string is longer than 8 characters
    if (s.length() > 8)
    {
        s = s.substring(0, 7);
    }

    // Preparing message for display
    String TextCmd = "";
    TextCmd += '\x10';
    TextCmd += '\x19';
    TextCmd += '\x76';
    TextCmd += '\x60';
    TextCmd += '\x01';
    TextCmd += s;
    TextCmd += '\x10';
    TextCmd += s;      //s.substring(0, 7); // substring not necessary as already trimmed to 8 characters
    TextCmd += "    "; // 4 spaces to make a 12-byte string
    char charArray[28] = {'\0'};
    TextCmd.toCharArray(charArray, 27);

    // Sending message
    send_to_display(DISPLAY_CONTENT_FRAME_ID, (byte *)(charArray), 27);
}

void send_to_display(word id, byte *data, byte datasz)
{
    do_send_to(id, data, datasz, 0x81);
}

void do_send_to(word id, byte *data, byte datasz, byte filler)
{
    unsigned char packet[8] = {'\0'};
    byte packetnum = 0, i, slen = datasz;

    while (slen > 0)
    {
        i = 0;
        if (packetnum > 0)
        {
            packet[0] = 0x20 + packetnum; /* Another package with one message */
            i++;
        }

        while ((i < 8) && (slen > 0))
        {
            packet[i] = *data;
            data++;
            slen--;
            i++;
        }

        for (; i < 8; i++)
            packet[i] = filler;

        canBus.sendMsgBuf((unsigned long)(id), 0, 8, packet);
        packetnum++;
        delay(2);
    }
}

void CalculateVelocity()
{
    velocity = 0;
    pulseDuration = pulseIn(signalPin, LOW);
    if (pulseDuration > 0)
    {
        velocity = round((distance * 3600) / pulseDuration);
    }
}

void CalculateVoltage()
{
    pinReading = analogRead(voltagePin);
    volOut = (pinReading * 5.0) / 1024.0;
    volIn = volOut / (r2 / (r1 + r2));
    if (volIn < 0.09)
    {
        volIn = 0.0; //statement to quash undesired reading !
    }
}

void CalculateDistance()
{
    dist += (double)(millis() - refreshTime) * velocity / 3600000.0;
}

void DisplaySpeed()
{
    String msg("SPD  ");
    String s((int)velocity);
    if (s.length() == 1)
    {
        msg += "  ";
    }
    if (s.length() == 2)
    {
        msg += " ";
    }
    msg += s;
    PrintDisplay(msg);
}

void DisplayVoltage()
{
    // rounding voltage value to 1 decimal place
    int tempValue = (int)(volIn * 10 + 0.5);
    volIn = (float)tempValue / 10;

    String msg("VLT ");
    String s(volIn);
    s = s.substring(0, 4);
    msg += s;
    PrintDisplay(msg);
}

void DisplayDistance()
{
    String msg("DST ");
    String s(dist);
    // if distance is greater than 100km but below 1000km, then trim trailing decimal point and add a space to msg
    if (dist > 100 && dist < 1000)
    {
        msg += " ";
        s = s.substring(0, 3);
    }
    else
    {
        s = s.substring(0, 4);
    }

    msg += s;
    PrintDisplay(msg);
}

void AdjustVolume()
{
    if (velocity > settings[2])
    {
        // calculate expected value of added volume
        int expectedValue = floor((velocity - settings[2]) / settings[3]);
        // adjust volume if added volume level differs from expected
        if (addedVolume < expectedValue)
        {
            carRadio.Action(VOL_UP);
            addedVolume++;
        }
        if (addedVolume > expectedValue)
        {
            carRadio.Action(VOL_DOWN);
            addedVolume--;
        }
    }
}

void ToggleMode(int direction)
{
    int mode = (int)displayMode + direction;
    if (mode > 2)
    {
        displayMode = (DisplayModeEnum)0;
    }
    else if (mode < 0)
    {
        displayMode = (DisplayModeEnum)2;
    }
    else
    {
        displayMode = (DisplayModeEnum)mode;
    }
}

void ReadSettings()
{
    for (int i = 0; i < 4; i++)
    {
        int value = -1;
        EEPROM.get(i, value);
        // Override setting only if value is not default
        if (value != 255)
        {
            settings[i] = value;
        }
    }
}

void UpdateSettings()
{
    for (int i = 0; i < 4; i++)
    {
        EEPROM.update(i, settings[i]);
    }
}

void ToggleSetting(int direction)
{
    if (displayedSettingIndex + direction > 3)
    {
        displayedSettingIndex = 0;
    }
    else if (displayedSettingIndex + direction < 0)
    {
        displayedSettingIndex = 3;
    }
    else
    {
        displayedSettingIndex = displayedSettingIndex + direction;
    }
    DisplaySetting();
}

void ChangeSettingValue(int sign)
{
    switch (displayedSettingIndex)
    {
    case 0: // auto volume
        if (settings[displayedSettingIndex] == 0)
        {
            settings[displayedSettingIndex] = 1;
        }
        else
        {
            settings[displayedSettingIndex] = 0;
        }
        break;
    case 1: // auto volume initalization speed
    case 2: // auto volume speed step
        if ((settings[displayedSettingIndex] == 0 && sign == -1) || (settings[displayedSettingIndex] == 255 && sign == 1))
        {
            // ignoring
        }
        else
        {
            // increase/decrease by 5 km/h
            settings[displayedSettingIndex] += 5 * sign;
        }
        break;
    case 3: // default display mode
        int mode = settings[displayedSettingIndex] + sign;
        if (mode > 2)
        {
            settings[displayedSettingIndex] = 0;
        }
        else if (mode < 0)
        {
            settings[displayedSettingIndex] = 2;
        }
        else
        {
            settings[displayedSettingIndex] = mode;
        } 
        break;
    default:
        break;
    }
    DisplaySetting();
}

void DisplaySetting()
{
    String settingValue(settings[displayedSettingIndex]);
    String toDisplay = labels[displayedSettingIndex];

    int spacesToAdd = 8 - (toDisplay.length() + settingValue.length());
    for (int i = 0; i < spacesToAdd; i++)
    {
        toDisplay += " ";
    }
    toDisplay += settingValue;

    PrintDisplay(toDisplay);
}

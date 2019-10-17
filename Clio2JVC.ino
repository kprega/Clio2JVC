#include "SPI.h"
#include "mcp_can.h" // https://github.com/coryjfowler/MCP_CAN_lib
#include "mcp_can_dfs.h"
#include "SimpleTimer.h"
#include "JvcRadio.h"
#include <commands_dfs.h>

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
const int addedVolumeLimit = 8;     // volume can be increased up to 8 points
const int activationThreshold = 50; // km/h
const int changeThreshold = 10;     // km/h

// Variables used for speed calculation
const double distance = 100; // mm
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

const unsigned char CLIO_CAN_KEEPALIVE[8]     = {0x79, 0x00, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81};
const unsigned char CLIO_CAN_KEEPALIVE_ACK[8] = {0x69, 0x00, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2};
const unsigned char CLIO_CAN_5C1_MESSAGE[8]   = {0x74, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81};
const unsigned char CLIO_CAN_REMOTE_ACK[8]    = {0x74, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81};

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

void setup()
{
    // Wait for 2s to allow car power up all systems
    delay(2000);

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
    CLIO_CAN_startSync();
    delay(1);
    CLIO_CAN_syncOK();
    delay(1);
    CLIO_CAN_syncDisp(); // triggers 1c1 and 0a9 on the display side: response 5C1 and 4A9
    delay(10);
    canBus.sendMsgBuf(0x5C1, 0, 8, (byte*)CLIO_CAN_5C1_MESSAGE);
    canBus.sendMsgBuf(0x4A9, 0, 8, (byte*)CLIO_CAN_REMOTE_ACK);
    CLIO_CAN_initDisplay();
    delay(1);
    CLIO_CAN_registerDisplay();
    delay(1);
    CLIO_CAN_enableDisplay();
    delay(50 + 10);
    timer.setInterval(700, CLIO_CAN_syncOK);
}

void loop()
{
    // receive CAN
    if (!digitalRead(interruptPin))
    {
        canBus.readMsgBuf(&canFrameID, &canMsgLength, canReceivedMsg); // read data,  canMsgLength: data length, buf: data buf
        if (canFrameID == 0x3CF && memcmp(canReceivedMsg, CLIO_CAN_KEEPALIVE_ACK, 8) == 0)
        {
            // Keep alive reception confirmation received
        }
        else if (canFrameID == 0x521)
        {
            // Display confirms reception of new content
        }
        else if (canFrameID == 0x1C1)
        {
            // Ping - pong
            canBus.sendMsgBuf(0x5C1, 0, 8, (byte*)CLIO_CAN_5C1_MESSAGE);
        }
        // Receiving from the steering wheel remote
        else if (canFrameID == 0x0A9)
        {
            canSendResult = canBus.sendMsgBuf(0x4A9, 0, 8, (byte*)CLIO_CAN_REMOTE_ACK);
            //if (canSendResult == CAN_OK)
            //{
            //    Serial.println("CLIO_CAN_REMOTE_ACK Message Sent Successfully!");
            //}
            //else
            //{
            //    Serial.println("Error Sending CLIO_CAN_REMOTE_ACK Message...");
            //}

            // store key pressed time
            timeKeyPressed = millis();

            // Determine key pressed and mode - iterate over collection of messages
            int i = 0;
            int messagesCount = 14;
            while (i < messagesCount && memcmp(canReceivedMsg, RemoteMessages[i], 8) != 0)
            {
                i++;
            }
            switch (i)
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
                PrintDisplay("EQ");
                carRadio.Action(EQUALIZER);
                break;
            case 12:
                //Serial.println("Roll up pressed");
                PrintDisplay("MODE");
                ToggleMode();
                break;
            case 13:
                //Serial.println("Roll down pressed");
                PrintDisplay("MODE");
                ToggleMode();
                break;
            default:
                //Serial.println("Index out of supported range");
                break;
            }
        }
        //else
        //{
        //    Serial.print("CAN ID: ");
        //    Serial.print(canFrameID, HEX);
        //    Serial.print(" Data: ");
        //    for (int i = 0; i < canMsgLength; i++) // Print each byte of the data
        //    {
        //        if (canReceivedMsg[i] < 0x10) // If data byte is less than 0x10, add a leading zero
        //        {
        //            Serial.print("0");
        //        }
        //        Serial.print(canReceivedMsg[i], HEX);
        //        Serial.print(" ");
        //    }
        //    Serial.println();
        //}
    }

    timer.run();

    if (millis() - timeKeyPressed > timeout && millis() - refreshTime > refreshRate)
    {
        CalculateVelocity();
        CalculateVoltage();
        CalculateDistance();

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
        refreshTime = millis();
        
        AdjustVolume();
    }
}

void CLIO_CAN_startSync()
{
    unsigned char startSyncMsg[8] = {0x7A, 0x01, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81};
    canSendResult = canBus.sendMsgBuf(0x3DF, 0, 8, startSyncMsg);
    //    if (canSendResult == CAN_OK)
    //        Serial.println("startSync Message Sent Successfully!");
    //    else
    //        Serial.println("Error Sending startSync Message...");
}

void CLIO_CAN_syncOK()
{
    canBus.sendMsgBuf(0x3DF, 0, 8, (byte*)CLIO_CAN_KEEPALIVE);
    //if (canSendResult == CAN_OK)
    //    Serial.println("syncOK Message Sent Successfully!");
    //else
    //    Serial.println("Error Sending syncOK Message...");*/
}

void CLIO_CAN_syncDisp()
{
    unsigned char syncDispMsg[8] = {0x70, 0x1A, 0x11, 0x00, 0x00, 0x00, 0x00, 0x01};
    canBus.sendMsgBuf(0x3DF, 0, 8, syncDispMsg);
    //if (canSendResult == CAN_OK)
    //    Serial.println("syncDisp Message Sent Successfully!");
    //else
    //    Serial.println("Error Sending syncDisp Message...");
}

void CLIO_CAN_initDisplay()
{
    unsigned char initDispMsg[8] = {0x70, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81};
    canSendResult = canBus.sendMsgBuf(0x121, 0, 8, initDispMsg);
    //if (canSendResult == CAN_OK)
    //    Serial.println("initDisp Message Sent Successfully!");
    //else
    //    Serial.println("Error Sending initDisp Message...");
}

void CLIO_CAN_registerDisplay()
{
    unsigned char registerDispMsg[8] = {0x70, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81};
    canSendResult = canBus.sendMsgBuf(0x1B1, 0, 8, registerDispMsg);
    //if (canSendResult == CAN_OK)
    //    Serial.println("registerDisp Message Sent Successfully!");
    //else
    //    Serial.println("Error Sending registerDisp Message...");
}

void CLIO_CAN_enableDisplay()
{
    unsigned char enableDispMsg[8] = {0x04, 0x52, 0x02, 0xFF, 0xFF, 0x81, 0x81, 0x81};
    canSendResult = canBus.sendMsgBuf(0x1B1, 0, 8, enableDispMsg);
    //if (canSendResult == CAN_OK)
    //    Serial.println("enableDisp Message Sent Successfully!");
    //else
    //    Serial.println("Error Sending enableDisp Message...");
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
    send_to_display(0x121, (byte *)(charArray), 27);
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
        // Wait for the response
        //if (!digitalRead(_interruptPin))
        //{
        //    canBus.readMsgBuf(&canFrameId, &canMsgLength, canReceivedMsg); // read data
        //}
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
    dist += (double)(millis() - refreshTime) * velocity / 3600000.0 ;
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
        s.substring(0,3);
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
    if (velocity > activationThreshold)
    {
        // calculate expected value of added volume
        int expectedValue = floor((velocity - activationThreshold) / changeThreshold);
        // adjust volume if added volume level differs from expected
        if (addedVolume < expectedValue && addedVolume < addedVolumeLimit)
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

void ToggleMode()
{
    if ((int)displayMode == 2)
    {
        displayMode = (DisplayModeEnum)0;
    }
    else
    {
        displayMode = (DisplayModeEnum)((int)displayMode + 1);
    }
}

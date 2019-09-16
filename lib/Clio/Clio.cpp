// ----------------------------------------------------------------------------------- //
// Clio.cpp                                                                            //
// ----------------------------------------------------------------------------------- //
// Class for handling CAN bus messages in Renault Clio update list radio               //
// Based on:                                                                           //
// https://hackaday.io/project/28150-jvc-to-clio                                       //
// https://megane.com.pl/topic/47797-wyswietlacz-radia-update-list-protokol/           //
// https://hackaday.io/project/27439-smart-car-radio                                   //
// ----------------------------------------------------------------------------------- //

#include "Clio.h"
#include "mcp_can.h"
#include "mcp_can_dfs.h"
#include "can_messages.h"
#include "Arduino.h"
#include "string.h"

Clio::Clio(byte csPin, byte interruptPin, byte displaySwitchPin)
{
    _interruptPin = interruptPin;
    _csPin = csPin;
    _displaySwitchPin = displaySwitchPin;
    canBus = MCP_CAN(csPin);

    // start CAN
    if (canBus.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ))
    {
        Serial.println("CAN init OK");
    }
    else
    {
        Serial.println("CAN init failed");
    }
    canBus.setMode(MCP_NORMAL);
}

void Clio::SetupDisplay()
{
    digitalWrite(_displaySwitchPin, HIGH);
    SendMessageAndPrintSerial(0x3DF, START_SYNC, "START_SYNC");
    delay(1);
    SendMessageAndPrintSerial(0x3DF, KEEPALIVE, "KEEPALIVE");
    delay(1);
    SendMessageAndPrintSerial(0x3DF, SYNC_DISPLAY, "SYNC_DISPLAY");
    delay(10);
    SendMessageAndPrintSerial(0x5C1, RESPONSE_MESSAGE, "RESPONSE_MESSAGE");
    SendMessageAndPrintSerial(0x4A9, REMOTE_ACK, "REMOTE_ACK");
    SendMessageAndPrintSerial(0x121, INIT_DISPLAY, "INIT_DISPLAY");
    delay(1);
    SendMessageAndPrintSerial(0x1B1, REGISTER_DISPLAY, "REGISTER_DISPLAY");
    delay(1);
    SendMessageAndPrintSerial(0x1B1, ENABLE_DISPLAY, "ENABLE_DISPLAY");
    delay(50 + 10);
    Serial.println("Setup display complete");
}

void Clio::SendMessageAndPrintSerial(int frame, unsigned char *message, String msgName)
{
    canBusSendResult = canBus.sendMsgBuf(frame, 0, 8, message);
    String str;
    if (canBusSendResult == CAN_OK)
    {
        str += msgName;
        str += " message sent successfully";
    }
    else
    {
        str += "Error sending ";
        str += msgName;
        str += " message";
    }
    Serial.println(str);
}

void Clio::Sync()
{
    SendMessageAndPrintSerial(0x3DF, KEEPALIVE, "KEEPALIVE");
}

void Clio::PrintDisplay(String s)
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

    Serial.print("Printing \"");
    Serial.print(s);
    Serial.println("\" to the display");

    // Preparing message for display
    String TextCmd = "";
    TextCmd += '\x10';
    TextCmd += '\x19';
    TextCmd += '\x76';
    TextCmd += '\x60';
    TextCmd += '\x01';
    TextCmd += s;
    TextCmd += '\x10';
    TextCmd += s; //s.substring(0, 7); // substring not necessary as already trimmed to 8 characters
    TextCmd += "    "; // 4 spaces to make a 12-byte string
    char charArray[28] = {'\0'};
    TextCmd.toCharArray(charArray, 27);

    // Sending message
    send_to_display(0x121, (byte *)(charArray), 27);
}

void Clio::send_to_display(word id, byte *data, byte datasz)
{
    do_send_to(id, data, datasz, 0x81);
}

void Clio::do_send_to(word id, byte *data, byte datasz, byte filler)
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
        delay(2); /* TODO: Here we should wait for the display / radio response instead of the delay */
        if (!digitalRead(_interruptPin))
        {
            canBus.readMsgBuf(&canFrameId, &canMsgLength, canReceivedMsg); // read data
        }
    }
}

int Clio::ReceiveFromRemote()
{
    if (!digitalRead(_interruptPin))
    {
        canBus.readMsgBuf(&canFrameId, &canMsgLength, canReceivedMsg); // read data

        // KEEPALIVE message received confirmation
        if (canFrameId == 0x3CF && memcmp(canReceivedMsg, KEEPALIVE_ACK, 8))
        {
            Serial.println("KEEPALIVE_ACK received");
        }

        // Ping-pong messages
        if (canFrameId == 0x1C1 && memcmp(canReceivedMsg, RESPONSE_MESSAGE, 8))
        {
            Serial.println("RESPONSE_MESSAGE received");
            SendMessageAndPrintSerial(0x5C1, RESPONSE_MESSAGE, "RESPONSE_MESSAGE");
        }

        // Remote control messages
        if (canFrameId == 0x0A9)
        {
            // Send remote acknowledged
            SendMessageAndPrintSerial(0x4A9, REMOTE_ACK, "REMOTE_ACK");

            // Find out what button was pressed
            if (memcmp(canReceivedMsg, REMOTE_PAUSE, 8))             return 1;
            if (memcmp(canReceivedMsg, REMOTE_PAUSE_LONG, 8))        return 2;
            if (memcmp(canReceivedMsg, REMOTE_VOL_UP, 8))            return 3;
            if (memcmp(canReceivedMsg, REMOTE_VOL_UP_LONG, 8))       return 4;
            if (memcmp(canReceivedMsg, REMOTE_VOL_DOWN, 8))          return 5;
            if (memcmp(canReceivedMsg, REMOTE_VOL_DOWN_LONG, 8))     return 6;
            if (memcmp(canReceivedMsg, REMOTE_SELECT, 8))            return 7;
            if (memcmp(canReceivedMsg, REMOTE_SELECT_LONG, 8))       return 8;
            if (memcmp(canReceivedMsg, REMOTE_SOURCE_LEFT, 8))       return 9;
            if (memcmp(canReceivedMsg, REMOTE_SOURCE_LEFT_LONG, 8))  return 10;
            if (memcmp(canReceivedMsg, REMOTE_SOURCE_RIGHT, 8))      return 11;
            if (memcmp(canReceivedMsg, REMOTE_SOURCE_RIGHT_LONG, 8)) return 12;
            if (memcmp(canReceivedMsg, REMOTE_ROLL_DOWN, 8))         return 13;
            if (memcmp(canReceivedMsg, REMOTE_ROLL_UP, 8))           return 14;
        }
    }
    return 0;
}
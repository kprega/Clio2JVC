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

Clio::Clio(byte csPin, byte interruptPin)
{
    _interruptPin = interruptPin;
    _csPin = csPin;
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

void Clio::SendMessageAndPrintSerial(int frame, unsigned char *message, char *msgName)
{
    canBusSendResult = canBus.sendMsgBuf(frame, 0, 8, message);
    if (canBusSendResult == CAN_OK)
    {
        Serial.print(msgName);
        Serial.println(" message sent successfully");
    }
    else
    {
        Serial.print("Error sending ");
        Serial.print(msgName);
        Serial.println(" message");
    }
}

void Clio::Sync()
{
    SendMessageAndPrintSerial(0x3DF, KEEPALIVE, "KEEPALIVE");
}

void Clio::PrintDisplay(char text_to_display)
{
    char *arg;
    String s = "";

    arg = &text_to_display;
    while (arg != NULL)
    {
        s += arg;
        arg = &text_to_display;
        if (arg != NULL)
        { // check if space separator (if several spaces, just one is inserted)
            s += ' ';
        }
        if (s.length() > 8)
        { // check if length max
            s.remove(8);
        }
    }

    while (s.length() < 8)
    {
        s += ' '; // pad string with spaces to make 8 byte string
    }

    String TextCmd = "";
    TextCmd += '\x10';
    TextCmd += '\x19';
    TextCmd += '\x76';
    TextCmd += '\x60';
    TextCmd += '\x01';
    TextCmd += s;
    TextCmd += '\x10';
    TextCmd += s;
    TextCmd += "    "; // 4 spaces to make a 12-byte string
    char charArray[28] = {'\0'};
    TextCmd.toCharArray(charArray, 27);
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
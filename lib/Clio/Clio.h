// ----------------------------------------------------------------------------------- //
// Clio.h                                                                              //
// ----------------------------------------------------------------------------------- //
// Class for handling CAN bus messages in Renault Clio update list radio               //
// Based on:                                                                           //
// https://hackaday.io/project/28150-jvc-to-clio                                       //
// https://megane.com.pl/topic/47797-wyswietlacz-radia-update-list-protokol/           //
// https://hackaday.io/project/27439-smart-car-radio                                   //
// ----------------------------------------------------------------------------------- //

#ifndef Clio_h
#define Clio_h

#include "mcp_can.h"
#include "string.h"

class Clio
{
public:
	Clio(byte csPin, byte interruptPin, byte displaySwitchPin);
	void PrintDisplay(String str);
    void Sync();
    int ReceiveFromRemote();
    void DisplayOn();
    void DisplayOff();
    void ShowPacketData();
private:
    bool isNewMessageAvailable;
    byte _csPin;
    byte _interruptPin;
    byte _displaySwitchPin;
    MCP_CAN canBus = MCP_CAN(_csPin);
    byte canBusSendResult;
	void SetupDisplay();
    void SendMessage(int frame, unsigned char *message);
    void send_to_display(word id, byte *data, byte datasz);
    void do_send_to(word id, byte * data, byte datasz, byte filler);
    long unsigned int canFrameId;
    unsigned char canMsgLength;
    unsigned char canReceivedMsg[8];
};

#endif
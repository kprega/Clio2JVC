// ----------------------------------------------------------------------------------- //
// JVC.cpp                                                                             //
// ----------------------------------------------------------------------------------- //
// Class for handling communication with JVC car radio through the remote control wire //
// Based on:                                                                           //
// https://hackaday.io/project/28150-jvc-to-clio                                       //
// https://pastebin.com/fXbScxV4                                                       //
// ----------------------------------------------------------------------------------- // 

#include "JVC.h"
#include "Arduino.h"

JVC::JVC()
{

}

void JVC::SetupRemote(int remote_pin)
{
    _remotePin = remote_pin;
	pinMode(_remotePin, OUTPUT);
	digitalWrite(_remotePin, LOW);  // Output LOW to make sure optocoupler is off
}

void JVC::VolumeUp()
{
	SendCommand(VOL_UP);
}

void JVC::VolumeDown()
{
	SendCommand(VOL_DOWN);
}

void JVC::ToggleSource()
{
	SendCommand(SOURCE);
}

void JVC::Mute()
{
	SendCommand(MUTE);
}

void JVC::TrackForward()
{
	SendCommand(TRACK_FORW);
}

void JVC::TrackBack()
{
	SendCommand(TRACK_BACK);
}

void JVC::FolderForward()
{
	SendCommand(FOLDER_FORW);
}

void JVC::FolderBack()
{
	SendCommand(FOLDER_BACK);
}

void JVC::VoiceControl()
{
	SendCommand(VOICE_CONTROL);
}

void JVC::Equalizer()
{
	SendCommand(EQUALIZER);
}

void JVC::AnswerBTCall()
{
	SendCommand(BTCALL);
}

void JVC::TogglePower()
{
	SendCommand(POWER);
}

// Send a command to the radio, including the header, start bit, address and stop bits
void JVC::SendCommand(unsigned char value)
{
    Serial.print("Sending to JVC device: ");
    Serial.println(value);
    Preamble();                               // Send signals to precede a command to the radio
    for (unsigned char i = 0; i < 3; i++)     // Repeat address, command and stop bits three times so radio will pick them up properly
	{           
        SendValue(ADDRESS);                   // Send the address
        SendValue((unsigned char)value);      // Send the command
        Postamble();                          // Send signals to follow a command to the radio
    }
}

// Send a value (7 bits, LSB is sent first, value can be an address or command)
void JVC::SendValue(unsigned char value)
{
    unsigned char i, tmp = 1;
    for (i = 0; i < sizeof(value) * 8 - 1; i++)
    {
        if (value & tmp)  // Do a bitwise AND on the value and tmp
        {
            SendOne();
        }
        else
        {
            SendZero();	
        }
        tmp = tmp << 1; // Bitshift left by 1
    }
}

// Signals to transmit a '0' bit
void JVC::SendZero()
{
    digitalWrite(_remotePin, HIGH);      // Output HIGH for 1 pulse width
    delayMicroseconds(PULSE_WIDTH);
    digitalWrite(_remotePin, LOW);       // Output LOW for 1 pulse width
    delayMicroseconds(PULSE_WIDTH);
}

// Signals to transmit a '1' bit
void JVC::SendOne()
{
    digitalWrite(_remotePin, HIGH);      // Output HIGH for 1 pulse width
    delayMicroseconds(PULSE_WIDTH);
    digitalWrite(_remotePin, LOW);       // Output LOW for 3 pulse widths
    delayMicroseconds(PULSE_WIDTH * 3);
}

// Signals to precede a command to the radio
void JVC::Preamble()
{
    // HEADER: always LOW (1 pulse width), HIGH (16 pulse widths), LOW (8 pulse widths)
    digitalWrite(_remotePin, LOW);       // Make sure output is LOW for 1 pulse width, so the header starts with a rising edge
    delayMicroseconds(PULSE_WIDTH * 1);
    digitalWrite(_remotePin, HIGH);      // Start of header, output HIGH for 16 pulse widths
    delayMicroseconds(PULSE_WIDTH * 16);
    digitalWrite(_remotePin, LOW);       // Second part of header, output LOW 8 pulse widths
    delayMicroseconds(PULSE_WIDTH * 8);
    
    // START BIT: always 1
    SendOne();
}

// Signals to follow a command to the radio
void JVC::Postamble()
{
    // STOP BITS: always 1
    SendOne();
    SendOne();
}
  
// ----------------------------------------------------------------------------------- //
// JVC.cpp                                                                             //
// ----------------------------------------------------------------------------------- //
// Class for handling communication with JVC car radio through the remote control wire //
// Based on:                                                                           //
// https://hackaday.io/project/28150-jvc-to-clio                                       //
// https://www.avforums.com/threads/jvc-stalk-adapter-diy.248455/page-3                //
// https://pastebin.com/fXbScxV4                                                       //
// https://www.youtube.com/watch?v=8OANaTe5kxI                                         //
// ----------------------------------------------------------------------------------- // 

#include "JVC.h"
#include "commands_dfs.h"
#include "Arduino.h"

JVC::JVC()
{
    _interval = 2; // ms
    _waitTime = 20; // ms
}

void JVC::SetupRemote(int remote_pin)
{
    _remotePin = remote_pin;
	pinMode(_remotePin, OUTPUT);
	digitalWrite(_remotePin, LOW);  // Output LOW to make sure optocoupler is off
}

// Executes action given as parameter 
void JVC::Action(unsigned char action)
{
    // Works on KD-X342BT
    SendCommand((unsigned char) action);
	delay(_interval);
    SendCommand((unsigned char) action);
    delay(_waitTime);
}

// Send a command to the radio, including the header, start bit, address and stop bits
void JVC::SendCommand(unsigned char value)
{
    Preamble();                               // Send signals to precede a command to the radio      
    SendValue(ADDRESS);                       // Send the address
    SendValue((unsigned char)value);          // Send the command
    Postamble();                              // Send signals to follow a command to the radio
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
  
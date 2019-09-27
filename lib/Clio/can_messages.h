// ----------------------------------------------------------------------------------- //
// can_messages.h                                                                      //
// ----------------------------------------------------------------------------------- //
// Collection of messages appearing on CAN bus in Renault Clio update list radio       //
// Based on:                                                                           //
// https://hackaday.io/project/28150-jvc-to-clio                                       //
// https://megane.com.pl/topic/47797-wyswietlacz-radia-update-list-protokol/           //
// https://hackaday.io/project/27439-smart-car-radio                                   //
// ----------------------------------------------------------------------------------- //

#ifndef can_messages_h
#define can_messages_h

const unsigned char KEEPALIVE[8]                = {0x79, 0x00, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81};
const unsigned char KEEPALIVE_ACK[8]            = {0x69, 0x00, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2};
const unsigned char RESPONSE_MESSAGE[8]         = {0x74, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81};
const unsigned char REMOTE_ACK[8]               = {0x74, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81};

const unsigned char REMOTE_VOL_UP[8]            = {0x03, 0x89, 0x00, 0x03, 0xA2, 0xA2, 0xA2, 0xA2};
const unsigned char REMOTE_VOL_UP_LONG[8]       = {0x03, 0x89, 0x00, 0x43, 0xA2, 0xA2, 0xA2, 0xA2};
const unsigned char REMOTE_VOL_DOWN[8]          = {0x03, 0x89, 0x00, 0x04, 0xA2, 0xA2, 0xA2, 0xA2};
const unsigned char REMOTE_VOL_DOWN_LONG[8]     = {0x03, 0x89, 0x00, 0x44, 0xA2, 0xA2, 0xA2, 0xA2};
const unsigned char REMOTE_PAUSE[8]             = {0x03, 0x89, 0x00, 0x05, 0xA2, 0xA2, 0xA2, 0xA2};
const unsigned char REMOTE_PAUSE_LONG[8]        = {0x03, 0x89, 0x00, 0x85, 0xA2, 0xA2, 0xA2, 0xA2};
const unsigned char REMOTE_SOURCE_RIGHT[8]      = {0x03, 0x89, 0x00, 0x01, 0xA2, 0xA2, 0xA2, 0xA2};
const unsigned char REMOTE_SOURCE_RIGHT_LONG[8] = {0x03, 0x89, 0x00, 0x81, 0xA2, 0xA2, 0xA2, 0xA2};
const unsigned char REMOTE_SOURCE_LEFT[8]       = {0x03, 0x89, 0x00, 0x02, 0xA2, 0xA2, 0xA2, 0xA2};
const unsigned char REMOTE_SOURCE_LEFT_LONG[8]  = {0x03, 0x89, 0x00, 0x82, 0xA2, 0xA2, 0xA2, 0xA2};
const unsigned char REMOTE_SELECT[8]            = {0x03, 0x89, 0x00, 0x00, 0xA2, 0xA2, 0xA2, 0xA2};
const unsigned char REMOTE_SELECT_LONG[8]       = {0x03, 0x89, 0x00, 0x80, 0xA2, 0xA2, 0xA2, 0xA2};
const unsigned char REMOTE_ROLL_DOWN[8]         = {0x03, 0x89, 0x01, 0x01, 0xA2, 0xA2, 0xA2, 0xA2};
const unsigned char REMOTE_ROLL_UP[8]           = {0x03, 0x89, 0x01, 0x41, 0xA2, 0xA2, 0xA2, 0xA2};

const unsigned char START_SYNC[8]               = {0x7A, 0x01, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81};
const unsigned char SYNC_DISPLAY[8]             = {0x70, 0x1A, 0x11, 0x00, 0x00, 0x00, 0x00, 0x01};
const unsigned char INIT_DISPLAY[8]             = {0x70, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81};
const unsigned char REGISTER_DISPLAY[8]         = {0x70, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81};
const unsigned char ENABLE_DISPLAY[8]           = {0x04, 0x52, 0x02, 0xFF, 0xFF, 0x81, 0x81, 0x81};

// Wrapper for remote commands
const unsigned char* REMOTE_COMMANDS[14][8] = 
{ 
    REMOTE_VOL_UP,
    REMOTE_VOL_UP_LONG,
    REMOTE_VOL_DOWN,
    REMOTE_VOL_DOWN_LONG,
    REMOTE_PAUSE,
    REMOTE_PAUSE_LONG,
    REMOTE_SOURCE_RIGHT,
    REMOTE_SOURCE_RIGHT_LONG,
    REMOTE_SOURCE_LEFT,
    REMOTE_SOURCE_LEFT_LONG,
    REMOTE_SELECT,
    REMOTE_SELECT_LONG,
    REMOTE_ROLL_DOWN,
    REMOTE_ROLL_UP 
};

#endif
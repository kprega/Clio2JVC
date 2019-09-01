// ----------------------------------------------------------------------------------- //
// commands_dfs.h                                                                      //
// ----------------------------------------------------------------------------------- //
// Definitions for commands to control JVC car radio through the remote control wire   //
// Based on:                                                                           //
// https://hackaday.io/project/28150-jvc-to-clio                                       //
// https://www.avforums.com/threads/jvc-stalk-adapter-diy.248455/page-3                //
// https://pastebin.com/fXbScxV4                                                       //
// ----------------------------------------------------------------------------------- //

#ifndef commands_dfs_h
#define commands_dfs_h

// Address that the radio responds to
#define ADDRESS        0x47

// Pulse width in µs
#define PULSE_WIDTH    527

// Available commands
#define VOL_UP         0x04  // Volume up
#define VOL_DOWN       0x05  // Volume down
#define SOURCE         0x08  // Toggle source
#define EQUALIZER      0x0D  // Equalizer
#define MUTE           0x0E  // Mute
#define TRACK_FORW     0x12  // Track forward
#define TRACK_BACK     0x13  // Track backward
#define FOLDER_FORW    0x14  // Track forward hold
#define FOLDER_BACK    0x15  // Track backward hold
#define VOICE_CONTROL  0x1A  // Voice control
#define BTCALL         0x1B  // Answer bluetooth call
#define POWER          0x80  // Power on/off
#define UNKNOWN1       0x37  // Unknown function 1
#define UNKNOWN2       0x58  // Unknown function 2

#endif
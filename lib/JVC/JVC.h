// ----------------------------------------------------------------------------------- //
// JVC.h                                                                               //
// ----------------------------------------------------------------------------------- //
// Class for handling communication with JVC car radio through the remote control wire //
// Based on:                                                                           //
// https://hackaday.io/project/28150-jvc-to-clio                                       //
// https://pastebin.com/fXbScxV4                                                       //
// ----------------------------------------------------------------------------------- // 

#ifndef JVC_h
#define JVC_h

class JVC
{
	public:
        JVC();
		void SetupRemote(int remote_pin);
		void VolumeUp();
		void VolumeDown();
		void ToggleSource();
		void Mute();
		void TrackForward();
		void TrackBack();
		void FolderForward();
		void FolderBack();
		void VoiceControl();
		void Equalizer();
		void AnswerBTCall();
		void TogglePower();
	private:
	    unsigned int _remotePin;
		void SendCommand(unsigned char value);
		void SendValue(unsigned char value);
		void SendZero();
		void SendOne();
		void Preamble();
		void Postamble();

		// Commands for radio
	    const int VOL_UP             = 0x04;   // Volume up
	    const int VOL_DOWN           = 0x05;   // Volume down
	    const int SOURCE             = 0x08;   // Toggle source
	    const int EQUALIZER          = 0x0D;   // Equalizer
	    const int MUTE               = 0x0E;   // Mute
	    const int TRACK_FORW         = 0x12;   // Track forward
	    const int TRACK_BACK         = 0x13;   // Track backward
	    const int FOLDER_FORW        = 0x14;   // Track forward hold
	    const int FOLDER_BACK        = 0x15;   // Track backward hold
	    const int VOICE_CONTROL      = 0x1A;   // Voice control
	    const int BTCALL             = 0x1B;   // Answer bluetooth call
	    const int POWER              = 0x80;   // Power on/off
	    const int UNKNOWN1           = 0x37;   // Unknown function 1
	    const int UNKNOWN2           = 0x58;   // Unknown function 2
	    
	    // Other parameters
	    const int ADDRESS            = 0x47;   // Address that the radio responds to
	    const int PULSE_WIDTH        = 527;    // Pulse width in µs
};

#endif

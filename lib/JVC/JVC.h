// ----------------------------------------------------------------------------------- //
// JVC.h                                                                               //
// ----------------------------------------------------------------------------------- //
// Class for handling communication with JVC car radio through the remote control wire //
// Based on:                                                                           //
// https://hackaday.io/project/28150-jvc-to-clio                                       //
// https://www.avforums.com/threads/jvc-stalk-adapter-diy.248455/page-3                //
// https://pastebin.com/fXbScxV4                                                       //
// https://www.youtube.com/watch?v=8OANaTe5kxI                                         //
// ----------------------------------------------------------------------------------- //

#ifndef JVC_h
#define JVC_h

class JVC
{
public:
	JVC();
	void SetupRemote(int remote_pin);
	void Action(unsigned char action);
private:
	void SendCommand(unsigned char value);
	void SendValue(unsigned char value);
	unsigned int _remotePin;
	unsigned int _interval;
	unsigned int _waitTime;
	void SendZero();
	void SendOne();
	void Preamble();
	void Postamble();
};

#endif

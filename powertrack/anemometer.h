#ifndef ANEMOMETER
#define ANEMOMETER
class anemometer {
	int wind_speed;
	unsigned long anemom_timer;
	unsigned long anemom_last_time = 0;
	long anemom_period = 5000; // ms. anything lower than around 5000 (the response time of anemometer) just creates repeated results.
	long anemom_timeout = 10000; //ms of silence from anemometer before another query sent

	enum anemometerStates{tx, rx};
	enum anemometerStates anemometerState;

    // TODO: clean up length hard-coding if possible
	//static byte binary [3];
	static const byte query[8];
	const int query_len = 8; // sizeof query;
	static const byte ex_reply[7];
	const int reply_len = 7; //sizeof ex_reply;
	byte reply[8+7];
	//byte reply[query_len + reply_len]; // have to catch the loopback too

	static uint16_t hAnemomLogVar;
	HardwareSerial &Port;
	bool sampleAvailableFlag;
	
	public:
	
	anemometer(HardwareSerial &P): Port(P) {}
	void init();
	void loopHandler();
	
	void clearBuffer();
	int parseWindSpeed(byte *reply);
	
	bool sampleAvailable();
	float result();

};
#endif
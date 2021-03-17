#include <Arduino.h>
#include <SoftwareSerial.h>

// TODO
//
// write docu for why one tick equals 4 microseconds
// wrap the Serial.print function
// communication via bluetooth
// add support for multiple steppers

// CNC-Shield Pins
#define COOLEN	A3
#define EN	8	// PB0
#define X_DIR	5	// PD5
#define Y_DIR	6	// PD6
#define Z_DIR	7	// PD7
#define X_STP	2	// PD2
#define Y_STP	3	// PD3
#define Z_STP	4	// PD4
#define SPNEN  12
#define SPNDIR 13

#define	CLOCKWISE		0
#define	COUNTERCLOCKWISE	1
#define STARTPOINT		0
#define ENDPOINT		1
#define CMD_MAXLENGHT		128
#define MODE_RUN		0
#define MODE_STOP		1
#define MODE_TRAVEL		2


#define BT_SERIAL 1 // set to 1 to receive commands via BT serial on BT_SERIAL_(RX,TX)
#define BT_SERIAL_RX 12
#define BT_SERIAL_TX 13

#define LOG_ENABLED 1

#if LOG_ENABLED
const static char LOG_TIME_SEP[] = "@L";
const static char LOG_CMDKEYSTART[] = " [";
const static char LOG_CMDKEYEND[] = "]: ";
#define LOG_BEGIN(baudrate) Serial.begin(baudrate)
#define LOGA(key, msg, arg) Serial.print(millis()); Serial.print(LOG_TIME_SEP); Serial.print(__LINE__); Serial.print(LOG_CMDKEYSTART); Serial.print(F(key)); Serial.print(LOG_CMDKEYEND); Serial.print(msg); Serial.println(arg);
#define LOG(key, msg) Serial.print(millis()); Serial.print(LOG_TIME_SEP); Serial.print(__LINE__); Serial.print(LOG_CMDKEYSTART); Serial.print(F(key)); Serial.print(LOG_CMDKEYEND); Serial.println(msg)
#define LOGF(key, msg) LOG(key, F(msg))
#else
#define LOG_BEGIN(baudrate)
#define LOG(key, msg)
#define LOGF(key, msg)
#endif


byte y_mode = MODE_STOP;
long y_startpoint_distance = 0;
long y_endpoint = 0;
bool y_travel_destination = STARTPOINT;
unsigned int y_speed = 500;
byte cmd_char_cnt = 0;
char cmd[CMD_MAXLENGHT];

#define SHUTTER_RELEASE_PIN	COOLEN
#define SMS_MODE_TO_STARTPOINT	0
#define SMS_MODE_WAIT_OFFSET	1
#define SMS_MODE_TAKE_PICTURE	2
#define SMS_MODE_MOVE		3

byte sms_mode;
bool sms_mode_first_run;
bool runSMS = false;
unsigned int exposure_time_ms = 1000;
unsigned int exposure_time_offset_ms = 1000;
unsigned int sms_pictures_amount = 5;
unsigned int sms_pictures_taken = 0;
unsigned long prev_millis;

// counter and compare values
// XXX do we need those?
uint16_t tl_load = 0;
uint16_t tl_comp = 250;

#if BT_SERIAL
SoftwareSerial btSerial(BT_SERIAL_RX, BT_SERIAL_TX);
#endif

void commBegin() {
#if BT_SERIAL
	btSerial.begin(9600);
#else
	Serial.begin(9600);
#endif
}


bool commAvailable() {
#if BT_SERIAL
	return btSerial.available();
#else
	return Serial.available();
#endif
}

char commRead() {
#if BT_SERIAL
	return btSerial.read();
#else
	return Serial.read();
#endif
}

void setupSMS() {
	pinMode(SHUTTER_RELEASE_PIN, OUTPUT);
	digitalWrite(SHUTTER_RELEASE_PIN, HIGH);
}

void SMS() {
	switch (sms_mode) {
		case SMS_MODE_TO_STARTPOINT:
			if (sms_mode_first_run) {
				sms_mode_first_run = false;
				y_mode = MODE_TRAVEL;
				sms_pictures_taken = 0;
				LOGF("SMS", "now traveling to startpoint");
			}
			if (y_travel_destination == ENDPOINT) {
				y_mode = MODE_STOP;
				sms_mode_first_run = true;
				sms_mode = SMS_MODE_WAIT_OFFSET;
			}
			break;
		case SMS_MODE_WAIT_OFFSET:
			if (sms_mode_first_run) {
				sms_mode_first_run = false;
				LOGF("SMS", "waiting for offset");
				prev_millis = millis();
			}
			if (millis() >= prev_millis + exposure_time_offset_ms) {
				sms_mode_first_run = true;
				sms_mode = SMS_MODE_TAKE_PICTURE;
			}
			break;
		case SMS_MODE_TAKE_PICTURE:
			if (sms_mode_first_run) {
				sms_mode_first_run = false;
				digitalWrite(SHUTTER_RELEASE_PIN, LOW);
				LOGF("SMS", "taking picture ");
				LOG("SMS", sms_pictures_taken + 1);
				prev_millis = millis();
			}
			if (millis() >= prev_millis + exposure_time_ms) {
				digitalWrite(SHUTTER_RELEASE_PIN, HIGH);
				sms_pictures_taken++;
				sms_mode_first_run = true;
				sms_mode = SMS_MODE_MOVE;
			}
			break;
		case SMS_MODE_MOVE:
			if (sms_mode_first_run) {
				sms_mode_first_run = false;
				y_mode = MODE_TRAVEL;
				LOGF("SMS", "moving steps");
			}
			if (y_travel_destination == STARTPOINT) {
				runSMS = false;
				y_mode = MODE_STOP;
				LOGF("SMS", "finished");
				return;
			}
			int limit = y_endpoint / (sms_pictures_amount - 1) * sms_pictures_taken;
			if (y_startpoint_distance >= limit) {
				y_mode = MODE_STOP;
				LOGA("SMS", "slider position: ", y_startpoint_distance);
				LOGA("SMS", "y_endpoint: ", y_endpoint);
				LOGA("SMS", "sms_picture_amount: ", sms_pictures_amount);
				LOGA("SMS", "sms_pictures_taken: ", sms_pictures_taken);
				LOGA("SMS", "limit: ", limit);
				sms_mode_first_run = true;
				sms_mode = SMS_MODE_WAIT_OFFSET;
			}
			break;
		default:
			// should never reach this
			LOGF("SMS", "never reach this!");
			break;
	}
}

void setSpeed(unsigned int microseconds) {
	noInterrupts();
	tl_comp = microseconds / 4; // one tick equals 4 microseconds
	OCR1A = tl_comp;
	interrupts();
}

void printUsage() {
	LOGF("USE", "Please structure your command like this:");
	LOG("USE", );
	LOGF("USE", "sid=<0-2>;cmd=<set|get>;[speed,direction,startpoint,endpoint,mode,exposure_time_ms,exposure_time_offset_ms,sms_pictures_amount,runSMS]");
	LOG("USE", );
	LOG("USE", );
	LOGF("USE", "Where:");
	LOG("USE", );
	LOGF("USE", "<sid> is you stepper id. Not implemented yet, always use 0");
	LOG("USE", );
	LOGF("USE", "speed");
	LOGF("USE", "int as delay in microseconds");
	LOG("USE", );
	LOGF("USE", "direction");
	LOGF("USE", "0 : clockwise");
	LOGF("USE", "1 : counterclockwise");
	LOG("USE", );
	LOGF("USE", "mode");
	LOGF("USE", "0 : run infinitely");
	LOGF("USE", "1 : stop");
	LOGF("USE", "2 : travel mode");
	LOG("USE", );
	LOG("USE", );
	LOGF("USE", "Examples:");
	LOG("USE", );
	LOGF("USE", "sid=0;cmd=get;speed;");
	LOGF("USE", "sid=0;cmd=set;speed=50;");
	LOG("USE", );
	LOGF("USE", "sid=0;cmd=get;direction;");
	LOGF("USE", "sid=0;cmd=set;direction=1;");
	LOG("USE", );
	LOGF("USE", "sid=0;cmd=get;mode;");
	LOGF("USE", "sid=0;cmd=set;mode=2;");
}

void readCMD() {
	if(commAvailable()) {
		char c = commRead();
		if (cmd_char_cnt >= CMD_MAXLENGHT) {
			LOGF("RCMD ERROR", "your command is to long");
			cmd_char_cnt = 0;
			return;
		}
		if (c == '\r') {
			if (cmd_char_cnt == 0) {
				LOGF("RCMD", "please enter command or type help");
				return;
			}
			cmd[cmd_char_cnt] = '\0';
			cmd_char_cnt = 0;
		} else {
			cmd[cmd_char_cnt] = c;
			cmd_char_cnt++;
#if BT_SERIAL
		if (cmd_char_cnt >= 7 && (strncmp(cmd, "OK+LOST", 7) == 0 || strncmp(cmd, "OK+CONN", 7) == 0)) {
			LOGF("RCMD", "Stripping OK+(LOST|CONN) from command");
			cmd[0] = '\0';
			cmd_char_cnt = 0;
		}
#endif
			return;
		}
	} else {
		return;
	}

	LOG("RCMD HANDLE", cmd);

	// split the command parts
	char *cmdparts[7];
	char *ptr = NULL;
	byte index = 0;
	ptr = strtok(cmd, ";="); // takes a list of delimiters
	while (ptr != NULL) {
		cmdparts[index] = ptr;
		index++;
		ptr = strtok(NULL, ";="); // takes a list of delimiters
	}

	// get stepper id
	if (strcmp(cmdparts[0], "sid") != 0) {
		LOGF("RCMD ERROR", "no sid found");
		printUsage();
		return;
	}

	// get command
	if (strcmp(cmdparts[2], "cmd") != 0) {
		LOGF("RCMD ERROR", "no cmd found");
		printUsage();
		return;
	}

	// run command: get
	if (strcmp(cmdparts[3], "get") == 0) {
		if (strcmp(cmdparts[4], "speed") == 0) {
			LOG("RCMD", y_speed);
			return;
		}
		if (strcmp(cmdparts[4], "direction") == 0) {
			LOG("RCMD", digitalRead(Y_DIR));
			return;
		}
		if (strcmp(cmdparts[4], "startpoint") == 0) {
			LOG("RCMD", y_startpoint_distance);
			return;
		}
		if (strcmp(cmdparts[4], "endpoint") == 0) {
			LOG("RCMD", y_endpoint);
			return;
		}
		if (strcmp(cmdparts[4], "mode") == 0) {
			LOG("RCMD", y_mode);
			return;
		}
		if (strcmp(cmdparts[4], "exposure_time_ms") == 0) {
			LOG("RCMD", exposure_time_ms);
			return;
		}
		if (strcmp(cmdparts[4], "exposure_time_offset_ms") == 0) {
			LOG("RCMD", exposure_time_offset_ms);
			return;
		}
		if (strcmp(cmdparts[4], "sms_pictures_amount") == 0) {
			LOG("RCMD", sms_pictures_amount);
			return;
		}
		if (strcmp(cmdparts[4], "runSMS") == 0) {
			LOG("RCMD", runSMS);
			return;
		}
		LOG("RCMDERR UNKNOWNVAR", cmdparts[4]);
		return;
	}
	// run command: set
	if (strcmp(cmdparts[3], "set") == 0) {
		if (strcmp(cmdparts[4], "speed") == 0) {
			// XXX validate value
			y_speed = atoi(cmdparts[5]);
			setSpeed(y_speed);
			return;
		}
		if (strcmp(cmdparts[4], "direction") == 0) {
			// XXX validate value
			digitalWrite(Y_DIR, atoi(cmdparts[5]));
			return;
		}
		if (strcmp(cmdparts[4], "direction") == 0) {
			// XXX validate value
			digitalWrite(Y_DIR, atoi(cmdparts[5]));
			return;
		}
		if (strcmp(cmdparts[4], "startpoint") == 0) {
			y_mode = MODE_STOP;
			y_startpoint_distance = 0;
			LOGF("RCMD", "set startpoint");
			return;
		}
		if (strcmp(cmdparts[4], "endpoint") == 0) {
			y_mode = MODE_STOP;
			y_endpoint = y_startpoint_distance;
			LOGA("RCMD", "set endpoint ", y_endpoint);
			return;
		}
		if (strcmp(cmdparts[4], "mode") == 0) {
			// XXX validate value
			y_mode = atoi(cmdparts[5]);
			if (y_mode == MODE_TRAVEL) {
				// XXX y_startpoint_distance must not be 0
				if (y_startpoint_distance == 0) {
					y_mode = MODE_STOP;
				}
			}
			if (runSMS) {
				LOGF("SMS", "aborting SMS mode");
				runSMS = false;
			}
			return;
		}
		if (strcmp(cmdparts[4], "exposure_time_ms") == 0) {
			// XXX validate value
			exposure_time_ms = atoi(cmdparts[5]);
			return;
		}
		if (strcmp(cmdparts[4], "exposure_time_offset_ms") == 0) {
			// XXX validate value
			exposure_time_offset_ms = atoi(cmdparts[5]);
			return;
		}
		if (strcmp(cmdparts[4], "sms_pictures_amount") == 0) {
			// XXX validate value
			sms_pictures_amount = atoi(cmdparts[5]);
			return;
		}
		if (strcmp(cmdparts[4], "runSMS") == 0) {
			// XXX validate value
			runSMS = atoi(cmdparts[5]);
			if (runSMS) {
				if (y_endpoint == 0) {
					LOGF("RCMD ERROR", "travel distance not set");
					runSMS = false;
					return;
				}
				sms_mode = SMS_MODE_TO_STARTPOINT;
				sms_mode_first_run = true;
				return;
			}
		}
		LOG("RCMDERR UNKNOWNVAR", cmdparts[4]);
		return;
	}
}

void setup() {
	LOG_BEGIN(9600);
	commBegin();
	//Serial.setTimeout(60000);
	LOGF("SETUP", "Starting Firmware");
#if BT_SERIAL
	LOGF("USE_CMD", "BLE");
#else
	LOGF("USE_CMD", "SERIAL");
#endif
	pinMode(Y_DIR, OUTPUT);
	pinMode(Y_STP, OUTPUT);
	pinMode(EN, OUTPUT);
	digitalWrite(EN, LOW);
	setSpeed(y_speed);
	setupSMS();

	// Timer 1
	noInterrupts();

	// reset the Timer 1 Counter Control Register A
	TCCR1A = 0;

	// set CTC mode so timer register TCNT1 resets itlef after every match
	TCCR1B &= ~(1 << WGM13);
	TCCR1B |= (1 << WGM12);

	// set prescaler to 64
	TCCR1B &= ~(1 << CS12);
	TCCR1B |= (1 << CS11);
	TCCR1B |= (1 << CS10);

	// reset Timer1 and set compare value
	TCNT1 = tl_load;
	OCR1A = tl_comp;

	// enable Timer1 compare interrupt
	TIMSK1 |= (1 << OCIE1A);

	interrupts();
}

void loop() {
	if (runSMS) {
		SMS();
	}
	readCMD();
}

ISR(TIMER1_COMPA_vect) {
	if (y_mode == MODE_STOP) {
		return;
	}
	if (y_mode == MODE_TRAVEL) {
		if (y_startpoint_distance == 0) {
			y_travel_destination = ENDPOINT;
		}
		if (y_startpoint_distance == y_endpoint) {
			y_travel_destination = STARTPOINT;
		}
		if (y_travel_destination == STARTPOINT) {
			if (y_endpoint > 0) {
				digitalWrite(Y_DIR, COUNTERCLOCKWISE);
			} else {
				digitalWrite(Y_DIR, CLOCKWISE);
			}
		} else {
			if (y_endpoint > 0) {
				digitalWrite(Y_DIR, CLOCKWISE);
			} else {
				digitalWrite(Y_DIR, COUNTERCLOCKWISE);
			}
		}
	}
	if (digitalRead(Y_DIR) == CLOCKWISE) {
		y_startpoint_distance++;
	} else {
		y_startpoint_distance--;
	}
	PORTD ^= (1 << PD3);	// flip step pin
}

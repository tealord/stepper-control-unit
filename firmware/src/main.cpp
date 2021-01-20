#include <Arduino.h>

// TODO
//
// write docu for why one tick equals 4 microseconds
// wrap the Serial.print function
// communication via bluetooth
// add support for multiple steppers
// add support for bulb trigger

// CNC-Shield Pins
#define EN	8	// PB0
#define X_DIR	5	// PD5
#define Y_DIR	6	// PD6
#define Z_DIR	7	// PD7
#define X_STP	2	// PD2
#define Y_STP	3	// PD3
#define Z_STP	4	// PD4

#define	CLOCKWISE		0
#define	COUNTERCLOCKWISE	1
#define STARTPOINT		0
#define ENDPOINT		1
#define CMD_MAXLENGHT		50
#define MODE_RUN		0
#define MODE_STOP		1
#define MODE_TRAVEL		2

byte y_mode = MODE_STOP;
long y_startpoint_distance = 0;
long y_endpoint = 0;
bool y_travel_destination = STARTPOINT;
unsigned int y_speed = 500;
byte cmd_char_cnt = 0;
char cmd[CMD_MAXLENGHT];

// counter and compare values
// XXX do we need those?
uint16_t tl_load = 0;
uint16_t tl_comp = 250;

void setSpeed(unsigned int microseconds) {
	noInterrupts();
	tl_comp = microseconds / 4; // one tick equals 4 microseconds
	OCR1A = tl_comp;
	interrupts();
}

void printUsage() {
	Serial.println("Please structure your command like this:");
	Serial.println();
	Serial.println("sid=<0-2>;cmd=<set|get>;[speed,direction,startpoint,endpoint,mode]");
	Serial.println();
	Serial.println();
	Serial.println("Where:");
	Serial.println();
	Serial.println("<sid> is you stepper id. Not implemented yet, always use 0");
	Serial.println();
	Serial.println("speed");
	Serial.println("int as delay in microseconds");
	Serial.println();
	Serial.println("direction");
	Serial.println("0 : clockwise");
	Serial.println("1 : counterclockwise");
	Serial.println();
	Serial.println("mode");
	Serial.println("0 : run infinitely");
	Serial.println("1 : stop");
	Serial.println("2 : travel a distance, first call to reset and second call to set and run");
	Serial.println();
	Serial.println();
	Serial.println("Examples:");
	Serial.println();
	Serial.println("sid=0;cmd=get;speed;");
	Serial.println("sid=0;cmd=set;speed=50;");
	Serial.println();
	Serial.println("sid=0;cmd=get;direction;");
	Serial.println("sid=0;cmd=set;direction=1;");
	Serial.println();
	Serial.println("sid=0;cmd=get;mode;");
	Serial.println("sid=0;cmd=set;mode=2;");
}

void readCMD() {
	if(Serial.available()){
		char c = Serial.read();
		if (cmd_char_cnt >= CMD_MAXLENGHT) {
			Serial.println("error: your command is to long");
			cmd_char_cnt = 0;
			return;
		}
		if (c == '\r') {
			if (cmd_char_cnt == 0) {
				Serial.println("please enter command or type help");
				return;
			}
			cmd[cmd_char_cnt] = '\0';
			cmd_char_cnt = 0;
		} else {
			cmd[cmd_char_cnt] = c;
			cmd_char_cnt++;
				return;
		}
	} else {
		return;
	}

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
		Serial.println("error: no sid found");
		printUsage();
		return;
	}

	// get command
	if (strcmp(cmdparts[2], "cmd") != 0) {
		Serial.println("error: no cmd found");
		printUsage();
		return;
	}

	// run command: get
	if (strcmp(cmdparts[3], "get") == 0) {
		if (strcmp(cmdparts[4], "speed") == 0) {
			Serial.println(y_speed);
			return;
		}
		if (strcmp(cmdparts[4], "direction") == 0) {
			Serial.println(digitalRead(Y_DIR));
			return;
		}
		if (strcmp(cmdparts[4], "startpoint") == 0) {
			Serial.println(y_startpoint_distance);
			return;
		}
		if (strcmp(cmdparts[4], "endpoint") == 0) {
			Serial.println(y_endpoint);
			return;
		}
		if (strcmp(cmdparts[4], "mode") == 0) {
			Serial.println(y_mode);
			return;
		}
		Serial.print("error: '");
		Serial.print(cmdparts[4]);
		Serial.println("' is an unkown variable");
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
			y_startpoint_distance = 0;
			y_mode = MODE_STOP;
			return;
		}
		if (strcmp(cmdparts[4], "endpoint") == 0) {
			y_endpoint = y_startpoint_distance;
			y_mode = MODE_STOP;
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
			return;
		}
		Serial.print("error: '");
		Serial.print(cmdparts[4]);
		Serial.println("' is an unkown variable");
		return;
	}
}

void setup() {
	Serial.begin(9600);
	//Serial.setTimeout(60000);
	Serial.println("Starting Firmware");
	while(!Serial.available());
	pinMode(Y_DIR, OUTPUT);
	pinMode(Y_STP, OUTPUT);
	pinMode(EN, OUTPUT);
	digitalWrite(EN, LOW);
	setSpeed(y_speed);

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

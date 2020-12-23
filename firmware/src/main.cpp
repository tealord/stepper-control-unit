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
#define MODE_RUN		0
#define MODE_STOP		1
#define MODE_TRAVEL		2

byte y_mode = MODE_STOP;
long y_startpoint_distance = 0;
long y_endpoint = 0;
bool y_travel_destination = STARTPOINT;

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

void setup() {
	Serial.begin(9600);
	Serial.setTimeout(60000);
	while(!Serial.available());
	pinMode(Y_DIR, OUTPUT);
	pinMode(Y_STP, OUTPUT);
	pinMode(EN, OUTPUT);
	digitalWrite(EN, LOW);

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

	byte cmd_lenght_max = 35;
	char cmd[cmd_lenght_max];
	
	unsigned int y_speed = 100;
	setSpeed(y_speed);

	while (true) {
		int cmd_lenght = Serial.readBytesUntil('\r', cmd, cmd_lenght_max);
		if (cmd_lenght == cmd_lenght_max) {
			Serial.println("error: your command is to long");
			continue;
		}
		if (cmd_lenght == 0) {
			continue;
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
			continue;
		}

		// get command
		if (strcmp(cmdparts[2], "cmd") != 0) {
			Serial.println("error: no cmd found");
			printUsage();
			continue;
		}

		// run command: get
		if (strcmp(cmdparts[3], "get") == 0) {
			if (strcmp(cmdparts[4], "speed") == 0) {
				Serial.println(y_speed);
				continue;
			}
			if (strcmp(cmdparts[4], "direction") == 0) {
				Serial.println(digitalRead(Y_DIR));
				continue;
			}
			if (strcmp(cmdparts[4], "startpoint") == 0) {
				Serial.println(y_startpoint_distance);
				continue;
			}
			if (strcmp(cmdparts[4], "endpoint") == 0) {
				Serial.println(y_endpoint);
				continue;
			}
			if (strcmp(cmdparts[4], "mode") == 0) {
				Serial.println(y_mode);
				continue;
			}
			Serial.print("error: '");
			Serial.print(cmdparts[4]);
			Serial.println("' is an unkown variable");
			continue;
		}
		// run command: set
		if (strcmp(cmdparts[3], "set") == 0) {
			if (strcmp(cmdparts[4], "speed") == 0) {
				// XXX validate value
				y_speed = atoi(cmdparts[5]);
				setSpeed(y_speed);
				continue;
			}
			if (strcmp(cmdparts[4], "direction") == 0) {
				// XXX validate value
				digitalWrite(Y_DIR, atoi(cmdparts[5]));
				continue;
			}
			if (strcmp(cmdparts[4], "direction") == 0) {
				// XXX validate value
				digitalWrite(Y_DIR, atoi(cmdparts[5]));
				continue;
			}
			if (strcmp(cmdparts[4], "startpoint") == 0) {
				y_startpoint_distance = 0;
				y_mode = MODE_STOP;
				continue;
			}
			if (strcmp(cmdparts[4], "endpoint") == 0) {
				y_endpoint = y_startpoint_distance;
				y_mode = MODE_STOP;
				continue;
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
				continue;
			}
			Serial.print("error: '");
			Serial.print(cmdparts[4]);
			Serial.println("' is an unkown variable");
			continue;
		}
	}
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

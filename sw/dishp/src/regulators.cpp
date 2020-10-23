#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>

#include "main.h"

using namespace std;

// --- mock regulators ---

class mock_regulators: public regulators {
public:
	bool running() { return true; }
	bool error() { return false; }
	bool step(bool should_exit, pos_t pos[NO_OF_AXES])
	{
		for (int i = 0; i < NO_OF_AXES; i++) {
			/*
			pos_t vel, acc;
			vel = (pos[i] - old_vel[i]) * frequency;
			acc = (old_vel[i] - vel) * frequency;
			fprintf(stderr, "-- %ld %ld\n", vel, acc);
			assert(labs(vel) <= AXIS_MAXIMUM_VELOCITY[i]);
			assert(labs(acc) <= AXIS_MAXIMUM_ACCELERATION[i]);
			old_vel[i] = vel;
			old_pos[i] = pos[i];
			*/
		}

		return should_exit;
	}

private:
	pos_t old_pos[NO_OF_AXES];
	pos_t old_vel[NO_OF_AXES];
};

regulators *new_mock_regulators()
{
	return new mock_regulators();
}

// --- true regulators ---

#define debug(msg) { \
	time_t my_time = time(NULL); \
	cerr << strtok(ctime(&my_time),"\n") << ": INFO: " << msg << endl; \
}

#define error_msg(msg)	{ \
	time_t my_time = time(NULL); \
	cerr << strtok(ctime(&my_time),"\n") << ": ERROR: " << msg << endl; \
}

#ifdef NORPI
#define HIGH 1
#define LOW 1
#define OUTPUT 0
#define INPUT 0

#define digitalWrite(X,Y)
#define digitalRead(X)  0
#define pinMode(X,Y)
#define wiringPiSetup()

#include <unistd.h>
#define delayMicroseconds(X) usleep(X);
#else
#include <wiringPi.h>
#endif /* NORPI */

#include "axis_settings.h"

void* thread_main(void *arg);

/*
 * Base class for every control task
 * The task is running in a separate thread
*/
class Task
{
private:
	static void* thread_main(void *arg)
	{
		static_cast<Task *>(arg)->execute();
		return NULL;
	}

	void execute() {
		init();
		while (!loop(terminated));
		done();
	}
protected:
	pthread_t tid;
	bool terminated;

	// Define initialisation code here
	virtual void init() {debug("default init");}

	// Define loop code here
	virtual bool loop(bool request_to_terminate) {return true;}	// Return true when done

	// Define code when the task is finished
	virtual void done() {debug("default done");}

public:

	Task() {}

	virtual void start(int priority = 0) {
		terminated = false;

		// Setting priority of the task
		// It works only when the program is running as root
		pthread_attr_t tattr;
		sched_param param;
		pthread_attr_init(&tattr);

		#ifndef NORPI
			pthread_attr_getschedparam(&tattr, &param);
			param.sched_priority = priority;
			pthread_attr_setschedpolicy(&tattr, priority > 0 ? SCHED_FIFO : SCHED_OTHER);// SCHED_RR
			pthread_attr_setschedparam(&tattr, &param);
			pthread_attr_setinheritsched(&tattr, PTHREAD_EXPLICIT_SCHED);
		#endif /* NORPI */

		// Creating thread of the task
		int ret = pthread_create(&tid, &tattr, &thread_main, this);

		//pthread_attr_destroy(&tattr);

		if (ret!=0) {
			throw "Task: Thread with the required priority could not be created - try to run the program as root";
		}

		// Checking that the priority was set correctly
		#ifdef DEBUG
		int policy;
		pthread_getschedparam(tid, &policy, &param);
		if ((param.sched_priority != priority) || (priority > 0 ? policy!=SCHED_FIFO : policy!=SCHED_OTHER)) {
			cerr << "Task: Setting task priority failed: " << policy << "," << param.sched_priority << endl;
		}
		#endif
	}

	// This method terminates the running task
	virtual void terminate() {
		terminated = true;
	}

	// This method waits for the running task
	void waitFor() {
		pthread_join(tid, NULL);
		pthread_detach(tid);
	}

	// Vlakno musi byt ukonceno v odvozenem destruktoru nebo volanim metody waitFor() - doporuceno
	~Task() {
		//pthread_join(tid, NULL);
	}
};

/*
 * This is a base class of all position controllers.
 */
class PositionControl : public Task
{
private:
	sem_t sem;

	virtual bool loop(bool request_to_terminate) {

		if (!request_to_terminate) sem_wait(&sem);

		if (movEnable) {

			busy = true;

			update_position();

			busy = false;
		}

		return request_to_terminate;
	}
protected:
	virtual void update_position() {}
public:
	prec_t movTargetPos[AXISCOUNT];		// in
	prec_t movPosition[AXISCOUNT];			// out
	bool movEnable;						// in
	bool busy;							// out

	PositionControl() : Task() {
		sem_init(&sem, 0, 0);
		for (int i=0; i<AXISCOUNT; i++) {
			movTargetPos[i] = 0;
			movPosition[i] = 0;
		}
		movEnable = false;
		busy = false;
		start(99);
	}

	virtual void update() {
		sem_post(&sem);
	}

	virtual void terminate() {
		movEnable = false;
		terminated = true;
		sem_post(&sem);
		Task::terminate();
	}

	void reset() {
		for (int i=0; i<AXISCOUNT; i++) {
			movTargetPos[i] = 0;
			movPosition[i] = 0;
		}
	}

	~PositionControl() {
		sem_destroy(&sem);
	}
};

/*
 * This task implements position control of a servo motor in pulse mode
 * The task is running with real-time priority
*/
class PositionControl_Pulse : public PositionControl
{
private:
	prec_t movIncrement[AXISCOUNT];
	bool alldone;

	virtual void init() {
		debug("PositionControl_Pulse: init");
	}

	virtual void done() {
		debug("PositionControl_Pulse: done");
	}

	virtual void update_position() {
		while (!alldone) {

			alldone = true;

			for (int i=0; i<AXISCOUNT; i++) {

				if (movTargetPos[i] == movPosition[i]) continue;

				alldone = false;

				movPosition[i] += movIncrement[i];

				/*
				* Maximum input frequency for low speed channel is 500kbps = 250kpps
				* Maximum input frequency for high speed channel is 4Mbps = 2Mpps
				*
				* The duration of one pulse is 2x3=6us => 167kpps < 250kpps
				* The required frequency is 128kpps
				* Result: 128kpps < 167kpps < 250kpps
				*/

				// GPIO Up
				digitalWrite(STEP[i], HIGH);

			}

			// delay
			delayMicroseconds(3);

			// GPIO Down
			for (int i=0; i<AXISCOUNT; i++) digitalWrite(STEP[i], LOW);

			// delay
			delayMicroseconds(3);

		}
	}
public:
	PositionControl_Pulse() : PositionControl() {}

	virtual void update() {
		for (int i=0; i<AXISCOUNT; i++) {

			if (movTargetPos[i] == movPosition[i]) {
				// In position - do nothing
				continue;
			}

			//debug(movTargetPos[i]); // Uncomment to print output position to standard output

			alldone = false;

			if (movTargetPos[i] > movPosition[i]) {
				movIncrement[i] = 1;
				// Set positive direction
				digitalWrite(DIR[i], P_DIR[i]);
			}
			else {
				movIncrement[i] = -1;
				// Set negative direction
				digitalWrite(DIR[i], N_DIR[i]);
			}

		}

		#ifdef DEBUG
		/* This message indicates that the position controller is too slow
		* and there will be a lag error
		*/
		//if (busy) debug("PositionControl_Pulse: WARNING: too slow");
		if (busy) cerr << "WARNING: too slow: " << movTargetPos[0] << "," << movPosition[0] << endl;
		#endif

		if (!alldone) PositionControl::update();
	}

	~PositionControl_Pulse() {}
};

/*
 * This task implements control of all axes
 * An axis is composed of a servo motor and a servo driver
 * This task is responsible for initialisation of a servo driver and for
 * periodic check for errors
*/
class AxisControl : public regulators
{
private:
	PositionControl_Pulse *gAxis;
	int state = 0;
	prec_t dx, v0;

	virtual bool step(bool request_to_terminate, pos_t axisTargetPosition[AXISCOUNT]) {

		bool result = false;

		for (int i=0; i<AXISCOUNT; i++) {
			if (state==30) {	// Omezovac funguje jen v rezimu running
				if (axisTargetPosition[i] > axisMaximumPosition[i]) {
					axisTargetPosition[i] = axisMaximumPosition[i];
				}
				else if (axisTargetPosition[i] < axisMinimumPosition[i]) {
					axisTargetPosition[i] = axisMinimumPosition[i];
				}
			}

			// Read DI home signals
			axisHomeSignal[i] = digitalRead(HOME[i]);
		}

		if ((state<40) && request_to_terminate) state = 40;		// Request to terminate

		// Check for alarms (only in states when drivers are running)
		if ((state>=20) && (state<40) && !digitalRead(ALARM)) {
			error_msg("AxisControl: Error: Servo driver is in an error state");
			state = 255;
		}

		switch (state) {

		case 0:		// Axis OFF
			state = 10;
			break;

		case 10:		// Starting
			digitalWrite(POWER, HIGH);	// Power on
			dx = 1000;
			state = 11;
			break;

		case 11:	// Waiting for servo drives with time out
			if (digitalRead(ALARM)) {
				gAxis->movEnable = true;
				state = 20;
			}
			else if (dx-- <= 0) {
				error_msg("AxisControl: Error: Time out when switching power on");
				state = 255;
			}
			break;

		case 20:		// Homing
			debug("AxisControl: Homing");

			state = skiphoming ? 23 : 21;
			break;

		case 21:	// Going down and searching for home switch
			dx = 0;
			for (int i=0; i<AXISCOUNT; i++) {
				axisPosition[i] = gAxis->movPosition[i];  // Reading actual position from controller
				axisVelocity[i] = -axisHomeVelocity[i];
				if (axisHomeSignal[i]) axisVelocity[i] = 0;
				dx+=abs(axisVelocity[i]);

				axisPosition[i] += axisVelocity[i]/frequency;
				gAxis->movTargetPos[i] = axisPosition[i];  // Writing desired position to controller

				if (axisPosition[i] < -(axisMaximumPosition[i] - axisMinimumPosition[i])) {
					error_msg("AxisControl: Error: Home switch not found");
					state = 255;
					break;
				}
			}
			if (dx==0) state = 22; else gAxis->update();
			break;

		case 22:	// Going up
			dx = 0;
			for (int i=0; i<AXISCOUNT; i++) {
				axisPosition[i] = gAxis->movPosition[i];  // Reading actual position from controller
				axisVelocity[i] = axisHomeVelocity[i];
				if (!axisHomeSignal[i]) axisVelocity[i] = 0;
				dx+=abs(axisVelocity[i]);

				axisPosition[i] += axisVelocity[i]/frequency;
				gAxis->movTargetPos[i] = axisPosition[i];  // Writing desired position to controller

				if (axisPosition[i] > axisMaximumPosition[i]) {
					error_msg("AxisControl: Error: Home switch");
					state = 255;
					break;
				}
			}
			if (dx==0) state = 23; else gAxis->update();
			break;

		case 23:	// Homing done
			gAxis->reset();
			debug("AxisControl: Running");

			state = 30;
			break;

		case 30:		// Running
			for (int i=0; i<AXISCOUNT; i++) {
				// Writing desired position to controller
				gAxis->movTargetPos[i] = axisTargetPosition[i];
			}

			gAxis->update();
			break;

		case 40:		// Stopping
			debug("AxisControl: Stopping");
			gAxis->movEnable = false;
			if (!gAxis->busy) {
				digitalWrite(POWER, LOW);	// Power off
				state = 100;
			}
			break;

		case 100:	// Done
			result = true;
			break;

		case 255:	// Error
			// Wait for request to terminate and then exit with power off
			gAxis->movEnable = false;
			if (request_to_terminate) {
				digitalWrite(POWER, LOW);	// Power off
				result = true;
			}
			break;
		}

		return result;
	}

	virtual void done() {
		debug("AxisControl: done");
	}
public:
	prec_t axisMaximumVelocity[AXISCOUNT];		// in
	prec_t axisMaximumAcceleration[AXISCOUNT];	// in
	prec_t axisMaximumPosition[AXISCOUNT];		// in
	prec_t axisMinimumPosition[AXISCOUNT];		// in
	prec_t axisHomeVelocity[AXISCOUNT];		// in
	prec_t axisPosition[AXISCOUNT];	// out
	prec_t axisVelocity[AXISCOUNT];	// out
	bool axisHomeSignal[AXISCOUNT];		// out

	AxisControl() {
		gAxis = new PositionControl_Pulse;

		for (int i=0; i<AXISCOUNT; i++) {
			axisMaximumVelocity[i] = DEFAULT_AXIS_MAXIMUM_VELOCITY[i];
			axisMaximumAcceleration[i] = DEFAULT_AXIS_MAXIMUM_ACCELERATION[i];
			axisMaximumPosition[i] = DEFAULT_AXIS_MAXIMUM_POSITION[i];
			axisMinimumPosition[i] = DEFAULT_AXIS_MINIMUM_POSITION[i];
			axisHomeSignal[i] = false;
			axisHomeVelocity[i] = DEFAULT_AXIS_HOME_VELOCITY[i];
			axisVelocity[i] = 0;
			axisPosition[i] = 0;
		}
	}

	~AxisControl() {
		gAxis->terminate();
		gAxis->waitFor();
		delete gAxis;
	}

	bool running() {
		return state == 30;
	}

	bool error() {
		return state > 200;
	}
};

void init_pi()
{
	wiringPiSetup();

	for (int i=0; i<AXISCOUNT; i++) {
		pinMode(STEP[i], OUTPUT);
		pinMode(DIR[i], OUTPUT);
		pinMode(HOME[i], INPUT);
		digitalWrite(STEP[i], LOW);
		digitalWrite(DIR[i], LOW);
	}
	pinMode(ALARM, INPUT);
	pinMode(POWER, OUTPUT);
	digitalWrite(POWER, LOW);
}

void done_pi()
{
	for (int i=0; i<AXISCOUNT; i++) {
		digitalWrite(STEP[i], LOW);
		digitalWrite(DIR[i], LOW);
	}
	digitalWrite(POWER, LOW);
}

regulators *new_true_regulators()
{
	init_pi();

	return new AxisControl();
}

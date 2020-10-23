#include <iostream>
#include <deque>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

using namespace std;

#include "main.h"
#include "poly.h"
#include "util.h"

bool simulate = false;
bool nowait = false;

#include <time.h>
utime_t utime_now() {
	struct timespec ts;
	if (clock_gettime(CLOCK_TAI, &ts) < 0) {
		fprintf(stderr, "clock_gettime(CLOCK_TAI, ...) unavailable\n");
		exit(1);
	}
	return ((utime_t) ts.tv_sec)*1000000 + ts.tv_nsec/1000;
}

struct pos_and_vel
{
	pos_t pos, vel;
};

pos_t interp(pos_t a, pos_t b, double w)
{
	pos_t m = 1<<16;
	pos_t w_ = (w*m + 0.5);
	return (b*w_ + a*(m-w_)) / m;
}

struct axis_interp
{
	axis_interp() : scheduled_until(-1), scheduled_start(-1), p({}) {}

	void schedule(utime_t now, pos_t pos, pos_t vel,
				  std::deque<utime_t> xs, std::deque<pos_t> ys) {
		if (xs.size() == 1) {
			scheduled_start = now;
			scheduled_until = xs[0];
			p = quadratic(utime_to_sec(xs[0] - now), -((double) period)/1e6,
						  ys[0], pos, vel);
			return;
		}

		double y2_slope = ((double) (ys[1] - ys[0])) / utime_to_sec(xs[1] - xs[0]);
		if (vel == 0) {
			double s = 2*(pos-ys[0])/y2_slope;
			if (s <= 0 && s >= utime_to_sec(now - xs[0]) && s >= 1e-3) {
				scheduled_start = sec_to_utime(s) + xs[0];
				scheduled_until = xs[0];
				double c = y2_slope / 2 / -s;
				p = poly({pos, 0, c});
				return;
			}
		}

		scheduled_start = now;
		scheduled_until = xs[0];
		//cerr << "schedule: " << -((double) period)/1e6 << " " << utime_to_sec(xs[0] - now)
		//		<< " " << pos << " " << ys[0] << " " << vel << " " << y2_slope << endl;
		p = cubic(-((double) period)/1e6, utime_to_sec(xs[0] - now),
				  pos, ys[0], vel, y2_slope);
	}

	pos_and_vel step(utime_t now, pos_t pos, pos_t vel,
				std::deque<utime_t> xs, std::deque<pos_t> ys) {
		/*
		if (now > scheduled_until
				&& xs.size() > 0
				&& utime_to_sec(xs[0] - now) < 60.0) {
			schedule(now, pos, vel, xs, ys);
		}

		if (now <= scheduled_until) {
			if (now >= scheduled_start) {
				double dt = utime_to_sec(now - scheduled_start);
				return pos_and_vel{p(dt), p.d()(dt)};
			} else {
				return pos_and_vel{p(0), 0};
			}
		}

		return pos_and_vel{pos, 0};
		*/

		if (now <= xs[0] - sec_to_utime(1)) {
			return pos_and_vel{
				pos,
				0
			};
		} 

		if (xs.size() == 1) {
			return pos_and_vel{
				ys[0],
				0
			};
		}

		double y2_slope = ((double) (ys[1] - ys[0])) / utime_to_sec(xs[1] - xs[0]);
		return pos_and_vel{
			interp(ys[0], ys[1], utime_to_sec(now - xs[0]) / utime_to_sec(xs[1] - xs[0])),
			y2_slope + 0.5
		};
	}

	void flush() {
		scheduled_start = 0;
		scheduled_until = 0;
	}

private:
	utime_t scheduled_start, scheduled_until;
	poly p;
};

enum {
	IDLE = 0,
	TRACK = 1,
	MOVE = 2
};

struct axis
{
	const int id;
	const string name;
	const pos_t minPos, maxPos, maxVelHard, maxAccel;
	double maxVelSoft; // maximum velocity set by user

	axis_interp interp;
	pos_t current;
	pos_t currentSub;
	pos_t currentVel;
	pos_t target;

	axis(int id, string name, pos_t minPos, pos_t maxPos, pos_t maxVel, pos_t maxAccel, double maxVelSoft)
		: id(id), name(name), minPos(minPos), maxPos(maxPos), maxVelHard(maxVel), maxAccel(maxAccel),
		  maxVelSoft(maxVelSoft), current(0), currentSub(0), currentVel(0), target(0) {}

	pos_t step(utime_t now, int mode,
			   std::deque<utime_t> track_xs,
			   std::deque<pos_t> track_ys)
	{
		pos_t targetVel, newTarget;
		pos_and_vel target_;

		switch (mode) {
		case TRACK:
			target_ = interp.step(
				now, current, currentVel, track_xs, track_ys
			);
			target = target_.pos;
			targetVel = target_.vel;
			break;

		case MOVE:
			// target set in loop()
			targetVel = 0;
			break;

		default:
		case IDLE:
			target = current;
			targetVel = 0;
		}

		target = clamp(target, minPos, maxPos);

		pos_t dir, newVel;
		if (target > current)
			dir = 1;
		else if (target < current)
			dir = -1;
		else
			dir = 0;

		pos_t dx = (target - current) * dir;

		// HACK: maxAccel is lowered to supposedly insert the effect of discretization
		//       in the formula below, as the formula is derived under assumption
		// 		 of continuous deceleration
		pos_t maxAccelReduced = maxAccel - maxAccel/12;

		pos_t maxVel = min(maxVelHard, (pos_t) maxVelSoft);
		newVel = min(
			maxVel,
			(pos_t) (sqrt(2*dx*maxAccelReduced + targetVel*targetVel))
		) * dir;

		newVel = min(
			newVel*dir,
			(pos_t) abs(target*frequency + frequency/2 - currentSub)
		) * dir;

		// clamp velocity change to -maxAccel/frequency, +maxAccel/frequency
		newVel = clamp(
			newVel,
			currentVel - maxAccel/frequency,
			currentVel + maxAccel/frequency
		);

		// TODO: clamp velocity so that we can always decelerate before hitting the axis limit

		// apply velocity change
		currentVel = newVel;

		// clamp velocity to range -maximum, +maximum. should not be necessary given
		// the computation above, but better be safe than sorry
		currentVel = clamp(currentVel, -maxVel, maxVel);

		// move with current velocity
		currentSub += currentVel;
		current = currentSub / frequency;

		return current;
	}

	bool in_position()
	{
		return current == target && currentVel == 0;
	}
};

struct native_coords : coord_system {
	string name() {
		return "native";
	}

	void to_pulses(pos_t *pulses, double *user) {
		for (int i = 0; i < NO_OF_AXES; i++)
			pulses[i] = user[i];
	}

	void from_pulses(pos_t *pulses, double *user) {
		for (int i = 0; i < NO_OF_AXES; i++)
			user[i] = pulses[i];
	}
};

struct user_coords : coord_system {
	string name() {
		return "user";
	}

	void to_pulses(pos_t *pulses, double *user) {
		pulses[0] = user[0]*12+0.5;
		pulses[1] = (user[1]*170+0.5)/3;
	}

	void from_pulses(pos_t *pulses, double *user) {
		user[0] = ((double) pulses[0])/12;
		user[1] = ((double) pulses[1])*3/170;
	}
};

coord_system *coord_systems[] = {
	new native_coords(),
	new user_coords(),
	NULL
};

coord_system *lookup_coord_system(string name) {
	coord_system **s;
	for (s = coord_systems; *s != NULL; s++)
		if ((*s)->name() == name)
			break;
	return *s;
}

axis axes[NO_OF_AXES] = {
	axis(0, "alt", 0, 240000, 2*12000, 12000, 12000),
	axis(1, "az", -1530000, 1530000, 2*17000, 17000, 17000)
};

struct point {
	pos_t pulses[NO_OF_AXES];
	utime_t time;
	char mode;

	void print() {
		char b[512];
		double u[NO_OF_AXES];
		int n = 0;
		coord_system **s;
		bool first = true;
		for (s = coord_systems; *s != NULL; s++) {
			string sname = (*s)->name();
			(*s)->from_pulses(pulses, u);
			for (int i = 0; i < NO_OF_AXES; i++) {
				if (n > 0) n += snprintf(b+n, sizeof(b)-n, ",");
				n += snprintf(b+n, sizeof(b)-n, "\"%s_%s\":%.4lf",
							  axes[i].name.c_str(), sname.c_str(), u[i]);
			}
		}
		printf("{\"t\":%" PRId64 ",%s,\"mode\":\"%c\"}\n", time, b, (int) mode);
		fflush(stdout);
	}
};

static sem_t new_point, point_printed;
static point point_to_print;
void print_points_thread()
{
	while (true) {
		sem_wait(&new_point);
		point_to_print.print();
		sem_post(&point_printed);
	}
}
void init_point_printing()
{
	sem_init(&new_point, 0, 0);
	sem_init(&point_printed, 0, 0);
	sem_post(&point_printed);
	thread(print_points_thread).detach();
}

void concurrent_print(point p)
{
	// we attempt to print the point in a separate thread
	// this way a stuck write will not block the realtime loop
	if (sem_trywait(&point_printed) < 0)
		return; // last point not printed yet, drop the new point
	point_to_print = p;
	sem_post(&new_point);
}

// a parameter whose value can be modified from the textual command input
struct param {
	const char *cname; // parameter name
	double *tdouble; // pointer to parameter double value, if applicable
	bool *tbool; // ditto bool value, if applicable

	param(const char *cname, double *tdouble, bool *tbool)
		: cname(cname), tdouble(tdouble), tbool(tbool)
	{
	}

	void set(string s)
	{
		if (tbool != NULL) {
			if (s == "") {
				*tbool = true;
			} else {
				*tbool = atoi(s.c_str()) != 0;
			}
		} else if (tdouble != NULL) {
			*tdouble = atof(s.c_str());
		}
	}
};

bool exit_on_error = false;

param params[] = {
	param("errexit", NULL, &exit_on_error),
	param("altspeed", &(axes[0].maxVelSoft), NULL),
	param("azspeed", &(axes[1].maxVelSoft), NULL)
};

int loop(regulators *regulators)
{
	int exit_code = 0;
	deque<utime_t> track_xs;
	deque<pos_t> track_ys[NO_OF_AXES];
	int mode = IDLE;
	bool retained_cmd = false;
	bool should_exit = false;
	deque<string> cmd_qu;
	utime_t now = 0;
	coord_system *curr_coords = lookup_coord_system("user");

	bool realtime = !(simulate && nowait);

	if (realtime)
		now = utime_now();

	while (true) {
		{
			string c;
			while (try_pick_up_cmd(c)) {
				if (c == "flush")
					cmd_qu.clear();
				cmd_qu.push_back(c);
			}
		}

		while (!cmd_qu.empty()) {
			utime_t track_x;
			double track_y[NO_OF_AXES];
			pos_t track_y_pulses[NO_OF_AXES];
			bool retained_cmd = false;
			string cmd = cmd_qu[0];

			if (sscanf(cmd.c_str(), "move %lf %lf",
					   &track_y[0], &track_y[1]) == 2) {
				if (mode == TRACK) {
					cerr << "Error: Dish busy with tracking, run 'flush' to abort: " << cmd << endl;
				} else {
					curr_coords->to_pulses(track_y_pulses, track_y);
					for (int i = 0; i < NO_OF_AXES; i++)
						axes[i].target = track_y_pulses[i];
					mode = MOVE;
				}
			} else if (sscanf(cmd.c_str(), "point %" PRId64 " %lf %lf",
							  &track_x, &track_y[0], &track_y[1]) == 3) {
				if (!track_xs.empty() && track_x <= track_xs.back()) {
					cerr << "Error: Tracking point too early: " << cmd << endl;
				} else {
					track_xs.push_back(track_x);
					curr_coords->to_pulses(track_y_pulses, track_y);
					for (int i = 0; i < NO_OF_AXES; i++)
						track_ys[i].push_back(track_y_pulses[i]);
				}
			} else if (cmd == "track") {
				mode = TRACK;
				if (track_xs.size() > 0)
					fprintf(stderr, "First tracking point in %.2f s\n", utime_to_sec(track_xs[0] - now));
			} else if (cmd == "flush") {
				track_xs.clear();
				for (int i = 0; i < NO_OF_AXES; i++) {
					track_ys[i].clear();
					axes[i].interp.flush();
				}
				mode = IDLE;
			} else if (cmd == "exit") {
				should_exit = true;
			} else if (cmd == "waitidle") {
				// waitidle will be processed once dish is idle
				// until then, we keep the command at head of queue
				if (mode != IDLE)
					break;
			} else if (strncmp(cmd.c_str(), "set ", 4) == 0) {
				// locate name in string
				const char *nameb = cmd.c_str() + 4;
				while (*nameb == ' ') nameb++;
				const char *namee = strchr(nameb, ' ');
				if (namee == NULL) namee = nameb+strlen(nameb);
				// locate value in string
				const char *valb = namee;
				while (*valb == ' ') valb++;
				// set parameter
				const int nparams = sizeof(params)/sizeof(param);
				int i;
				for (i = 0; i < nparams; i++)
					if (strlen(params[i].cname) == (namee-nameb)
						&& strncmp(nameb, params[i].cname, namee-nameb) == 0)
						break;
				if (i != nparams) {
					params[i].set(string(valb));
				} else {
					cerr << "No such parameter: " << string(nameb, namee-nameb) << endl;
				}
			} else {
				cerr << "Invalid command: " << cmd << endl;
			}

			cmd_qu.pop_front();
		}

		if (regulators->error() && exit_on_error) {
			exit_code = 1;
			should_exit = true;
		}

		if (!regulators->running()) {
			// regulators are not initialized yet
			pos_t empty[NO_OF_AXES];
			if (regulators->step(should_exit, empty))
				break;
			if (realtime) {
				now += period;
				usleep(std::max(0, (int) (now - utime_now())));
			}
			continue;
		}

		switch (mode) {
		case TRACK:
			/*
			while (track_xs.size() > 0 && track_xs[0] <= now) {
				track_xs.pop_front();
				for (int i = 0; i < NO_OF_AXES; i++)
					track_ys[i].pop_front();
			}
			if (track_xs.empty())
				mode = IDLE;
			*/
			while (track_xs.size() > 1 && track_xs[1] <= now) {
				track_xs.pop_front();
				for (int i = 0; i < NO_OF_AXES; i++)
					track_ys[i].pop_front();
			}
			if (track_xs.size() <= 1) {
				track_xs.clear();
				for (int i = 0; i < NO_OF_AXES; i++)
					track_ys[i].clear();
				mode = IDLE;
			}
			break;
		case MOVE:
			{
				bool done = true;
				for (int i = 0; i < NO_OF_AXES; i++)
					if (!axes[i].in_position())
						done = false;
				if (done)
					mode = IDLE;
			}
			break;
		}

		pos_t pos[NO_OF_AXES];
		for (int i = 0; i < NO_OF_AXES; i++)
			pos[i] = axes[i].step(now, mode, track_xs, track_ys[i]);
		if (regulators->step(should_exit, pos))
			break;

		point p;
		{

			char mode_c;
			switch (mode) {
			case IDLE:
				mode_c = 'I';
				break;
			case MOVE:
				mode_c = 'M';
				break;
			case TRACK:
				mode_c = 'T';
				break;
			default:
				mode_c = '*';
			}
			p.time = now;
			for (int i = 0; i < NO_OF_AXES; i++)
				p.pulses[i] = axes[i].current;
			p.mode = mode_c;
		}

		if (realtime)
			concurrent_print(p);
		else
			p.print();

		now += period;
		if (realtime)
			usleep(std::max(0, (int) (now - utime_now())));
	}

	delete regulators;
	return exit_code;
}

int main(int argc, char *argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, "tsn")) != -1) {
		switch (opt) {
		case 's':
			simulate = true;
			break;
		case 'n':
			nowait = true;
			break;
		case 't':
			fprintf(stderr, "%" PRId64 "\n", utime_now());
			return 0;
		default:
			cerr << "Usage: " << argv[0] << " [-s] [-n]" << endl;
			return 1;
		}
	}

	init_cmd_entry();
	init_point_printing();

	regulators *g;
	if (simulate)
		g = new_mock_regulators();
	else
		g = new_true_regulators();

	return loop(g);
}

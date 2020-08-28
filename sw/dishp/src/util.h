#include <iostream>
#include <thread>
#include <semaphore.h>

double utime_to_sec(utime_t t) {
	return ((double) t) / 1e6;
}
utime_t sec_to_utime(double s) {
	return s*1e6;
}

template <class T>
T clamp(T v, T lo, T hi) {
	if (v < lo)
		return lo;
	if (v < hi)
		return v;
	return hi;
}

static string line;
static sem_t have_cmd, cmd_picked_up;
void read_commands_thread()
{
	while (true) {
		if (!getline(cin, line))
			line = "exit";
		sem_post(&have_cmd);
		sem_wait(&cmd_picked_up);
	}
}
void init_cmd_entry()
{
	sem_init(&have_cmd, 0, 0);
	sem_init(&cmd_picked_up, 0, 0);
	thread(read_commands_thread).detach();
}
bool try_pick_up_cmd(string &target)
{
	if (sem_trywait(&have_cmd) < 0)
		return false;
	target = line;
	sem_post(&cmd_picked_up);
	return true;
}

#ifndef __MAIN_H
#define __MAIN_H

#include <stdint.h>
#include <string>

#define NO_OF_AXES 2

typedef int64_t utime_t;
typedef int64_t pos_t;
typedef pos_t prec_t;
const utime_t period = 4000;
const int period_us = period;
const int frequency = 250;

struct coord_system {
	virtual std::string name() = 0;
	virtual void to_pulses(pos_t *pulses, double *user) = 0;
	virtual void from_pulses(pos_t *pulses, double *user) = 0;
};

/* abstract class for regulators */
class regulators {
public:
	virtual bool running() = 0;
	virtual bool error() = 0;
	virtual bool step(bool should_exit, pos_t pos[NO_OF_AXES]) = 0;
};
regulators *new_mock_regulators();
regulators *new_true_regulators();

extern bool skiphoming;

#endif /* __MAIN_H */

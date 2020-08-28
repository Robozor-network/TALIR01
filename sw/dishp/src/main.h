#ifndef __MAIN_H
#define __MAIN_H

#include <stdint.h>

#define NO_OF_AXES 2

typedef int64_t utime_t;
typedef int64_t pos_t;
typedef pos_t prec_t;
const utime_t period = 4000;
const int period_us = period;
const int frequency = 250;

/* abstract class for regulators */
class regulators {
public:
	virtual bool running() = 0;
	virtual bool error() = 0;
	virtual bool step(bool should_exit, pos_t pos[NO_OF_AXES]) = 0;
};
regulators *new_mock_regulators();
regulators *new_true_regulators();

#endif /* __MAIN_H */

#ifndef AXIS_SETTINGS
#define AXIS_SETTINGS

// Sampling period of the controller
#define PERIOD_MS	4

// Define number of axes here <1 .. 6>
#define AXISCOUNT	2

// Default axis maximum velocity [pulses/s]
const prec_t DEFAULT_AXIS_MAXIMUM_VELOCITY[] =  {12000, 17000};		// 1200 x 10 = 10 mm/s, 17000 x 1 = 3 deg/s

// Default axis maximum acceleration [pulses/s2]
const prec_t DEFAULT_AXIS_MAXIMUM_ACCELERATION[] = {12000, 17000};		// 1200 x 10 = 10 mm/s2, 17000 x 1 = 3 deg/s2 (max rychlost dosazena za 1 s)

// Default axis maximum position [pulses]
const prec_t DEFAULT_AXIS_MAXIMUM_POSITION[] = {240000, 1530000};		// 1200 x 200 = 200 mm, 17000 x 180/3 = 180 deg

// Default axis minimum position [pulses]
const prec_t DEFAULT_AXIS_MINIMUM_POSITION[] = {0, -1530000}; 			// 1200 x 0 = 0 mm, -180 deg

// Default axis homing velocity [pulses/s]
const prec_t DEFAULT_AXIS_HOME_VELOCITY[] = {12000, 17000};			// 1200 x 10 = 10 mm/s, 17000 x 1 = 3 deg/s

// [out] GPIOs for step signal
const int STEP[] = {5, 4};

// [out] GPIOs for direction signal
const int DIR[] = {2, 0};

// [in] GPIO for alarm (error) signal
const int ALARM = 29;

// [in] GPIOs for home position signal
const int HOME[] = {28, 25};

// [out] GPIO for power signal
const int POWER = 21;

// Signal level for positive direction
const int P_DIR[] = {LOW, HIGH};

// Signal level for negative direction
const int N_DIR[] = {HIGH, LOW};

// Transformation: actuator position -> pulses
const prec_t num[] = {1200, 17000};
const prec_t den[] = {100, 300};

#endif

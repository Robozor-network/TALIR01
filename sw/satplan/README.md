# satplan

`satplan` searches for upcoming satellite passes and, optionally, produces a dishp tracking script which contains the pass trajectory.

Supplying the script to `dishp` will cause the dish to track the satellite pass in the sky. The script will first move the dish into waiting position at the beginning of the pass trajectory, then wait for the pass to start, then track the satellite in the sky and, after the pass is over, move the dish into the home position (0, 0).


```
$ ./satplan.py --help
usage: satplan.py [-h] [--satname SAT] [-n NO] [--dishp] [SINCE]

Search for satellite passes and prepare dishp tracking scripts. Times are UTC.
Location is hardcoded to Svákov observatory.

positional arguments:
  SINCE          point in time at which to start searching for a satellite
                 pass (default: now)

optional arguments:
  -h, --help     show this help message and exit
  --satname SAT  name of satellite of interest (default: 2019-038AE)
  -n NO          number of upcoming passes to list (default: 5)
  --dishp        export a dishp tracking script for the first upcoming pass,
                 redirect stdout to save to a file

$ ./satplan.py
Next 5 passes of sat '2019-038AE':

               Start     End        Max Ele
  ------------------------------------------
   2020-08-19  12:56:26  13:08:26      50.4
               14:29:26  14:44:26      13.6
   2020-08-20  00:26:26  00:35:26       2.4
               01:59:26  02:14:26      39.7
               03:32:26  03:47:26      16.9


$ ./satplan.py --dishp > l
Found pass between 2020-08-1912:54:18Z and 2020-08-1913:09:18Z with maximum elevation 39.8
Mapping azimuth into actuator range -270 to 90
flush
move 48.21 -2821.73
point 1597841794271847 48.21 -2821.73
point 1597841802271844 100.54 -2842.66
point 1597841810271841 154.04 -2864.61
point 1597841818271837 208.80 -2887.65
point 1597841826271834 264.88 -2911.86
point 1597841834271831 322.39 -2937.33
point 1597841842271828 381.42 -2964.16
point 1597841850271825 442.07 -2992.46
point 1597841858271822 504.48 -3022.34

...

point 1597842474271578 162.34 -18785.20
point 1597842482271575 109.69 -18805.50
point 1597842490271572 58.15 -18824.80
point 1597842498271569 7.68 -18843.16
waitidle
track
waitidle
move 0 0
waitidle
```

## Bugs

 * Passes are searched for by sampling the sat's altitude and azimuth once per minute into the future, starting with now and iteratively adding 1 minute. This causes the found start and end time and max ele to fluctuate.

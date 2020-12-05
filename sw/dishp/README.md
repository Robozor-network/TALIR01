# dishp

`dishp` is a computer program responsible for mechanically moving the dish to point in directions of interest. It controls two regulators which in turn drive motors moving the dish in azimuth and elevation.

## Commands

`dishp` reads commands separated by newline on its standard input.

 * `move <x> <y>`: Start moving the dish into position `<x> <y>`.

 * `point <t> <x> <y>`: Append a tracking point at time `<t>` and position `<x> <y>` to the tracking point list.

 * `track`: Put the dish into tracking mode. In tracking mode, the dish movement smoothly passes through the points specified by earlier `point` commands.

 * `waitidle`: Wait and do not process subsequent commands until the dish finishes movements due to earlier `move` or `track` commands.

 * `flush`: Abort tracking or movement, if in progress, and clear the command queue. The command's effect will be immediate even if preceded by unfinished `waitidle` commands. The command also clears the list of tracking points, and skips over homing if in progress.

 * `set <param> <value>`: Set parameter `<param>` to value `<value>`. The following parameters may be set through this command:

    * `errexit`: Influences `dishp` behaviour when regulators are in an error state. Value may be `1`, in which case the program will exit with non-zero exit code, or `0`, in which case the program will keep running. In both cases an error message will be printed to stderr.

    * `azspeed` and `altspeed`: Maximal speed for movement in x and y axis. Hard limits in source code prevent maximal speed to be set too high.

~~Axis coordinates are in unit of one hundreth of a degree~~ (wip). Times are UTC, in microseconds since the epoch.

## Homing Behaviour

During start-up, to find the zero position on both axes, the program goes through the following procedure.

	1. Go continuously down until hitting the home switch. Abort if going further than size of the axis range, i.e. if going further than the difference between maximum and minimum of the axis range.
	2. Go continuously up until releasing the home switch. Abort if, measured from the start-up point, we have gone further than size of the axis range.
	3. Stop and take current position as zero.

## License & Authorship

This program contains adapted parts of an original software for control of actuators by Evžen Thöndel of [PRAGOLET, s.r.o](http://www.pragolet.cz/) (2020). These acquired parts were adapted, and other parts of the program were written, by Martin Povišer and other respective Git commiters. By agreement of all authors, the source code of this program is released for anyone's use and reproduction under the following license.

```
Copyright 2020 Evžen Thöndel
Copyright 2020 Martin Povišer, and other dishp contributors

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

# controlmux

`controlmux` is a Go program for multiplexing the control of an arbitrary other program. It takes in the other program's name, and allows clients to take over the process' stdin and stdout for periods of time. `controlmux` enforces exclusivity of control and allows higher-priority client to seize control from lower-priority client at any time. `controlmux` also starts up the encapsulated program on-demand and stops it after a period of idleness.

In TALIR01, `controlmux` encapsulates the dish movement control program `dishp` and allows multiple clients to compete for control of the dish.


```
$ ./controlmux -help
Usage of ./controlmux:
  -name string
        (client only) name of the control claimant, to identify in error messages (default "unknown")
  -pri string
        (server only) comma-separated priority levels, from lowest to highest (default "a,b")
  -program string
        (server only) name of the program to encapsulate and multiplex control of
  -server
        switch to server process
  -socket string
        path to socket. in server mode, a dash and a priority level name will be appended to the path and separate socket will posted for each priority level
  -timeout int
        (server only) timeout in seconds, the maximum amount of time the encapsulated process can spend unattended, i.e. without a controlling client, before it is caused to exit by closing its standard input (default 5)
```

package main

import (
	"bytes"
	"errors"
	"flag"
	"os"
	"os/exec"
	"strings"
	"time"
)

type session struct {
	pingch          chan struct{}
	started         chan struct{}
	exited          chan struct{}
	pipeIn, pipeOut *os.File

	err error
}

func newSession() (*session, error) {
	s := new(session)
	s.pingch = make(chan struct{})
	s.started = make(chan struct{})
	s.exited = make(chan struct{})

	var err error
	s.pipeOut, s.pipeIn, err = os.Pipe()
	if err != nil {
		return nil, err
	}
	go s.run()
	return s, nil
}

func (s *session) ping() {
	select {
	case s.pingch <- struct{}{}:
	case <-s.exited:
	}
}

func (s *session) waitWithTimeout(d time.Duration) (bool, error) {
	select {
	case <-time.After(d):
		return false, nil
	case <-s.exited:
		return true, s.err
	}
}

func (s *session) run() {
	defer close(s.exited)

	cmd := exec.Command(flag.Args()[0], flag.Args()[1:]...)
	cmd.Stdin = s.pipeOut
	cmd.Stdout = os.Stdout
	var buff bytes.Buffer
	cmd.Stderr = &buff

	s.err = cmd.Start()
	if s.err != nil {
		return
	}

	var waitErr error
	waitDone := make(chan struct{})
	go func() {
		waitErr = cmd.Wait()
		close(waitDone)
	}()

	t := time.NewTimer(5 * time.Second)
loop:
	for {
		select {
		case <-s.pingch:
			if !t.Stop() {
				<-t.C
			}
			t.Reset(5 * time.Second)
		case <-t.C:
			s.pipeIn.Close()
		case <-waitDone:
			break loop
		}
	}

	if buff.Len() > 0 {
		s.err = errors.New(strings.TrimSpace(buff.String()))
	} else if waitErr != nil {
		s.err = waitErr
	}
}

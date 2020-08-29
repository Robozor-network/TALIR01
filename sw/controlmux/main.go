package main

import (
	"errors"
	"flag"
	"fmt"
	"io"
	"log"
	"net"
	"os"
	"os/exec"
	"strings"
	"sync"
	"syscall"
	"time"

	"golang.org/x/sys/unix"
)

var (
	server     = flag.Bool("server", false, "switch to server process")
	socketPath = flag.String("socket", "", "socket path. in server mode, separate socket is posted for each priority level."+
		" in that case the full socket path is formed by appending the user-provided path, dash and a priority level name")
	priorityLevels = flag.String("pri", "a,b", "(server only) comma-separated priority levels, from lowest to highest")
	name           = flag.String("name", "unknown", "(client only) name of the control claimant, to identify in error messages")
	timeout        = flag.Int("timeout", 5, "(server only) timeout in seconds, the maximum amount of time the encapsulated "+
		"process can spend unattended, i.e. without a controlling client, before it is caused to exit by closing its standard input")
)

type Owner struct {
	Name string
	Prio int
}

type KingOfTheHill struct {
	kickch  chan error
	freech  chan struct{}
	claimed bool
	owner   Owner
	sync.Mutex
}

func NewKOTH() *KingOfTheHill {
	k := new(KingOfTheHill)
	return k
}

func (k *KingOfTheHill) Claim(owner Owner) (kickch <-chan error, freech chan<- struct{}, err error) {
	k.Lock()
	defer k.Unlock()

	if k.claimed {
		select {
		case <-k.freech:
			k.claimed = false
		default:
		}
	}

	if k.claimed {
		if owner.Prio > k.owner.Prio {
			k.kickch <- errors.New(fmt.Sprintf("claimed by '%s'", owner.Name))
			close(k.kickch)
			<-k.freech
			k.claimed = false
		} else {
			return nil, nil, errors.New(fmt.Sprintf("claimed by '%s'", k.owner.Name))
		}
	}

	k.claimed = true
	k.owner = owner
	k.kickch = make(chan error)
	k.freech = make(chan struct{})

	return k.kickch, k.freech, nil
}

type process struct {
	pipeIn       io.WriteCloser
	pipeOutBuffs chan []byte
	exitErr      error
	pipesReady   chan struct{}
	closed       chan struct{}
	exited       chan struct{}

	timeout *time.Timer

	*KingOfTheHill
}

func startProcess() *process {
	p := new(process)
	p.exited = make(chan struct{})
	p.closed = make(chan struct{})
	p.pipesReady = make(chan struct{})
	p.pipeOutBuffs = make(chan []byte)
	p.timeout = time.NewTimer(2 * time.Second)
	p.KingOfTheHill = NewKOTH()
	go p.run()
	return p
}

func (p *process) run() {
	var err error
	programName := flag.Args()[0]
	defer func() {
		if err != nil {
			log.Printf("'%s' exited with error: %s", programName, err)
			p.exitErr = err
		} else {
			log.Printf("'%s' exited gracefully", programName)
		}
		close(p.exited)
	}()
	log.Printf("running '%s'", programName)
	cmd := exec.Command(programName, flag.Args()[1:]...)
	p.pipeIn, err = cmd.StdinPipe()
	if err != nil {
		return
	}
	pipeOut, err := cmd.StdoutPipe()
	if err != nil {
		return
	}
	cmd.Stderr = os.Stderr
	outCopyErrch := make(chan error)
	go func() {
		var buff [4096]byte
		var err error
		for {
			n, err := pipeOut.Read(buff[:])
			if n > 0 {
				p.pipeOutBuffs <- buff[:n]
			}
			if err != nil {
				if err == io.EOF {
					err = nil
				}
				break
			}
		}
		close(p.pipeOutBuffs)
		outCopyErrch <- err
	}()
	close(p.pipesReady)
	err = cmd.Start()
	if err != nil {
		return
	}
	go func() {
		defer close(p.closed)
		select {
		case <-p.exited:
			return
		case <-p.timeout.C:
		}
		p.pipeIn.Close()
	}()
	err = <-outCopyErrch
	if err != nil {
		cmd.Wait()
		return
	}
	err = cmd.Wait()
}

type AttachReader interface {
	io.Reader
	SetReadDeadline(t time.Time) error
}

type AttachWriter interface {
	io.Writer
	SetWriteDeadline(t time.Time) error
}

func (p *process) attach(owner Owner, in AttachReader, out AttachWriter) error {
	select {
	case <-p.exited:
		return nil
	case <-p.pipesReady:
	}

	kickch, freech, err := p.Claim(owner)
	if err != nil {
		return err
	}
	defer close(freech)

	if !p.timeout.Stop() {
		return errors.New("closed")
	}
	defer p.timeout.Reset(2 * time.Second)

	var wg sync.WaitGroup
	wg.Add(2)

	errch := make(chan error, 2)

	go func() {
		_, err := io.Copy(p.pipeIn, in)
		errch <- err
		wg.Done()
	}()

	exitcopyoutch := make(chan struct{})
	go func() {
	loop:
		for {
			select {
			case <-exitcopyoutch:
				break loop
			case b, ok := <-p.pipeOutBuffs:
				if !ok {
					errch <- nil
					break loop
				}
				_, err := out.Write(b)
				if err != nil {
					errch <- err
					break loop
				}
			}
		}
		wg.Done()
	}()

	select {
	case err = <-kickch:
	case err = <-errch:
	}

	in.SetReadDeadline(time.Now())
	out.SetWriteDeadline(time.Now())
	close(exitcopyoutch)
	wg.Wait()
	in.SetReadDeadline(time.Time{})
	out.SetWriteDeadline(time.Time{})
	return err
}

func (p *process) Exited() bool {
	select {
	case <-p.exited:
		return true
	default:
		return false
	}
}

func (p *process) Closed() bool {
	select {
	case <-p.closed:
		return true
	default:
		return false
	}
}

var (
	proc  *process
	procM sync.Mutex
)

func serve(conn *net.UnixConn, pri int) {
	defer conn.Close()

	b := make([]byte, 1024)
	oob := make([]byte, unix.CmsgSpace(8))

	n, oobn, _, _, err := conn.ReadMsgUnix(b, oob)
	if err != nil {
		log.Printf("handshake read: %s", err)
		return
	}

	b = b[:n]
	oob = oob[:oobn]

	scms, err := unix.ParseSocketControlMessage(oob)
	if err != nil || len(scms) != 1 {
		log.Printf("handshake oob parse: err=%s len(scms)=%d", err, len(scms))
		return
	}

	fds, err := unix.ParseUnixRights(&scms[0])
	if err != nil || len(fds) != 2 {
		log.Printf("handshake parse unix rights: err=%s len(fds)=%d", err, len(fds))
		return
	}

	syscall.SetNonblock(fds[0], true)
	syscall.SetNonblock(fds[1], true)
	f1 := os.NewFile(uintptr(fds[0]), "")
	f2 := os.NewFile(uintptr(fds[1]), "")
	defer f1.Close()
	defer f2.Close()

	var procRef *process

acquire_ref:
	procM.Lock()
	if proc != nil && proc.Exited() {
		proc = nil
	}
	if proc == nil {
		proc = startProcess()
	}
	procRef = proc
	procM.Unlock()

	if proc.Closed() {
		<-proc.exited
		goto acquire_ref
	}

	quitsigch := make(chan struct{}, 2)
	var wg sync.WaitGroup
	wg.Add(2)
	go func() {
		err := procRef.attach(Owner{string(b), pri}, f1, f2)
		if err != nil {
			conn.Write([]byte(err.Error()))
		}
		quitsigch <- struct{}{}
		wg.Done()
	}()
	go func() {
		var b [1]byte
		conn.Read(b[:])
		quitsigch <- struct{}{}
		wg.Done()
	}()

	<-quitsigch
	f1.SetReadDeadline(time.Now())
	conn.SetReadDeadline(time.Now())
	wg.Wait()
	f1.SetReadDeadline(time.Time{})
	conn.SetReadDeadline(time.Time{})
}

func listen(socketPath string, prio int) {
	l, err := net.Listen("unix", socketPath)
	if err != nil {
		log.Print(err)
		return
	}
	for {
		conn, err := l.Accept()
		if err != nil {
			log.Print(err)
		}

		go serve(conn.(*net.UnixConn), prio)
	}
}

func connect(socketPath string) {
	addr := &net.UnixAddr{Name: socketPath, Net: "unix"}
	c, err := net.DialUnix("unix", nil, addr)
	if err != nil {
		log.Print(err)
		return
	}
	defer c.Close()
	_, _, err = c.WriteMsgUnix([]byte(*name), unix.UnixRights(0, 1), nil)
	if err != nil {
		log.Print(err)
		return
	}
	errFlag := false
	for {
		var buff [1024]byte
		n, _, _, _, err := c.ReadMsgUnix(buff[:], nil)
		if err != nil {
			if !errors.Is(err, io.EOF) {
				fmt.Fprintln(os.Stderr, err)
			}
			break
		}
		b := buff[:n]
		errFlag = true
		fmt.Fprintln(os.Stderr, string(b))
	}
	if errFlag {
		os.Exit(1)
	}
}

func main() {
	flag.Usage = func() {
		fmt.Fprintf(os.Stderr, "Usage: %s [options] [ENCAPSULATED_PROGRAM PROGRAM_ARGS...]\n", os.Args[0])
		fmt.Fprintf(os.Stderr, "\nOptions:\n")
		flag.PrintDefaults()
	}
	flag.Parse()

	if *socketPath == "" {
		fmt.Fprint(os.Stderr, "No socket path\n\n")
		flag.Usage()
		os.Exit(1)
	}

	if *server {
		if flag.NArg() == 0 {
			fmt.Fprint(os.Stderr, "No encapsulated program name\n\n")
			flag.Usage()
			os.Exit(1)
		}

		var wg sync.WaitGroup
		for i, name := range strings.Split(*priorityLevels, ",") {
			wg.Add(1)
			go func(name string, i int) {
				listen(*socketPath+"-"+name, i)
				wg.Done()
			}(name, i)
		}
		wg.Wait()
	} else {
		connect(*socketPath)
	}
}

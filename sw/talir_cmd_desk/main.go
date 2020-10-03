package main

import (
	"bufio"
	"flag"
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
	"os/exec"
	"sync"
	"time"

	"github.com/gorilla/websocket"
)

var (
	listenAddr = flag.String("listen", ":8080", "the address to listen on")
	staticPath = flag.String("static", "static", "path to directory with static files to serve")
	logProgram = flag.String("logProgram", "log.sh", "program to follow logs through")
)

/*
var (
	cmdch = make(chan string)
)


var logBroadcast = NewLineBroadcast(5)

func Log(line string) {
	logBroadcast.Input() <- line
}

func logFromReader(reader io.Reader) {
	scanner := bufio.NewScanner(reader)
	for scanner.Scan() {
		Log(scanner.Text())
	}
}
*/

var upgrader = websocket.Upgrader{
	ReadBufferSize:  1024,
	WriteBufferSize: 1024,
}

func logHandler(w http.ResponseWriter, r *http.Request) {
	cmd := exec.Command(*logProgram)
	out, err := cmd.StdoutPipe()
	if err != nil {
		http.Error(w, err.Error(), 500)
		return
	}

	err = cmd.Start()
	if err != nil {
		http.Error(w, err.Error(), 500)
		return
	}

	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Println(err)
		cmd.Process.Signal(os.Interrupt)
		return
	}
	defer conn.Close()

	scanner := bufio.NewScanner(out)
	for scanner.Scan() {
		err = conn.WriteMessage(websocket.TextMessage, []byte(scanner.Text()))
		if err != nil {
			log.Printf("log web socket write: %s", err)
			cmd.Process.Signal(os.Interrupt)
			break
		}
	}

	cmd.Wait()
}

var (
	sessM sync.Mutex
	sess  *session
)

func cmdHandler(w http.ResponseWriter, r *http.Request) {
	if r.Method != "POST" {
		http.Error(w, "only POST method supported", 404)
	}

	sessM.Lock()
	sessLoc := sess
	sessM.Unlock()

	if sessLoc == nil {
		http.Error(w, "no session", 500)
		return
	}

	_, err := io.Copy(sessLoc.pipeIn, r.Body)
	if err != nil {
		http.Error(w, err.Error(), 500)
		return
	}
	fmt.Fprint(w, "ok\n")
}

func startHandler(w http.ResponseWriter, r *http.Request) {
	var err error

	sessM.Lock()
	if sess != nil {
		select {
		case <-sess.exited:
			sess = nil
		default:
		}
	}
	if sess == nil {
		sess, err = newSession()
	}
	sessM.Unlock()
	if err != nil {
		http.Error(w, err.Error(), 500)
		return
	}
	fmt.Fprintln(w, "ok")
}

func upHandler(w http.ResponseWriter, r *http.Request) {
	var err error

	sessM.Lock()
	sessLoc := sess
	sessM.Unlock()

	if sessLoc == nil {
		http.Error(w, "no session", 500)
		return
	}

	sessLoc.ping()
	exited, err := sessLoc.waitWithTimeout(3 * time.Second)
	if exited {
		if err != nil {
			http.Error(w, err.Error(), 500)
		} else {
			http.Error(w, "", 500)
		}
	} else {
		fmt.Fprintln(w, "ok")
	}
}

func shutDownHandler(w http.ResponseWriter, r *http.Request) {
	sessM.Lock()
	if sess != nil {
		select {
		case <-sess.exited:
			sess = nil
		default:
		}
	}
	sessLoc := sess
	sessM.Unlock()

	if sessLoc != nil {
		sessLoc.pipeIn.Close()
	}

	fmt.Fprintln(w, "ok")
}

func main() {
	flag.Parse()

	http.Handle("/", http.FileServer(http.Dir(*staticPath)))
	http.HandleFunc("/log", logHandler)
	http.HandleFunc("/session/cmd", cmdHandler)
	http.HandleFunc("/session/up", upHandler)
	http.HandleFunc("/session/start", startHandler)
	http.HandleFunc("/session/shut-down", shutDownHandler)

	log.Fatal(http.ListenAndServe(*listenAddr, nil))
}

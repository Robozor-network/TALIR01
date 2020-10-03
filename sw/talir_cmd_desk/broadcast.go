package main

/*
import (
	"strings"
)

type lineBroadcast struct {
	inputch chan string
	subch chan chan<- string
	unsubch chan chan<- string
}

func NewLineBroadcast(history int) *lineBroadcast {
	lb := &lineBroadcast{
		inputch: make(chan string),
		subch: make(chan chan<- string),
		unsubch: make(chan chan<- string),
	}
	go lb.loop(history)
	return lb
}

func (lb *lineBroadcast) Input() chan<- string {
	return lb.inputch
}

func (lb *lineBroadcast) Sub(ch chan<- string) {
	lb.subch <- ch
}

func (lb *lineBroadcast) Unsub(ch chan<- string) {
	lb.unsubch <- ch
}

func lastN(arr []string, n int) []string {
	if len(arr) >= n {
		return arr[len(arr)-n:]
	} else {
		return arr
	}
}

func (lb *lineBroadcast) loop(historyLen int) {
	var history []string
	subs := make(map[chan<- string]bool)

loop:
	for {
		select {
		case line, ok := <-lb.inputch:
			if !ok {
				break loop
			}
			for ch, _ := range subs {
				select {
				case ch <- line:
				default:
				}
			}
			history = lastN(append(history, line), historyLen)
		case ch := <-lb.subch:
			subs[ch] = true
			select {
			case ch <- strings.Join(history, "\n"):
			default:
			}
		case ch := <-lb.unsubch:
			if subs[ch] {
				close(ch)
				delete(subs, ch)
			}
		}
	}
}
*/

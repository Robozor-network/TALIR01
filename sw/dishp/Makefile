dishp: src/main.cpp src/regulators.cpp
	g++ -o ./bin/dishp $^ -lpthread -lrt -lwiringPi -lwiringPiDev

test:  src/main.cpp src/regulators.cpp
	g++ -DNORPI -o ./bin/dishp $^ -lpthread -lrt

install:
	cp ./bin/dishp /usr/local/bin

copy:
	scp -r ./src Makefile pi@10.1.1.11:/home/pi/dishp

clean:
	rm -rf ./src/*.o
	rm -rf ./bin/dishp

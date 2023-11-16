SHELL=cmd.exe

sender: sender.o Terminal.o
	g++ sender.o Terminal.o -o sender -lwsock32 -fexec-charset=gbk

receiver: receiver.o Terminal.o
	g++ receiver.o Terminal.o -o receiver -lwsock32 -fexec-charset=gbk

sender.o: sender.cpp
	g++ -c sender.cpp -o sender.o -lwsock32 -fexec-charset=gbk

receiver.o: receiver.cpp
	g++ -c receiver.cpp -o receiver.o -lwsock32 -fexec-charset=gbk

Terminal.o: Terminal.cpp 
	g++ -c Terminal.cpp -o Terminal.o -lwsock32 -fexec-charset=gbk

run: receiver sender
	start cmd /k receiver & start cmd /k sender

runs: sender
	start cmd /k sender &

runr: receiver
	start cmd /k receiver &

.PHONY: run runr runs

all: 
	g++ -std=c++11 serverA.cpp -o serverA
	g++ -std=c++11 serverB.cpp -o serverB
	g++ -std=c++11 mainServer.cpp -o mainServer
	g++ -std=c++11 client.cpp -o client

.PHONY: serverA
serverA:
		./serverA

.PHONY: serverB
serverB:
		./serverB

.PHONY: mainServer
mainServer:
		./mainServer

.PHONY: client
client:
		./client

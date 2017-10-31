build:
	g++ -o biliquery -std=c++11 -O3 ConnHandler.cpp QTableMain.cpp Responder.cpp TCPServer.cpp -lev
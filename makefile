e: main.cpp Overlay.cpp persistence.pb.o
	g++-4.6 -std=c++0x main.cpp persistence.pb.o -g -o e `pkg-config --cflags --libs gtkmm-3.0  protobuf` -pthread

persistence.pb.o: persistence.pb.cc persistence.pb.h
	g++-4.6 -std=c++0x persistence.pb.cc -c

persistence.pb.cc: persistence.proto
	protoc persistence.proto --cpp_out=./

#flick -DSY_MENUBAR_ENABLED on this one for a menubar;
#flick -DNO_SYTEXTVIEW on this one to disable sytextview's pointer-cursor unification which is currently completely untuned;
e: main.cpp Overlay.cpp persistence.pb.o undoableTextView.o
	g++-4.6 -std=c++0x main.cpp persistence.pb.o undoableTextView.o -g -o e `pkg-config --cflags --libs gtkmm-3.0  protobuf` -pthread

persistence.pb.o: persistence.pb.cc persistence.pb.h
	g++-4.6 -std=c++0x persistence.pb.cc -c

undoableTextView.o: undoableTextView.cc undoableTextView.hh
	g++-4.6 undoableTextView.cc -c -g `pkg-config --cflags --libs gtkmm-3.0`

persistence.pb.cc: persistence.proto
	protoc persistence.proto --cpp_out=./

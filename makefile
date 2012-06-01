cppc = g++-4.7
debuggingOpt = -g 
travelToTheYearTwoThousand = -std=c++11

#flick -DNO_SYTEXTVIEW on this one to disable sytextview's pointer-cursor unification which is currently completely untuned;
e: main.cpp Overlay.cpp persistence.pb.o undoableTextView.o ActionLine.o actionSearchDB
	$(cppc) $(travelToTheYearTwoThousand) main.cpp persistence.pb.o undoableTextView.o $(debuggingOpt) -o e `pkg-config --cflags --libs gtkmm-3.0 protobuf` `xapian-config --libs --cxxflags` -pthread

undoableTextView.o: undoableTextView.cc undoableTextView.hh
	$(cppc) undoableTextView.cc -c $(debuggingOpt) `pkg-config --cflags --libs gtkmm-3.0`

ActionLine.o: ActionLine.cpp
	$(cppc) $(travelToTheYearTwoThousand) $(debuggingOpt) ActionLine.cpp -c  `pkg-config --cflags --libs gtkmm-3.0`


persistence.pb.o: persistence.pb.cc
	$(cppc) persistence.pb.cc -c

persistence.pb.cc: persistence.proto
	protoc persistence.proto --cpp_out=./


#conf.pb.o: conf.pb.cc
	#$(cppc) conf.pb.cc -c

#conf.pb.cc: conf.proto
	#protoc conf.proto --cpp_out=./


actionSearchDB: buildSearchDB.cpp dbDef.hpp
	$(cppc) $(travelToTheYearTwoThousand) buildSearchDB.cpp -o buildSearchDB `xapian-config --libs --cxxflags` && ./buildSearchDB actionSearchDB

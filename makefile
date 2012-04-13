e: main.cpp
	g++-4.6 -std=c++0x main.cpp -g -o e `pkg-config --cflags --libs gtkmm-3.0`

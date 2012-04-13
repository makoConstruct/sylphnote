#include <iostream>
#include <gtkmm.h>
#include <gdkmm.h>
#include "sharedlib/SyTextView.cpp"
#include "SyScrolledWindow.cpp"

using namespace Gtk;
using namespace std;

namespace appglobal{
	
}

int main(int argc, char** argv){
	Main main(argc, argv);
	auto buf = TextBuffer::create();
	auto window = new Window();
	window->set_default_size(800,400);
	auto view = new SyTextView(buf);
	auto syv = new SyScrolledWindow();
	view->set_wrap_mode(WRAP_WORD_CHAR);
	syv->set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	syv->add(*view);
	window->add(*syv);
	window->show_all();
	main.run(*window);
}

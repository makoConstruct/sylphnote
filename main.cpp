#include <iostream>
#include <gtkmm.h>
	#include <gtkmm/accelmap.h> //why must I do this!?
#include <gdkmm.h>
#include <sstream>
#include <string>
#include <fstream>
#include "SyTextView.cpp"
#include "SyScrolledWindow.cpp"
#include "Overlay.cpp"
#include "persistence.pb.h"
#ifndef NDEBUG
	#define warn(x) cerr<<"warning: "<<x<<'\n'
#else
	#define warn(x)
#endif

using namespace Gtk;
using namespace std;
using Glib::RefPtr;

class SyWindow;

namespace appglobal{
	const string appname = "sylphnote";
	const string memoryFilePath = "persistence";
	unsigned const defaultWindowWidth = 800;
	unsigned const defaultWindowHeight = 400;
	MakoApp::Persistence persistence;
	namespace icon{
		Cairo::RefPtr<Cairo::ImageSurface> save;
		Cairo::RefPtr<Cairo::ImageSurface> saveDifferent;
		Cairo::RefPtr<Cairo::ImageSurface> saveOver;
		Cairo::RefPtr<Cairo::ImageSurface> saveFail;
		Cairo::RefPtr<Cairo::ImageSurface> open;
		Cairo::RefPtr<Cairo::ImageSurface> toStdout;
		Cairo::RefPtr<Cairo::ImageSurface> saveWtf;
		Cairo::RefPtr<Cairo::ImageSurface> newFile;
		Cairo::RefPtr<Cairo::ImageSurface> fromStdin;
		Cairo::RefPtr<Cairo::ImageSurface> didNotSave;
	}
	bool fileHasChanged = false;
	bool fromStdin=false;
	bool toStdout=false;
	Overlay* ov;
	string sourceFileName;
	string destFileName; //a feature that probably wont get used by the user;
	ostream* out=NULL;
	RefPtr<TextBuffer> buff; //is here for saveOrWhatever and isItCoolToQuit;
	SyTextView* textview;
	SyWindow* window;
	Menu* menu;
}

bool isItCoolToQuit(){ //quits if there's no reason not to.
	return (appglobal::textview->isUnmodified()) || (appglobal::buff->get_char_count()==0);
}

void shortFlash(Cairo::RefPtr<Cairo::ImageSurface>& image){ //warning, depends on ov being initialized;
	appglobal::ov->flash(image, chrono::duration_cast<Overlay::Duration>(chrono::milliseconds(520)), chrono::duration_cast<Overlay::Duration>(chrono::milliseconds(600)));
}
void longishFlash(Cairo::RefPtr<Cairo::ImageSurface>& image){ //warning, depends on ov being initialized;
	appglobal::ov->flash(image, chrono::duration_cast<Overlay::Duration>(chrono::milliseconds(1200)), chrono::duration_cast<Overlay::Duration>(chrono::milliseconds(800)));
}
void longFlash(Cairo::RefPtr<Cairo::ImageSurface>& image){ //warning, depends on ov being initialized;
	appglobal::ov->flash(image, chrono::duration_cast<Overlay::Duration>(chrono::milliseconds(2000)), chrono::duration_cast<Overlay::Duration>(chrono::milliseconds(1000)));
}

class SyWindow : public Window{
protected:
	virtual void windowDestruct(){//here we quickly collect some data we need for the cleanup that happens in int main() after the gtk::main ceases;
		int x, y;
		get_size(x,y);
		MakoApp::Point* size = appglobal::persistence.mutable_lastsessionwindowshit()->mutable_size();
		size->set_x(x);  size->set_y(y);
	}
	virtual bool on_delete_event(GdkEventAny* event){
		if((isItCoolToQuit())){
			return false;
		}else{
			shortFlash(appglobal::icon::didNotSave);
			return true;
		}
	}
	virtual void on_hide(){
		windowDestruct();
	}
public:
	SyWindow():Window(){
		set_has_resize_grip(false);
	}
};

void quit(){
	appglobal::window->hide();
}

bool saveAsAction(string& data, Cairo::RefPtr<Cairo::ImageSurface>*& image){ //returns true iff the users follows through;
	using namespace appglobal;
	Gtk::FileChooserDialog dialog(*window, "specify an output file", FILE_CHOOSER_ACTION_SAVE);
	dialog.set_transient_for(*window);
	dialog.add_button(Stock::CANCEL, RESPONSE_CANCEL);
	dialog.add_button(Stock::SAVE, RESPONSE_ACCEPT);
	switch(dialog.run()){
	case(RESPONSE_ACCEPT):{
		string outFileName = dialog.get_filename();
		ofstream outFile(outFileName, ios_base::out|ios_base::trunc); //assumes the file is to be overwritten, that the user meant it;
		bool savedOver = outFile.is_open();
		outFile << data;
		if(outFile){
			destFileName = move(outFileName);
			image = (savedOver?
				&icon::saveOver:
				&icon::save);
		}else{ //something done went wrong;
			image = &icon::saveFail;
		}
	return true;}
	case(RESPONSE_DELETE_EVENT):
	case(RESPONSE_CANCEL):
	return false;
	#ifndef NDEBUG
	case(RESPONSE_CLOSE):
	case(RESPONSE_REJECT):
	case(RESPONSE_NONE):
	case(RESPONSE_OK):
	default:
		image = &icon::saveWtf; //I don't see how the requisite conditions can happen and if they do I want to know about it.
	return false;
	#endif
	}
}

void saveAs(){
	string data = appglobal::buff->get_text();
	Cairo::RefPtr<Cairo::ImageSurface>* image=NULL;
	bool didsave = saveAsAction(data, image);
	
	if(image) shortFlash(*image);
	if(appglobal::toStdout){
		cout << data;
		shortFlash(appglobal::icon::toStdout);
	}
	if(appglobal::toStdout || didsave)   appglobal::textview->setUnmodifiedNow();
}

void saveOrWhatever(){
	using namespace appglobal;
	Cairo::RefPtr<Cairo::ImageSurface>* image=NULL;
	string data = buff->get_text();
	if(!destFileName.empty()){
		ofstream outFile(destFileName, ios_base::out|ios_base::trunc);
		bool savedOver = outFile.is_open();
		outFile << data;
		image = &icon::save;
	}else{
		if(!sourceFileName.empty()){
			ofstream outFile(sourceFileName, ios_base::out|ios_base::trunc);
			outFile << data;
			image = &icon::save;
			textview->setUnmodifiedNow();
		}else if(!toStdout){//where do you want it to go, then?
			if(saveAsAction(data, image))   textview->setUnmodifiedNow();
		}
	}
	
	if(image) shortFlash(*image);
	if(toStdout){
		cout << data;
		shortFlash(icon::toStdout);
		textview->setUnmodifiedNow();
	}
}

sigc::signal<void> framePusher;
bool framePush(){ //replace this with a lambda when sigc++ gets support for them. Something this simple shouldn't be defined separate from its context;
	framePusher.emit();
	return true;
}

int main(int argc, char** argv){
	Gtk::Main mein(argc, argv);
	{
	unsigned wordi=0;
	while(wordi < argc-1){
		++wordi;
		if(argv[wordi][0] == '-'){
			char* flags = argv[wordi];
			unsigned i=1;
			char thisone;
			while((thisone = flags[i]) != '\0'){
				if(thisone == 'i'){
					appglobal::fromStdin=true;
				}else if(thisone == 'o'){
					appglobal::toStdout=true;
				}else if(thisone == 'h'){
					cout<<"usage: \n\t"<< appglobal::appname <<" [file] [destination file] , where options can be inserted anywhere.\noptions include:\n\t\'i\': read subject from standard input\n\t\'o\': put to the standard output on saving\nDestination file is <file> if a <destination file> isn't specified.";
				}
				++i;
			}
		}else{ //interpret as filename;
			if(!appglobal::sourceFileName.empty()){
				appglobal::destFileName = (string)argv[wordi];
			}else{
				appglobal::sourceFileName = (string)argv[wordi];
			}
		}
	}}
	
	//deserialize the persistece data;
	ifstream persistenceRead(appglobal::memoryFilePath, ios_base::in|ios_base::binary);
	if(!persistenceRead.is_open()){cerr<<"persistence file not found. WTF"<<endl; return -1;}
	if(!appglobal::persistence.ParseFromIstream(&persistenceRead)){
		cerr<<"failed in parsing my persistant memory. This can't be good. Report it, and send it in with your report. It's located at \'"<<appglobal::memoryFilePath<<'\''<<endl;
		return -1;
	}
	persistenceRead.close();
	
	Cairo::RefPtr<Cairo::ImageSurface> * openImage;
	appglobal::buff = TextBuffer::create();
	stringstream datacontents;
	bool isBacked = true;
	if(appglobal::fromStdin){
		datacontents << cin.rdbuf();
		isBacked=false;
		openImage = &appglobal::icon::fromStdin;
	}else{ //getting from a file;
		if(!appglobal::sourceFileName.empty()){ //using sourceFile
			datacontents << ifstream(appglobal::sourceFileName).rdbuf();
			openImage = &appglobal::icon::open;
		}else{//else leave datacontents blank, and hence the document, empty;
			openImage = &appglobal::icon::newFile;
		}
	}
	appglobal::buff->set_text(datacontents.str());
	
	SyWindow window;
	appglobal::window = &window;
	{ //setting windowshit from persistence;
		bool question = appglobal::persistence.has_lastsessionwindowshit();
		MakoApp::Persistence::LastSessionWindowShit* windowshit = appglobal::persistence.mutable_lastsessionwindowshit(); //if question was false, it would no longer be;
		if(!question){
			MakoApp::Point* size = windowshit->mutable_size();
			size->set_x(appglobal::defaultWindowWidth);
			size->set_y(appglobal::defaultWindowHeight);
			windowshit->set_wasmaximizedtall(false);
			windowshit->set_wasmaximizedwide(false);
			goto use_sizes; //---->
		}else{
			if(windowshit->wasmaximizedwide()){
				if(windowshit->wasmaximizedtall()){
					window.maximize();
				}else{
					//um... can't really do this...
					goto use_sizes; //---->
				}
			}else{
				if(windowshit->wasmaximizedtall()){
					//um...
					goto use_sizes; //---->
				}else{
					use_sizes:    //<----
					window.set_default_size(
						windowshit->size().x(),
						windowshit->size().y());
				}
			}
		}
	}
	window.set_title(appglobal::appname);
	
	appglobal::icon::save = Cairo::ImageSurface::create_from_png("saveIcon.png");
	appglobal::icon::saveFail = Cairo::ImageSurface::create_from_png("saveFail.png");
	appglobal::icon::saveOver = Cairo::ImageSurface::create_from_png("saveOver.png");
	appglobal::icon::saveDifferent = appglobal::icon::save;
	appglobal::icon::toStdout = Cairo::ImageSurface::create_from_png("toStdout.png");
	appglobal::icon::saveWtf = Cairo::ImageSurface::create_from_png("saveWtf.png");
	appglobal::icon::open = Cairo::ImageSurface::create_from_png("openIcon.png");
	appglobal::icon::newFile = Cairo::ImageSurface::create_from_png("newFile.png");
	appglobal::icon::fromStdin = Cairo::ImageSurface::create_from_png("fromStdin.png");
	appglobal::icon::didNotSave = Cairo::ImageSurface::create_from_png("didnotSaveIcon.png");
	
	auto accelGroup = AccelGroup::create();
	#ifdef SY_MENUBAR_ENABLED
	Menu subMenu; subMenu.set_take_focus(false); appglobal::menu = &subMenu;
	MenuItem mainM("_main",true);
	mainM.set_submenu(subMenu);
	#endif
	SyTextView textview(appglobal::buff); textview.set_wrap_mode(WRAP_WORD_CHAR);
	appglobal::textview = &textview;
	
		const string quitPath = "<main>/main/quit";
		auto quitAction = Action::create("quit", Stock::QUIT, "just _Quit"); quitAction->signal_activate().connect(sigc::ptr_fun(quit)); quitAction->set_accel_path(quitPath); quitAction->set_accel_group(accelGroup);
		Gtk::AccelMap::add_entry(quitPath, GDK_KEY_q, Gdk::CONTROL_MASK);
		quitAction->connect_accelerator();
		#ifdef SY_MENUBAR_ENABLED
		ImageMenuItem quitI(Stock::QUIT); quitI.set_accel_path(quitPath); quitI.set_related_action(quitAction);
		subMenu.append(quitI);
		#endif
		
		const string saveAsPath = "<main>/main/saveAs";
		auto saveAsAction = Action::create("save as", Stock::SAVE, "save _as"); saveAsAction->signal_activate().connect(sigc::ptr_fun(saveAs)); saveAsAction->set_accel_path(saveAsPath); saveAsAction->set_accel_group(accelGroup);
		Gtk::AccelMap::add_entry(saveAsPath, GDK_KEY_s, Gdk::CONTROL_MASK|Gdk::SHIFT_MASK);
		saveAsAction->connect_accelerator();
		#ifdef SY_MENUBAR_ENABLED
		ImageMenuItem saveAsI(Stock::SAVE_AS); saveAsI.set_accel_path(saveAsPath); saveAsI.set_related_action(saveAsAction);
		subMenu.append(saveAsI);
		#endif
		
		const string savePath = "<main>/main/save";
		auto saveAction = Action::create("save", Stock::SAVE, "_save"); saveAction->signal_activate().connect(sigc::ptr_fun(saveOrWhatever)); saveAction->set_accel_path(savePath); saveAction->set_accel_group(accelGroup);
		Gtk::AccelMap::add_entry(savePath, GDK_KEY_s, Gdk::CONTROL_MASK);
		saveAction->connect_accelerator();
		#ifdef SY_MENUBAR_ENABLED
		ImageMenuItem saveI(Stock::SAVE); saveI.set_accel_path(savePath); saveI.set_related_action(saveAction);
		subMenu.append(saveI);
		#endif
		
		const string undoPath = "<main>/main/undo";
		auto undoAction = Action::create("undo", Stock::UNDO, "_undo"); undoAction->signal_activate().connect(sigc::mem_fun(&textview, &view::UndoableTextView::Undo)); undoAction->set_accel_path(undoPath); undoAction->set_accel_group(accelGroup);
		Gtk::AccelMap::add_entry(undoPath, GDK_KEY_z, Gdk::CONTROL_MASK);
		undoAction->connect_accelerator();
		#ifdef SY_MENUBAR_ENABLED
		ImageMenuItem undoI(Stock::UNDO); undoI.set_accel_path(undoPath); undoI.set_related_action(undoAction);
		subMenu.append(undoI);
		#endif
		
		const string redoPath = "<main>/main/redo";
		auto redoAction = Action::create("redo", Stock::REDO, "_redo"); redoAction->signal_activate().connect(sigc::mem_fun(&textview, &view::UndoableTextView::Redo)); redoAction->set_accel_path(redoPath); redoAction->set_accel_group(accelGroup);
		Gtk::AccelMap::add_entry(redoPath, GDK_KEY_z, Gdk::CONTROL_MASK | Gdk::SHIFT_MASK);
		redoAction->connect_accelerator();
		#ifdef SY_MENUBAR_ENABLED
		ImageMenuItem redoI(Stock::REDO); redoI.set_accel_path(redoPath); redoI.set_related_action(redoAction);
		subMenu.append(redoI);
		#endif
	#ifdef SY_MENUBAR_ENABLED
	MenuBar menuBar; menuBar.append(mainM); menuBar.set_take_focus(false);
	#endif
	Box box; box.set_orientation(ORIENTATION_VERTICAL); box.set_homogeneous(false);// grid.set_resize_mode(RESIZE_PARENT);
	SyScrolledWindow syv; syv.set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	unsigned animationFrameTime = 16; //milliseconds;
	Glib::signal_timeout().connect(sigc::ptr_fun(framePush), animationFrameTime);
	Overlay ov(textview, framePusher, animationFrameTime); appglobal::ov = &ov;
	longFlash(*openImage);
	syv.add(textview);
	#ifdef SY_MENUBAR_ENABLED
	box.pack_start(menuBar,false,true);
	#endif
	box.pack_start(syv,true,true);
	window.add(box);
	window.add_accel_group(accelGroup);
	window.show_all();
	mein.run(window);
	
	//persisting;
	ofstream persistenceWrite(appglobal::memoryFilePath, ios_base::out|ios_base::trunc|ios_base::binary);
	if(!persistenceWrite.is_open()){cerr <<"could not open persistence file for persisting out. wtf" <<endl; return -1;}
	if(!appglobal::persistence.SerializeToOstream(&persistenceWrite)){
		cerr<< "was unable to save my persistent memory file to "<<appglobal::memoryFilePath<<". wtf."<< endl;
	}
	persistenceWrite.close();
	return 0;
}

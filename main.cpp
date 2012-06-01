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
#include "ActionLine.cpp"
#include "persistence.pb.h"
//#include        "conf.pb.h"
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
	const string configurationFilePath = "config";
	unsigned const defaultWindowWidth = 800;
	unsigned const defaultWindowHeight = 400;
	MakoApp::Persistence persistence;
	namespace icon{
		 Cairo::RefPtr<Cairo::ImageSurface> failFlash;
		Cairo::RefPtr<Cairo::ImageSurface> save;
		Cairo::RefPtr<Cairo::ImageSurface> dontBotherSave;
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
	bool fromStdin=false;
	bool toStdout=false;
	Overlay* ov;
	string sourceFileName;
	string destFileName; //a feature that probably wont get used by the user;
	ostream* out=NULL;
	RefPtr<TextBuffer> buff; //is here for saveOrWhatever and isItCoolToQuit;
	SyTextView* textView;
	SyWindow* window;
	Menu* menu;
}

bool isItCoolToQuit(){ //quits if there's no reason not to.
	return (appglobal::textView->isUnmodified()) || (appglobal::buff->get_char_count()==0);
}

void shortFlash(Cairo::RefPtr<Cairo::ImageSurface>& image){ //warning, depends on appglobal::ov being initialized;
	appglobal::ov->flash(image, chrono::duration_cast<Overlay::Duration>(chrono::milliseconds(520)), chrono::duration_cast<Overlay::Duration>(chrono::milliseconds(600)));
}
void longishFlash(Cairo::RefPtr<Cairo::ImageSurface>& image){ //warning, depends on appglobal::ov being initialized;
	appglobal::ov->flash(image, chrono::duration_cast<Overlay::Duration>(chrono::milliseconds(1200)), chrono::duration_cast<Overlay::Duration>(chrono::milliseconds(800)));
}
void longFlash(Cairo::RefPtr<Cairo::ImageSurface>& image){ //warning, depends on appglobal::ov being initialized;
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
	bool forcedQuit;
	void forbitQuitAgain(){
		forcedQuit=false;
	}
	virtual bool on_delete_event(GdkEventAny* event){
		if((isItCoolToQuit())){
			return false;
		}else{
			if(forcedQuit){
				return false;
			}else{
				forcedQuit=true;
				Glib::signal_timeout().connect_once(   sigc::mem_fun(this, &SyWindow::forbitQuitAgain),   3500);
				shortFlash(appglobal::icon::didNotSave);
				return true;
			}
		}
	}
	virtual void on_hide(){
		windowDestruct();
	}
public:
	SyWindow():Window(),forcedQuit(false){
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
	if(appglobal::toStdout || didsave)   appglobal::textView->setUnmodifiedNow();
}

void saveOrWhatever(){
	using namespace appglobal;
	Cairo::RefPtr<Cairo::ImageSurface>* image=NULL;
	if(textView->isUnmodified() && !sourceFileName.empty()){ //if there's no source file backing things the user will still expect you to put something there;
		image = & icon::dontBotherSave;
	}else{
		string data = buff->get_text();
		if(!destFileName.empty()){
			ofstream outFile(destFileName, ios_base::out|ios_base::trunc);
			outFile << data;
			image = &icon::save;
			textView->setUnmodifiedNow();
		}else if(!toStdout){ //where do you want it to go, then?
			if(saveAsAction(data, image))   textView->setUnmodifiedNow();
		}
	}
	
	if(image) shortFlash(*image);
	if(toStdout){
		cout << buff->get_text();
		shortFlash(icon::toStdout);
		textView->setUnmodifiedNow();
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
	if(!appglobal::sourceFileName.empty() && appglobal::destFileName.empty()) appglobal::destFileName = appglobal::sourceFileName; //save to destFileName only;
	
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
	
	{using namespace appglobal::icon;
	 failFlash = Cairo::ImageSurface::create_from_png("failFlash.png");
	save = Cairo::ImageSurface::create_from_png("saveIcon.png");
	saveFail = Cairo::ImageSurface::create_from_png("saveFail.png");
	saveOver = Cairo::ImageSurface::create_from_png("saveOver.png");
	dontBotherSave = save;
	saveDifferent = save;
	toStdout = Cairo::ImageSurface::create_from_png("toStdout.png");
	saveWtf = Cairo::ImageSurface::create_from_png("saveWtf.png");
	open = Cairo::ImageSurface::create_from_png("openIcon.png");
	newFile = Cairo::ImageSurface::create_from_png("newFile.png");
	fromStdin = Cairo::ImageSurface::create_from_png("fromStdin.png");
	didNotSave = Cairo::ImageSurface::create_from_png("didnotSaveIcon.png");
	}
	
	auto accelGroup = AccelGroup::create();
	unsigned animationFrameTime = 16; //milliseconds;
	SyTextView textView(appglobal::buff); textView.set_wrap_mode(WRAP_WORD_CHAR);
	appglobal::textView = &textView;
	Glib::signal_timeout().connect(sigc::ptr_fun(framePush), animationFrameTime);
	auto entryBuffer = EntryBuffer::create();
	ActionLine actionLine(entryBuffer, "actionSearchDB", framePusher, animationFrameTime, appglobal::icon::failFlash, &textView);

		const string toActionLinePath = "<main>/main/toActionLine";
		auto toActionLineAction = Action::create("shift focus to actionLine", Stock::OK, "toAction_Line");
		toActionLineAction->signal_activate().connect(sigc::mem_fun(&actionLine, &ActionLine::grab_focus));
		toActionLineAction->set_accel_path(toActionLinePath); toActionLineAction->set_accel_group(accelGroup);
		Gtk::AccelMap::add_entry(toActionLinePath, GDK_KEY_l, Gdk::MOD1_MASK);
		toActionLineAction->connect_accelerator();
		//unregistered;
		
		const string quitPath = "<main>/main/quit";
		auto quitAction = Action::create("quit", Stock::QUIT, "just _Quit");
		quitAction->signal_activate().connect(sigc::ptr_fun(quit));
		quitAction->set_accel_path(quitPath);
		quitAction->set_accel_group(accelGroup);
		Gtk::AccelMap::add_entry(quitPath, GDK_KEY_q, Gdk::CONTROL_MASK);
		quitAction->connect_accelerator();
		actionLine.actionsearcher.reg(AID::quit, quitAction);

		const string saveAsPath = "<main>/main/saveAs";
		auto saveAsAction = Action::create("save as", Stock::SAVE, "save _as");
		saveAsAction->signal_activate().connect(sigc::ptr_fun(saveAs));
		saveAsAction->set_accel_path(saveAsPath);
		saveAsAction->set_accel_group(accelGroup);
		Gtk::AccelMap::add_entry(saveAsPath, GDK_KEY_s, Gdk::CONTROL_MASK|Gdk::SHIFT_MASK);
		saveAsAction->connect_accelerator();
		actionLine.actionsearcher.reg(AID::saveAs, saveAsAction);

		const string savePath = "<main>/main/save";
		auto saveAction = Action::create("save", Stock::SAVE, "_save");
		saveAction->signal_activate().connect(sigc::ptr_fun(saveOrWhatever));
		saveAction->set_accel_path(savePath);
		saveAction->set_accel_group(accelGroup);
		Gtk::AccelMap::add_entry(savePath, GDK_KEY_s, Gdk::CONTROL_MASK);
		saveAction->connect_accelerator();
		actionLine.actionsearcher.reg(AID::save, saveAction);

		const string undoPath = "<main>/main/undo";
		auto undoAction = Action::create("undo", Stock::UNDO, "_undo");
		undoAction->signal_activate().connect(sigc::mem_fun(&textView, &view::UndoableTextView::Undo));
		undoAction->set_accel_path(undoPath);
		undoAction->set_accel_group(accelGroup);
		Gtk::AccelMap::add_entry(undoPath, GDK_KEY_z, Gdk::CONTROL_MASK);
		undoAction->connect_accelerator();
		actionLine.actionsearcher.reg(AID::undo, undoAction);

		const string redoPath = "<main>/main/redo";
		auto redoAction = Action::create("redo", Stock::REDO, "_redo");
		redoAction->signal_activate().connect(sigc::mem_fun(&textView, &view::UndoableTextView::Redo));
		redoAction->set_accel_path(redoPath);
		redoAction->set_accel_group(accelGroup);
		Gtk::AccelMap::add_entry(redoPath, GDK_KEY_z, Gdk::CONTROL_MASK | Gdk::SHIFT_MASK);
		redoAction->connect_accelerator();
		actionLine.actionsearcher.reg(AID::redo, redoAction);
	Box box; box.set_orientation(ORIENTATION_VERTICAL); box.set_homogeneous(false);// grid.set_resize_mode(RESIZE_PARENT);
	SyScrolledWindow syv; syv.set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	Overlay ov(textView, framePusher, animationFrameTime); appglobal::ov = &ov;
	longFlash(*openImage);
	syv.add(textView);
	box.pack_start(actionLine,false,true);
	box.pack_start(syv,true,true);
	window.add(box);
	window.add_accel_group(accelGroup);
	textView.grab_focus();
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

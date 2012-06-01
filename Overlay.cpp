#ifndef MGTK_OVERLAY
#define MGTK_OVERLAY

#include <gtkmm.h>
#include <queue>
#include <thread>
#include <atomic>

namespace{using namespace std; using namespace Gtk;}

class Overlay{ //flashes an image over the atached widget for a variable amount of time. Aspires to eventually be able to flash many things at once, allocating the area for this task dynamically;
public:
	typedef chrono::duration<unsigned,ratio<1,60>> Duration;
protected:
	typedef Cairo::RefPtr<Cairo::ImageSurface> ImageRef;
	struct ImageTab{
		ImageTab(ImageRef icon, Duration opaqueTime, Duration fadingTime):image(icon),duration(opaqueTime),fadeDuration(fadingTime){}
		ImageRef image;
		//Rectangle placement;
		Duration duration; //the amount of time it's opaque;
		Duration fadeDuration; //ideally this would be got dynamically from the framerate manager, changed according to how long the last frame took;
	};
	//Glib::RefPtr<Glib::TimeoutSource> framePusher; fuck this curmudgeounous bastard. I spit upon its memory;
	sigc::signal<void>& framePusher;
	sigc::connection pusherLink;
	float currentAlpha;
	std::queue<ImageTab*> tabs;
	unsigned frameTime; 
	unsigned currentOpaqueDuration, currentTotalDuration, currentLifetime, currentTransparentPeriod; //lifetime is the amount of ticks it's been up, the others are the number of ticks to endure before transitioning;
	ImageTab* currentTab;
	bool draw(Cairo::RefPtr<Cairo::Context> const & cr){
		if(pusherLink){ //otherwise there's no current imageTab
			using namespace Cairo;
			//if(currentAlpha!=1) cr->mask(SolidPattern::create_rgba(1,1,1, currentAlpha));
			double w=ward.get_allocation().get_width(),
			       h=ward.get_allocation().get_height();
			double iw = currentTab->image->get_width();
			double ih = currentTab->image->get_height();
			double x = w-gaps-iw,
			       y = h-gaps-ih;
			cr->set_source(currentTab->image, x,y);
			cr->rectangle(x,y,iw,ih);
			cr->paint_with_alpha(currentAlpha);
		}
		return false;
	}
	void reassignCurrentImageTabDealings(ImageTab* newCurrent){
		currentTab = newCurrent;
		currentAlpha = 1;
		currentOpaqueDuration = (unsigned)((currentTab->duration.count()*1000*Duration::period::num)/Duration::period::den);
		currentTotalDuration = (unsigned)(((currentTab->duration.count()+currentTab->fadeDuration.count())*1000*Duration::period::num)/Duration::period::den);
		currentTransparentPeriod = currentTotalDuration - currentOpaqueDuration;
		currentLifetime = 0;
	}
	void queueDraw(){
		auto alloc = ward.get_allocation();
		double w=alloc.get_width(),
					 h=alloc.get_height();
		double iw = currentTab->image->get_width();
		double ih = currentTab->image->get_height();
		ward.queue_draw_area(w-gaps-iw , h-gaps-ih , iw , ih);
	}
	void tick(){//determines the transparency to draw with and works through the queue;
		if(currentLifetime > currentTotalDuration){ //tab expired;
			delete tabs.front(); tabs.pop();
			if(!tabs.empty()){
				reassignCurrentImageTabDealings(tabs.front());
				queueDraw();
			}else{
				pusherLink.disconnect();
			}
		}else{
			if(currentLifetime > currentOpaqueDuration){
				currentAlpha = (float)(currentTransparentPeriod- (currentLifetime-currentOpaqueDuration)) /currentTransparentPeriod;
				queueDraw();
			}
			currentLifetime+=frameTime;
		}
	}
	void connectUp(){
		pusherLink = framePusher.connect(sigc::mem_fun(this, &Overlay::tick));
	}
public:
	unsigned gaps;
	Gtk::Widget& ward;
	Overlay(Widget& subject, sigc::signal<void>& frame_update_signal, unsigned frameLength, unsigned edge_gaps = 3)
		:ward(subject),gaps(edge_gaps),frameTime(frameLength),framePusher(frame_update_signal){ //frametime: length of a frame in milliseconds;
		ward.signal_draw().connect(sigc::mem_fun(this, &Overlay::draw));
	}
	~Overlay(){
		while(!tabs.empty()){delete tabs.front(); tabs.pop();}
	}
	bool flash(ImageRef image, Duration duration, Duration fadeDuration){ //returns true if doing now;
		ImageTab* newun = new ImageTab(image, duration, fadeDuration);
		tabs.push(newun);
		if(!pusherLink){
			reassignCurrentImageTabDealings(newun);
			connectUp();
			queueDraw();
			return true;
		}
		return false;
	}
};

#endif

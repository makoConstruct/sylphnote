#include <gtkmm.h>
#include <iostream>
#include <xapian.h>
#include <algorithm>
#include <cctype>
#include <cassert>
#include "dbDef.hpp"
#include "Overlay.cpp"

namespace{   using namespace std;
             using namespace Gtk;
             using namespace Xapian;
             using Glib::RefPtr;      }


class ActionDictionary{
protected:
	Xapian::Database xadb;
	vector<RefPtr<Action>> table;
	typedef tuple<string,unsigned,AID> Result;
public:
	ActionDictionary(string const& searchDB):xadb(searchDB){
		table.resize((unsigned)AID::end, RefPtr<Action>());
	}
	void reg(AID id, RefPtr<Action>& in){(table[(unsigned)id] = in)->set_name(AIDnames[(unsigned)id]);}
	vector<RefPtr<Action>*> matches(string const& query, unsigned nResults){ //return value may be larger than nResults;
		Enquire eq(xadb);
		TermIterator begin = xadb.allterms_begin(query);
		TermIterator end   =  xadb.allterms_end(query);
		vector<Result> results;
		for(; begin != end ; ++begin){
			Query q(*begin);
			eq.set_query(q);
			MSet mset = eq.get_mset(0,nResults); //and we know there's at least one in there because begin != end is established
			docid doc = *mset.begin();
			auto positionListBegin = xadb.positionlist_begin(doc, *begin);
			auto positionListEnd = xadb.positionlist_end(doc, *begin);
			unsigned position = (positionListBegin != positionListEnd)? *positionListBegin : 0;
			AID actionId = *(AID*)(mset.begin().get_document().get_value(0).data());
			Result result = Result(*begin,  position, actionId);
			results.push_back(result);
		}
		sort(results.begin(), results.end(),  [&](Result const& l, Result const& r)->bool{
			return (get<0>(l).size() == query.size())? //then this is probably what the user meant precisely, and should be given max precedence;
				true:
				(get<1>(l) < get<1>(r));
		});
		set<AID> isIn; //now remove unneeded repetitions;
		vector<RefPtr<Action>*> out;
		auto it = results.begin();
		while(it != results.end()){
			auto adding = &table[(unsigned)get<2>(*it)];
			if(!*adding) adding = NULL;
			out.push_back(adding);
			isIn.insert(get<2>(*it));
			++it;
			while(it != results.end() && isIn.count(get<2>(*it))){
				++it;
			}
		}
		return out;
	}
};

class ActionLine: public Entry{
protected:
	RefPtr<Action>* current;
	Widget* focusRecipient;
	Cairo::RefPtr<Cairo::ImageSurface> failFlash;
	Overlay ov;
	virtual void on_changed(){
		//ok, the notion of actually threading off the search strikes me as mad with a dataset of 7 items. But if I had a larger set it should be noted that I should do that here.
		vector<RefPtr<Action>*> results = actionsearcher.matches((string)get_text(), (unsigned)8);
		current = (results.empty()? (NULL) : (results.front()));
		/*for_each(results.begin(), results.end(), [](RefPtr<Action>* in){
			cout<<(in?(*in)->get_name():"not registered")<<'\n';
		});
		cout<<flush;*/
	}
	virtual void on_activate(){
		if(current){
			(*current)->activate();
			if(focusRecipient){
				set_text("");
				focusRecipient->grab_focus();
			}
		}
		else{ov.flash(failFlash, chrono::duration_cast<Overlay::Duration>(chrono::milliseconds(1200)), chrono::duration_cast<Overlay::Duration>(chrono::milliseconds(800)));}
	}
	virtual bool on_key_press_event(GdkEventKey* event){
		if(event->type == GDK_KEY_PRESS && event->keyval == GDK_KEY_Escape && focusRecipient) focusRecipient->grab_focus();
		return Entry::on_key_press_event(event);
	}
public:
	ActionDictionary actionsearcher;
	ActionLine(
		const Glib::RefPtr<EntryBuffer>& buffer,
		string const& db,
		sigc::signal<void>& framepush, 
		unsigned frametime, //frametime is in milliseconds;
		Cairo::RefPtr<Cairo::ImageSurface> failflash,
		Widget* focusTarget = NULL) //what do I hand focus to after activation or escape;
		:Entry(buffer),actionsearcher(db),current(NULL),ov(*this, framepush, frametime, 1),failFlash(failflash),focusRecipient(focusTarget){
		set_placeholder_text("press Alt+l to begin issuing a command");
	}
};

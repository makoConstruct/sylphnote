#ifndef ACTIONLINE
#define ACTIONLINE

#include <gtkmm.h>
#include <iostream>
#include <cctype>
#include "actionIDEnum.h"

class ActionDictionary{
public:
	ActionDictionary(Database searchDB);
	void reg(AID id, Action& in);
	typedef tuple<string,unsigned,AID> Result;
	vector<Result> matches(string query, unsigned nResults);
};

class ActionLine: public Entry{
public:
	set_actiondb(ActionDictionary* db);
	ActionDictionary* get_actiondb();
	ActionLine(const Glib::RefPtr<EntryBuffer>& buffer, string const& db);
};

#endif

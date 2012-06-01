#include <xapian.h>
#include <iostream>
#include <vector>
#include <cctype>
#include <algorithm>
#include "dbDef.hpp"

namespace{ using namespace std;
           using namespace Xapian; }

int main(int argc, char** argv){
	if(argc<2){
		cerr<<"buildSearchDB were misused"<<endl;
		return 1;
	}
	WritableDatabase db(string(argv[1]), DB_CREATE_OR_OVERWRITE);
	auto matchit = matchSynonyms.begin();
	AID id = (AID)0;
	while(matchit < matchSynonyms.end()){
		Document doc;
		string value((char*)&id,sizeof(AID));
		doc.add_value(0,value);
		termpos pos=0;
		for_each(matchit->begin(), matchit->end(), [&](string const& in){
			doc.add_term(in, pos);
			db.add_spelling(in);
			++pos;
		});
		db.add_document(doc);
		++matchit, id=(AID)((unsigned)id+1);
	}
	return 0;
}

#ifndef ACTIONIDENUM
#define ACTIONIDENUM

#include <vector>
#include <array>
#include <string>

namespace{ using namespace std; }

enum class AID: unsigned{
	save,
	saveAs,
	quit,
	open,
	help,
	revert,
	undo,
	redo,
	end
};

array<char const *, (unsigned)AID::end> const AIDnames = {
	"store to file",
	"select new storage file and store",
	"quit now",
	"pick an existing file to edit",
	"explain",
	"reload file from disk, discarding changes",
	"undo operation",
	"redo operation"
};

array<vector<char const *> const, (unsigned)AID::end> const matchSynonyms = {{
{"save", "save to disk", "write", "write to disk", "store", "stasis"},
{"save as", "save to another file", "write to another file", "write to"},
{"quit", "just quit", "discard work", "exit"},
{"open", "open a file", "read from disk", "restore a file"},
{"help", "explain", "what"},
{"revert file state", "reload from disk"},
{"undo", "reverse operation"},
{"redo", "unundo", "unreverse operation"}
  }};

//static_assert(((unsigned)AID::end == AIDnames.size()) && (AIDnames.size() == matchSynonyms.size()), "AIDnames and matchSynonyms must contain an entry for each AID and no more");   //this absolutely cannot be made to work.

#endif

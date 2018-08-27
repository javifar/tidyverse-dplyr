#ifndef dplyr_tools_SymbolMap_h
#define dplyr_tools_SymbolMap_h

#include <tools/hash.h>
#include <tools/match.h>
#include <tools/SymbolString.h>
#include <tools/SymbolVector.h>

namespace dplyr {

enum Origin { HASH, RMATCH, NEW };

struct SymbolMapIndex {
  int pos;
  Origin origin;

  SymbolMapIndex(int pos_, Origin origin_) :
    pos(pos_), origin(origin_)
  {}
};

class SymbolMap {
private:
  dplyr_hash_map<SEXP, int> lookup;
  SymbolVector names;

public:
  SymbolMap(): lookup(), names() {}

  SymbolMap(int n, const CharacterVector& names_): lookup(n), names((SEXP)names_) {
    for (int i = 0; i < n; i++) {
      lookup.insert(std::make_pair(names_[i], i));
    }
  }

  SymbolMap(const SymbolVector& names_): lookup(names_.size()), names(names_) {
    int n = names.size();
    for (int i = 0; i < n; i++) {
      lookup.insert(std::make_pair(names_[i].get_sexp(), i));
    }
  }

  SymbolMapIndex insert(const SymbolString& name) {
    // first, lookup the map
    dplyr_hash_map<SEXP, int>::const_iterator it = lookup.find(name.get_sexp());
    if (it != lookup.end()) {
      return SymbolMapIndex(it->second, HASH);
    }

    int idx = names.match(name);
    if (idx != NA_INTEGER) {
      // if it is in the names, insert it in the map with the right index
      lookup.insert(std::make_pair(name.get_sexp(), idx - 1));
      return SymbolMapIndex(idx - 1, RMATCH);
    } else {
      // otherwise insert it at the back
      idx = names.size();
      lookup.insert(std::make_pair(name.get_sexp(), idx));
      names.push_back(name.get_string());
      return SymbolMapIndex(idx, NEW);
    }
  }

  SymbolVector get_names() const {
    return names;
  }

  SymbolString get_name(const int i) const {
    return names[i];
  }

  int size() const {
    return names.size();
  }

  bool has(const SymbolString& name) const {
    return lookup.find(name.get_sexp()) != lookup.end();
  }

  int get(const SymbolString& name) const {
    // first, lookup the map
    dplyr_hash_map<SEXP, int>::const_iterator it = lookup.find(name.get_sexp());
    if (it == lookup.end()) {
      stop("variable '%s' not found", name.get_utf8_cstring());
    }
    return it->second;
  }

  SymbolMapIndex rm(const SymbolString& name) {

    dplyr_hash_map<SEXP, int>::const_iterator it = lookup.find(name.get_sexp());
    if (it != lookup.end()) {
      int idx = it->second;
      names.remove(idx);

      for (dplyr_hash_map<SEXP, int>::iterator it = lookup.begin(); it != lookup.end();) {
        int k = it->second;

        if (k < idx) {
          // nothing to do in that case
          ++it;
          continue;
        } else if (k == idx) {
          // need to remove the data from the hash table
          it = lookup.erase(it);
          continue;
        } else {
          // decrement the index
          it->second--;
          ++it;
        }
      }
      return SymbolMapIndex(idx, HASH);
    }

    return SymbolMapIndex(names.size(), NEW);
  }


};

}

#endif

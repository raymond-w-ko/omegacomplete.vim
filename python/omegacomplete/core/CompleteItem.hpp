#pragma once

struct CompleteItem {
 public:
  float Score;

  // VIM specific portion

  // word that will be inserted
  String Word;

  // how it will look like
  /* String Abbr; */

  // extra text after Word or Abbr
  String Menu;

  // displayed in Preview Window
  // String Info;
  // single letter to indicate type of completion
  /*
     The "kind" item uses a single letter to indicate the kind of completion.
     This may be used to show the completion differently (different color or
     icon).  Currently these types can be used:
     v	variable
     f	function or method
     m	member of a struct or class
     t	typedef
     d	#define or macro
     */
  // String Kind;

 private:
  // when non-zero case is to be ignored when comparing items to be equal;
  // when omitted zero is used, thus items that only differ in case are added
  // bool Icase;

  // when non-zero this match will be added even when an item with the same
  // word is already present.
  bool Dup;

  // when non-zero this match will be added even when it is an empty string
  // bool Empty;

 public:
  CompleteItem()
      : Score(0),
        // Icase(false),
        Dup(true)
  // Empty(false)
  {}

  CompleteItem(const std::string& word)
      : Score(0),
        Word(word),
        // Icase(false),
        Dup(true)
  // Empty(false)
  {}

  CompleteItem(const std::string& word, float score)
      : Score(score),
        Word(word),
        // Icase(false),
        Dup(true)
  // Empty(false)
  {}

  CompleteItem(const CompleteItem& other)
      : Score(other.Score),
        Word(other.Word),
        /* Abbr(other.Abbr), */
        Menu(other.Menu),
        // Info(other.Info),
        // Kind(other.Kind),
        // Icase(other.Icase),
        Dup(other.Dup)
  // Empty(other.Empty)
  {}

  bool operator<(const CompleteItem& other) const {
    if (Score > other.Score)
      return true;
    else if (Score < other.Score)
      return false;

    int result;

    result = Word.compare(other.Word);
    if (result < 0)
      return true;
    else if (result > 0)
      return false;

    /* result = Abbr.compare(other.Abbr); */
    /* if (result < 0) */
    /*   return true; */
    /* else if (result > 0) */
    /*   return false; */

    result = Menu.compare(other.Menu);
    if (result < 0)
      return true;
    else if (result > 0)
      return false;

    return false;
  }

  bool operator==(const CompleteItem& other) const {
    return Score == other.Score && Word == other.Word && Menu == other.Menu;
  }

  std::string SerializeToVimDict(size_t index) const {
    if (Word.empty()) return "";

    std::string result;
    result += "{";

    result += boost::str(boost::format("'word':'%s',") % Word);
    result += boost::str(boost::format("'abbr':'%d %s',") % (index + 1) % Word);

    /* if (!Abbr.empty()) */
    /*   result += boost::str(boost::format("'abbr':'%s',") % Abbr); */
    /* if (!Menu.empty()) */
    /*   result += boost::str(boost::format("'menu':'%s',") % Menu); */
    // if (!Info.empty())
    // result += boost::str(boost::format("'info':'%s',") % Info);
    // if (!Kind.empty())
    // result += boost::str(boost::format("'kind':'%s',") % Kind);

    // if (Icase)
    // result += "'icase':1,";
    if (Dup) result += "'dup':1,";
    // if (Empty)
    // result += "'empty':1,";

    result += "},";

    return result;
  }
};

typedef std::vector<CompleteItem> CompleteItemVector;
typedef boost::shared_ptr<CompleteItemVector> CompleteItemVectorPtr;

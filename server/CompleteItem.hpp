#pragma once

struct CompleteItem
{
    // word that will be inserted
    String Word;
    // how it will look like
    String Abbr;
    // extra text after Word or Abbr
    String Menu;
    // displayed in Preview Window
    String Info;
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
    String Kind;
    // when non-zero case is to be ignored when comparing items to be equal;
    // when omitted zero is used, thus items that only differ in case are added
    bool Icase;
    // when non-zero this match will be added even when an item with the same
    // word is already present.
    bool Dup;
    // when non-zero this match will be added even when it is an empty string
    bool Empty;

    CompleteItem()
    :
    Icase(false),
    Dup(false),
    Empty(false)
    {
        ;
    }

    bool operator<(const CompleteItem& other) const
    {
        return this->Word < other.Word;
    }
};


#pragma once

struct CompleteItem
{
    unsigned Weight;

    // VIM specific portion

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
    Weight(0),
    Icase(false),
    Dup(false),
    Empty(false)
    {
        ;
    }

    CompleteItem(const std::string& word)
    :
    Weight(0),
    Word(word),
    Icase(false),
    Dup(false),
    Empty(false)
    {
        ;
    }

    CompleteItem(const std::string& word, unsigned weight)
    :
    Weight(weight),
    Word(word),
    Icase(false),
    Dup(false),
    Empty(false)
    {
        ;
    }

    CompleteItem(const CompleteItem& other)
    :
    Weight(other.Weight),
    Word(other.Word),
    Abbr(other.Abbr),
    Menu(other.Menu),
    Info(other.Info),
    Kind(other.Kind),
    Icase(other.Icase),
    Dup(other.Dup),
    Empty(other.Empty)
    {
    }

    bool operator<(const CompleteItem& other) const
    {
        if (Weight < other.Weight)
            return true;
        else if (Weight > other.Weight)
            return false;
        else
            return Word < other.Word;
    }

    bool operator==(const CompleteItem& other) const
    {
        return (Weight == other.Weight) &&
               (Word == other.Word);
    }

    std::string SerializeToVimDict() const
    {
        if (Word.empty())
            return "";

        std::string result;
        result += "{";

        result += boost::str(boost::format("'word':'%s',") % Word);

        if (!Abbr.empty())
            result += boost::str(boost::format("'abbr':'%s',") % Abbr);
        if (!Menu.empty())
            result += boost::str(boost::format("'menu':'%s',") % Menu);
        if (!Info.empty())
            result += boost::str(boost::format("'info':'%s',") % Info);
        if (!Kind.empty())
            result += boost::str(boost::format("'kind':'%s',") % Kind);

        if (Icase)
            result += "'icase':1,";
        if (Dup)
            result += "'dup':1,";
        if (Empty)
            result += "'empty':1,";

        result += "},";

        return result;
    }
};


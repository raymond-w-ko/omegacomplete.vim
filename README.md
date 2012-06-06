# OmegaComplete

## General Information
This is my crazy sideproject which attempts to provide the fastest and most
useful keyword completions for VIM.

To accomplish this, the VIM portion of OmegaComplete talks to a server component
coded in C++03 via sockets (for performance reasons), and uses VIM Python bindings
to process buffers and send them to the server for synchronization. Issuing commands
to the server is also done in Python.

Processing of buffers and keyword extraction is done asynchronously,
so results might lag behind a bit. But because of this, editing large files is not a problem
and does not introduce input lag when in insert mode.

The only bottleneck is in the Python code where the buffer is joined into one huge string before sending to the server.
For large ( > 2MB ) files, there might be pauses in insert mode when you type too fast.
If anyone has a better solution, please, please let me know. For now I am not treating it
as a problem, because you are screwed if you have to maintain source code files that big.

## Prerequisites for Use
Since this uses C++11, you need a recent compiler and also the compiled portion
of the Boost C++ libraries. Your version of VIM also needs to have Python 2.X
bindings.

To compile the server portion you need a relatively standards compliant C++03 compiler
along with the compiled Boost C++ libraries for boost::asio. Your version of VIM also needs
to have the Python 2.X bindings.

## Completion Types

### Prefix Completion
Standard completion provided by VIM when you press Ctrl-N / Ctrl-P


### Title Case Completion
If have a word like "MyAwesomeVariable", then typing "mav" would offer this
completion first before prefix completion.

### Underscore Completion
Similar to the above, if you have "foo\_bar\_fizz\_buzz", the typeing "fbfb" would
offer this completion first before prefix completion.

### Levenshtein Distance <= X
Completion Calculates the levenshtein distance of
the current word against all known words in all buffers.  If the distance is
less than or equal to X then, offers it as completions.  Right now this only
triggers if words are longer than 3 characters and the distance <= 2.

## Random Ramblings
Apparently freeing structures (such as the TrieNode) is so expensive (taking
100 ms out of 300ms blocking operation on my machine) that I have to have
a separate thread acting as a "garbage collector" and free there. I guess Herb
Sutter was right when he said that you sometimes need a real garbage collector
for absolute performant C++ code.

Building a trie of all words in a buffer can get expensive. Similarly, building
all abbreviations for all words is also expensive. It can probably be less
expensive if we don't want words to be kept in sync but accuracy is probably
more important.

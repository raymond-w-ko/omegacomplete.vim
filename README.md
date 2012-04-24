# OmegaComplete

This is my crazy sideproject which attempts to provide the fastest and most
useful keyword completions for VIM.

To accomplish this it talks to a server component coded in C++11 for maximum
speed, and uses python glue code to send the contents of the current buffer to the
server component for synchronization.

Since this uses C++11, you need a recent compiler and also the compiled portion
of the Boost C++ libraries. Your version of VIM also needs to have Python 2.X bindings.

## Completion Types

### Prefix Completion
Standard completion provided by VIM when you press Ctrl-N / Ctrl-P

### Levenshtein Distance <= X Completion
Calculates the levenshtein distance of the current word against all known words in all buffers.
If the distance is less than or equal to X then, offers it as completions.
Right now this only triggers if words are longer than 3 characters and the distance <= 2.

### Title Case Completion
If have a word like "MyAwesomeVariable", then typing "mav" would offer this completion first before prefix completion.

### Underscore Completion
Similar to the above, if you have "foo_bar_fizz_buzz", the typeing "fbfb" would offer this completion first before prefix completion.

## Random Ramblings
Apparently freeing structures (such as the TrieNode) is so expensive (taking 100 ms out of 300ms blocking operation on my machine) that I have to have a separate thread acting as a "garbage collector" and free there. I guess Herb Sutter was right when he said that you sometimes need a real garbage collector for absolute performant code.

Building a trie of all words in a buffer can get expensive. Similarly, building all abbreviations for all words is also expensive. It can probably be less expensive if we don't want words to be kept in sync but accuracy is probably more important.
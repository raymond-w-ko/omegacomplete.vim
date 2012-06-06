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
as a problem, because you are screwed anyways if you have to maintain source code files that big.

## Prerequisites for Use
To compile the server portion you need a relatively standards compliant C++03 compiler
along with the compiled Boost C++ libraries for boost::asio. Your version of VIM also needs
to have the Python 2.X bindings.

If you use Visual Studio 2008 and have Boost compiled and installed in "C:\Boost", then you
can just use with the solution file provided.

## Completion Types

### Prefix Completion
Standard completion provided by VIM when you press Ctrl-N / Ctrl-P in insert mode.

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

## Other features

### Exuberant ctags Completion
Understands what 'tags' is current set to in VIM and can use the list of tags
found in those files to offer as a completion candidate. The tag files are
cached in memory, so OmegaComplete does not do any file accesses after the
initial parse. When it detects that the timestamp has changed, then it
automatically tries to reparse.

# OmegaComplete

## General Information
This is my crazy side project which attempts to provide the fastest and most useful keyword completions for VIM.

To accomplish this, OmegaComplete uses a Python extension module coded in C++03 to do the
actual parsing and completions. Tiny bits of Python are used to "glue" VIM and the C++ module together.

Processing of buffers, which include keyword extraction, abbreviation calculation,
and maintaining a trie of all words, is done asynchronously,
so results might lag behind a tiny bit. But because of this, editing large files is not a problem
and does not introduce input lag when in insert mode.

The only bottleneck is in the Python code where the buffer is joined into one huge string before sending
to the C++ module. For large (greater than 5MB) files, there might be pauses in insert mode when you type too fast.
If anyone has a better solution, please, please let me know. For now I am not treating it
as a problem, because you are screwed anyways if you have to maintain source code files that big.

## Prerequisites for Use and Installation
To compile the server portion you need a relatively standards compliant C++03 compiler
along with the compiled Boost C++ libraries for boost threads. Your version of VIM also needs
to have the Python 2.X bindings.

### Windows

If you have Visual Studio 2012 and have Boost installed in "C:\boost",
and the libraries installed in "C:\boost\stage\lib32" or "C:\boost\stage\lib64",
then you can use with the solution file provided.

With other version of Visual Studio, it should be fairly simple to setup your project file,
just create project for a DLL and have a post-build event to copy the DLL to
python/omegacomplete/omegacomplete/core.pyd

Let me know if you need any help.

### Linux / Mac OS X / Cygwin / UNIX

#### Summary
<pre>
cd python/omegacomplete/core
make
</pre>

You can optionally strip the executable to save space (8MB -> 300 KB in Cygwin!)

The Makefile basically just calls setup.py, which uses Python distutils to compile with the appropriate options.

## Important Tip
You really should map 'Tab' (or favorite equivalent completion key) to this.
Otherwise, this plugin becomes somewhat useless. I didn't map this by default because I don't want
to trample anyone's keybinds.

Example of what to place in your vimrc:
<pre>
inoremap &lt;expr&gt; &lt;Tab&gt; pumvisible() ? "\&lt;C-n&gt;\&lt;C-y&gt;" : "\&lt;Tab&gt;"
</pre>

## Completion Types

### Prefix Completion
Standard completion provided by VIM when you press Ctrl-N / Ctrl-P in insert mode.

### Title Case Completion
If have a word like "MyAwesomeVariable", then typing "mav" would offer this
completion first before prefix completion.

### Underscore Completion
Similar to the above, if you have "foo\_bar\_fizz\_buzz", the typeing "fbfb" would
offer this completion first before prefix completion.

### Hyphen Completion
This is really helpful for Lisp or CSS which uses the hyphen key ("-") as a separator or "space" between words.
For example, if you have a Lisp function like "(defun list-matching-lines)", "lml" would produce the function name.
As of now this is always on, so I hope you don't do subtraction in other infix languages (like C) without
having a space between the operator ("vector1-vector2" vs "vector1 - vector2"), otherwise there will be false
positives.

### Sematic Completion (of the above two completions)
There is no good way to briefly explain this in a few words, so you will have to read
through the example I have.

First is the concept of "index points." For instance, in Title Case Completion,
"MyAwesomeVariable"'s "index points" are the capital letters and first letter,
namely "M", "A", "V". In the case of Underscore Completion "foo\_bar\_fizz\_buzz"'s
"index points" are the the first letter and the letters following the underscores.
So, they would be "f", "b", "f", "b".

Second, we can assign a "depth" to these index points, which just means how many letters
(including the "index point") to consider.

We now consider "MyAwesomeVariable" with a depth of 3. That means we work with the following chunks:
"My", "Awe", and "Var". For each chunk, we generate a set of all possible prefixes:

<pre>
My -> m, my

Awe -> a, aw, awe

Var -> v, va, var
</pre>

We then create all possible combinations of choosing a prefix from each set and appending them
together inorder. Typing any of these combinations will offer a completion for the original word.
In essence, any of the following abbreviations will offer a match to "MyAwesomeVariable":

"mav", "mava", "mavar", "mawv", "mawva", "mawvar", "mawev", "maweva", "mawevar", ...

The whole point of this is to reduce the number of false positives for Title Case and Underscore Completions
with only two index points, as they eventually become ambiguous given enough buffers open. In fact, if the
"depth" is set to 1, it degenerates to the normal Title Case or Underscore Completion algorithm.

Right now it only generate the semantic abbreviations if the number of "index points" is less than 5.
The "depth" is also capped at 3. The reason for this is that the number of possible abbreviations per
word is "depth" to the power of the number of "index points", which can consum a lot of memory quickly.

***
### Disambiguate Mode
As you may have noticed, completions in general have a number after them in the VIM popup menu.
This is so that you may append that number and cause that to be the only match that comes up.
In the cases where there are 10 or so ambiguous entries, this is useful because you don't have to use
Ctrl-N or Ctrl-P to select that entry.

#### Example of completions offered
<pre>
GetCode GetChar GarbageCollect
gc|
GarbageCollect  1
GetChar         2
GetCode         3
</pre>

#### Disambiguate Mode triggered by '2'
<pre>
GetCode GetChar GarbageCollect
gc2|
GetChar  2
</pre>

***

### Underscore Terminus Mode
This is really for Google style C++ (http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml)
where member variables end in an underscore. From my experience,
when creating constructors or false positives are triggered, sometimes you want the completions that ends with
an underscore. This means playing around with Ctrl-N or Ctrl-P, which is not that efficient. Now with this mode, when
your input word ends in an underscore, the completions are filtered so that only those ending with
an underscore are offered. OmegaComplete automatically falls back to the above completions if it finds no
words ending with an underscore.

#### Example of completions offered
<pre>
camera_list_ camera_list CommonLisp
cl|
CommonLisp   1
camera_list  2
camera_list_ 3
</pre>

#### Disambiguate Mode triggered by 'S'
<pre>
camera_list_ camera_list CommonLisp
cl_|
camera_list_ 1
</pre>
***

### Levenshtein Distance Correction Completion
OmegaComplete calculates the Levenshtein distance of the current word against all known words in all buffers.
If the Levenshtein distance between the current word and a candidate word is less than or equal to X,
it offers the word as a correction candidate.  Right now this only
triggers if words are longer than 3 characters, the distance is <= 2 to preserve speed.
As an additional condition, this completion is only triggered when it finds no other completions,
as this usually means that a typo has occurred.

You can also set the colorscheme of the VIM popup menu to be different when this type of completion
pops up. I find it very useful to have the color change so you know whether OmegaComplete is offering completions
or corrections.

## Other features

### Exuberant ctags Completion
Understands what 'tags' is current set to in VIM and can use the list of tags
found in those files to offer as a completion candidate. The tag files are
cached in memory, so OmegaComplete does not do any file accesses after the
initial parse. When it detects that the timestamp has changed, then it
automatically tries to reparse.

Note that this is a blocking operation, so expect slight freezes when OmegaComplete
is parsing tags files.

### VIM taglist("^keyword$") Function Replacement and Enhancement
As a natural result of cached tags, OmegaComplete offers a function to lookup
tag information in logarithmic time (cache is implemented as a std::multimap).

The VIM dict returned by this function also has a "prefix" key-value pair added, which is the text
that prefixes the keyword in the Ex command needed to find it. This is useful
for extracting return types from C++ function definitions, as ctags does
not natively offer it. See my vimrc repository on how I use it for an example.

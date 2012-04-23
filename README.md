# OmegaComplete

This is my crazy sideproject which attempts to provide the fastest and most
useful keyword completions for VIM.

To accomplish this it talks to a server component coded in C++ for maximum
speed, uses python glue code to send the contents of the current buffer to the
server component for synchronization.

## Current Completion Types

### Prefix Completion (Implemented)
Standard completion provided by VIM when you press Ctrl-N / Ctrl-P

### Levenshtein Distance <= X Completion (Implemented)
Calculates the levenshtein distance of the current word against all known words in all buffers.
If the distance is less than or equal to X then, offers it as completions.
Right now this only triggers if words are longer than 3 characters and the distance <= 2.

### Title Case Completion (TODO)
If have a word like "MyAwesomeVariable", then typing "mav" would offer this completion first before prefix completion.

### Underscore Completion (TODO)
Similar to the above, if you have "foo_bar_fizz_buzz", the typeing "fbfb" would offer this completion first before prefix completion.

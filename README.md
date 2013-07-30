YABI 
=======
(Yet Another Brainfuck Interpreter)

Yabi is a brainfuck interpreter and debugger, which aims to provide a versatile, full-featured set of tools for writing/debugging brainfuck.
Still very much a work in progress, so it's a bit rough.

Features:
- - - - - - -
Interpreter:

- set files other than stdin/stdout for input and output
- tunable memory size

Debugger:

- Interactive, somewhat gdb-like
- breakpoints
- memory/code peeking and poking
- stepping
- runtime patching via breakpoint hooks
- variables
- basic scripts

- - - - - - -

To run yabi:

    make
    ./yabi -f [file]		# standard interpreter
    ./yabi -f [file] -d		# to run with debugger attached

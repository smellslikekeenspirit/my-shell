# my-shell
a simple interactive shell capable of executing simple UNIX® commands and some internal commands

To compile:

`gcc -o mysh mysh.c`

To run in verbose mode:

`./mysh -v`

To make it remember n number of commands it has executed:

`./mysh -h[n]`

Both verbose mode and history argument can be given, in no order, i.e.

`./mysh -v -h[n]` or `./mysh -h[n] -v`

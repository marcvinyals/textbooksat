# Textbooksat

Textbooksat is an interactive SAT solver that implements the model of
CDCL described in [Trade-offs Between Time and Memory in a Tighter
Model of CDCL SAT
Solvers](http://dx.doi.org/10.1007/978-3-319-40970-2_11). Its primary use
case is to run interactively to help finding proofs in that model, and
it can also be used to verify such proofs.

## Installation

`make sat`

Dependencies:

* boost

Optional dependencies (disable with `$ NO_VIZ=NO_VIZ make sat`):

* >=graphviz-2.30
* cimg

## Usage

Use `sat < input.cnf` to run with default settings.
Use `sat --help` for information about command line flags.

### Interactive use

Use `sat -i input.cnf -d ask` to run textbooksat in interactive
mode. In this mode, the solver will output the steps it takes, stop
every time it reaches the default state, and ask for user input. Use
`help` for information about the available commands.

To save and resume a session:
```
$ ./sat -d ask
  ...
> save <filename>
> ^D
$ cat <filename> - | ./sat -d ask
```

To have a history of commands:
```
$ rlwrap ./sat -d ask
```

### Resolution proof logging

Use `sat -i input.cnf -p proof.tex; pdflatex proof` to obtain a
graphical representation of the resolution proof the solver
found. This is only intended for small proofs (at most tens of
conflicts); otherwise the picture will be too large for LaTeX to
handle.

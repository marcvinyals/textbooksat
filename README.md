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

* \>=graphviz-2.30
* cimg

## Usage

Use `sat < input.cnf` to run with default settings (which are not what
you would expect from a standard implementation). Use `sat -d vsids -r
luby -w 2wl -v 0 < input.cnf` to run with a more usual set of
heuristics. Use `sat --help` for information about command line flags.

### Interactive use

Use `sat -i input.cnf -d ask` to run textbooksat in interactive
mode. In this mode, the solver will output the steps it takes, stop
every time it reaches the default state, and ask for user input. Type
`help` in interactive mode for information about the available
interactive commands.

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

It can be convenient to work with human-readable variable names
instead of DIMACS literals. If the CNF input contains a map from
DIMACS variable numbers to variable names of the form
```
c varname <dimacs> <name>
```
then textbooksat will use these names for input/output. There is a
[CNFgen patch](https://github.com/marcvinyals/cnfgen/tree/varnames) to
generate such a mapping.

### Proof logging

Use `sat -i input.cnf -p proof.ext` to obtain a representation of the proof in some other format, which gets chosen from the extension.

#### Verification

* `.rup`: Verifiable with [DRAT-trim](https://www.cs.utexas.edu/~marijn/drat-trim/).

#### Graphical

This is only intended for small proofs (at most tens of conflicts); otherwise the picture will
be too large to handle.

* `.tex`: Standalone tikz. Compile with `pdflatex proof.tex`.
* `.beamer.tex`: Beamer slides. Compile with `pdflatex proof.tex`.
* `.asy`: Asymptote. Compile with `asy -f pdf proof.asy`. Requires
  [node.asy](https://github.com/taoari/asy-graphtheory) version 4.0,
  not tested with 5.0.
* `.dot`: Graphviz. Compile with `dot -O -Tpdf out2.dot`.

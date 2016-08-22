# Textbooksat

Textbooksat is an interactive SAT solver that implements the model of
CDCL described in [Trade-offs Between Time and Memory in a Tighter
Model of CDCL SAT
Solvers](dx.doi.org/10.1007/978-3-319-40970-2_11). Its primary use
case is to run interactively to help finding proofs in that model, and
it can also be used to verify such proofs.

## Installation

`make sat`

Dependencies:

* boost

Optional dependencies (disable with `$ NO_VIZ=NO_VIZ make sat`):

* >=graphviz-2.30
* cimg

## Interactive use

Use `sat -i input.cnf -d ask` to run textbooksat in interactive
mode. In this mode, the solver will output the steps it takes, stop
every time it reaches the default state, and ask for user input.

## Resolution proof logging

Use `sat -i input.cnf -p proof.tex; pdflatex proof` to obtain a
graphical representation of the resolution proof the solver found.

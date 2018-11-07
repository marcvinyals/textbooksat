#include "data_structures.h"

#include <ostream>

void measure(const proof& proof);
void draw(std::ostream& out, const proof& proof);
void tikz(std::ostream& out, const proof& proof, bool beamer=false);
void asy(std::ostream& out, const proof& proof);
void rup(std::ostream& out, const proof& proof);

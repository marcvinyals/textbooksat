#include "analysis.h"
#include "formatting.h"

#include <unordered_set>
#include <unordered_map>
#include <cassert>
#include <sstream>
#include <stack>
#include <iostream>

using namespace std;

void measure(const proof& proof) {
  int length=0;
  unordered_map<const proof_clause*, int> last_used;
  int t=0;
  for (auto& c:proof.proof) {
    length+=c.derivation.size();
    for (auto d:c.derivation) last_used[d]=t;
    ++t;
  }
  for (auto& c:proof.formula) last_used.erase(&c);
  vector<int> remove_n(t);
  for (auto& kv:last_used) remove_n[kv.second]++;
  int space=0;
  int in_use=0;
  t=0;
  for (auto& c:proof.proof) {
    in_use += last_used.count(&c);
    space = max(space, in_use);
    in_use -= remove_n[t];
    ++t;
  }
  assert(in_use==0);
  cerr << "Length " << length << endl;
  cerr << "Space " << space << endl;
  cerr << "Non-trivial length " << proof.proof.size() << endl;
}

void draw(std::ostream& out, const proof& proof) {
  unordered_set<const proof_clause*> axioms;
  for (auto& c:proof.formula) axioms.insert(&c);
  out << "digraph G {" << endl;
  for (auto& c:proof.proof) {
    out << "subgraph cluster" << uint64_t(&c) << " {";
    out << "style=filled;" << endl;
    out << "color=lightgrey;" << endl;
    out << "node [style=filled,color=white];" << endl;
    vector<string> lemma_names;
    for (uint i=0;i<c.derivation.size()-1;++i) {
      stringstream ss;
      ss << "lemma" << uint64_t(&c);
      if (i<c.derivation.size()-2) ss << "d" << i;
      if (i) out << lemma_names.back() << " -> " << ss.str() << endl;
      lemma_names.push_back(ss.str());
    }
    int i=-1;
    clause d = c.derivation.front()->c;
    for (auto it=c.derivation.begin();it!=c.derivation.end();++it,++i) {
      if (i>=0) {
        d = resolve(d,(*it)->c);
        out << lemma_names[i] << " [label=\"" << d << "\"];" << endl;
      }
      if (axioms.count(*it)) {
        out << "axiom" << uint64_t(&c) << "d" << i+1 << " [label=\"" << (*it)->c << "\"];" << endl;
        out << "axiom" << uint64_t(&c) << "d" << i+1;
      }
      else out << "lemma" << uint64_t(*it);
      out << " -> " << lemma_names[max(i,0)] << endl;
    }
    out << "}" << endl; // cluster
  }
  out << "}" << endl; // digraph
}

void tikz(std::ostream& out, const proof& proof) {
  unordered_set<const proof_clause*> axioms;
  for (auto& c:proof.formula) axioms.insert(&c);
  out << "\\documentclass{standalone}" << endl;
  out << "\\usepackage{tikz}" << endl;
  out << "\\usetikzlibrary{graphs,positioning,backgrounds}" << endl;
  out << "\\begin{document}" << endl;
  out << "\\begin{tikzpicture}[on grid,axiom/.style={rectangle,fill=blue!20},lemma/.style={rectangle,fill=green!20}]" << endl;
  string previous_lemma;
  for (auto& c:proof.proof) {
    // cluster
    vector<string> lemma_names;
    for (uint i=0;i<c.derivation.size()-1;++i) {
      stringstream ss;
      ss << "lemma" << uint64_t(&c);
      if (i<c.derivation.size()-2) ss << "d" << i;
      lemma_names.push_back(ss.str());
    }
    int i=-1;
    clause d = c.derivation.front()->c;
    stack<string> lines;
    for (auto it=c.derivation.begin();it!=c.derivation.end();++it,++i) {
      if (axioms.count(*it)) {
        stringstream ss;
        ss << "\\node (axiom" << uint64_t(&c) << "d" << i+1 << ") [axiom,below";
        if (i>=0) ss << " left";
        ss << "=of " << lemma_names[max(i,0)];
        if (i==-1) ss << ",yshift=-1cm";
        ss << "]  {" << (*it)->c << "};";
        lines.push(ss.str());
      }
      if (i>=0) {
        d = resolve(d,(*it)->c);
        stringstream ss;
        ss << "\\node (" << lemma_names[i] << ") [lemma,";
        if (i+1<lemma_names.size()) ss << "below=of " << lemma_names[i+1] << ",yshift=-1cm";
        else if (not previous_lemma.empty()) ss << "right=of " << previous_lemma << ",xshift=3cm";
        ss <<"] {" << d << "};";
        lines.push(ss.str());
      }
      
    }
    while(not lines.empty()) {
      out << lines.top() << endl;
      lines.pop();
    }
    out << "\\begin{scope}[on background layer]" << endl;
    out << "\\graph [use existing nodes] {" << endl;
    for (uint i=1;i<c.derivation.size()-1;++i) {
      out << lemma_names[i-1] << " -> " << lemma_names[i] << ";" << endl;
    }
    i=-1;
    for (auto it=c.derivation.begin();it!=c.derivation.end();++it,++i) {    
      if (axioms.count(*it)) {
        out << "axiom" << uint64_t(&c) << "d" << i+1;
      }
      else {
        out << "lemma" << uint64_t(*it);
      }
      out << " -> " << lemma_names[max(i,0)] << ";" << endl;
    }
    out << "};" << endl; // graph
    out << "\\end{scope}" << endl;
    previous_lemma = lemma_names.back();
    // cluster
  }
  out << "\\end{tikzpicture}" << endl;
  out << "\\end{document}" << endl;
}

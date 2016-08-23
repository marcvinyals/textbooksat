#include "analysis.h"
#include "formatting.h"

#include <unordered_set>
#include <unordered_map>
#include <cassert>
#include <sstream>
#include <stack>
#include <iostream>

using namespace std;

template<typename S, typename T>
map<T,int> count_values(const unordered_map<S,T>& dict) {
  map<T,int> ret;
  for (const auto& kv : dict) ret[kv.second]+=1;
  return ret;
}

template<typename S, typename T>
ostream& operator << (ostream& o, const map<S,T>& dict) {
  for (const auto& kv : dict) o << kv.first << ":" << kv.second << " ";
  return o;
}
template<typename T>
ostream& operator << (ostream& o, const unordered_map<const proof_clause*,T>& dict) {
  for (const auto& kv : dict) o << kv.first->c << ":" << kv.second << " ";
  return o;
}

void measure(const proof& proof) {
  unordered_set<const proof_clause*> axioms;
  for (const proof_clause& c:proof.formula) axioms.insert(&c);
  int length=0;
  unordered_map<const proof_clause*, int> last_used;
  unordered_map<const proof_clause*, int> out_degree;
  int t=0;
  for (const proof_clause& c:proof.resolution) {
    length+=c.derivation.size();
    for (auto d:c.derivation) {
      last_used[d]=t;
      if (not axioms.count(d)) out_degree[d]+=1;
    }
    ++t;
  }
  for (auto& c:proof.formula) last_used.erase(&c);
  vector<int> remove_n(t);
  for (auto& kv:last_used) remove_n[kv.second]++;
  int space=0;
  int in_use=0;
  t=0;
  for (const proof_clause& c:proof.resolution) {
    in_use += last_used.count(&c);
    space = max(space, in_use);
    in_use -= remove_n[t];
    ++t;
  }
  assert(in_use==0);
  auto out_degree_sequence = count_values(out_degree);
  cerr << "Length " << length << endl;
  cerr << "Space " << space << endl;
  cerr << "Conflicts " << proof.resolution.size() << endl;
  cerr << "Out degree " << out_degree_sequence << endl;
}

void draw(std::ostream& out, const proof& proof) {
  unordered_set<const proof_clause*> axioms;
  for (const proof_clause& c:proof.formula) axioms.insert(&c);
  out << "digraph G {" << endl;
  for (const proof_clause& c:proof.resolution) {
    out << "subgraph cluster" << uint64_t(&c) << " {";
    out << "style=filled;" << endl;
    out << "color=lightgrey;" << endl;
    out << "node [style=filled,color=white];" << endl;
    vector<string> lemma_names;
    for (size_t i=0;i<c.derivation.size()-1;++i) {
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

void tikz(std::ostream& out, const proof& proof, bool beamer) {
  pretty.mode = pretty.LATEX;
  pretty.lor = " \\lor ";
  pretty.bot = "\\bot";
  unordered_set<const proof_clause*> axioms;
  for (const proof_clause& c:proof.formula) axioms.insert(&c);
  if (beamer) out << "\\documentclass{beamer}" << endl;
  else out << "\\documentclass{standalone}" << endl;
  out << "\\usepackage{tikz}" << endl;
  out << "\\usetikzlibrary{graphs,positioning,backgrounds}" << endl;
  out << "\\begin{document}" << endl;
  if (beamer) out << "\\begin{frame}" << endl;
  out << "\\begin{tikzpicture}[on grid,axiom/.style={rectangle,fill=blue!20},lemma/.style={rectangle,fill=green!20},learned/.style={rectangle,fill=green!40,draw},dc/.style={rectangle,fill=gray!20}]" << endl;
  out << "\\tikzset{arrows=->}" << endl;
  int overlay=0;
  string previous_lemma;
  for (const proof_clause& c:proof.resolution) {
    // cluster
    overlay++;
    vector<string> lemma_names;
    for (size_t i=0;i<c.derivation.size()-1;++i) {
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
        ss << "\\node (" << lemma_names[i] << ") [";
        if (i+1<int(lemma_names.size())) ss << "lemma,below=of " << lemma_names[i+1] << ",yshift=-1cm";
        else {
          ss << "learned";
          if (not previous_lemma.empty()) ss << ",right=of " << previous_lemma << ",xshift=3cm";
        }
        ss <<"] {" << d << "};";
        lines.push(ss.str());
      }      
    }
    if (beamer) out << "\\visible<" << overlay << "->{" << endl;
    while(not lines.empty()) {
      out << lines.top() << endl;
      lines.pop();
    }

    stringstream trail;
    for (const branch& b : c.trail) {
      trail << pretty << variable(b.to);
      if (b.reason) {
        trail << "{=}";
        if (not axioms.count(b.reason)) {
          stringstream ss;
          ss << "\\draw [dashed] (lemma" << uint64_t(b.reason) << ") to (dc" << uint64_t(&c) << ");";
          lines.push(ss.str());
        }
      }
      else trail << "\\stackrel{d}{=}";
      trail << b.to.polarity();
      trail << "\\;";
    }
    out << "\\node (dc" << uint64_t(&c) << ") [dc,above=of lemma" << uint64_t(&c) << ",yshift=0.5cm] {\\footnotesize{$" << trail.str() << "$}};" << endl;
    if (beamer) out << "}" << endl; // visible

    if (beamer) out << "\\only<" << overlay << "->{" << endl;
    out << "\\begin{scope}[on background layer]" << endl;
    for (size_t i=1;i<c.derivation.size()-1;++i) {
      out << "\\draw (" << lemma_names[i-1] << ") to (" << lemma_names[i] << ");" << endl;
    }
    i=-1;
    for (auto it=c.derivation.begin();it!=c.derivation.end();++it,++i) {
      out << "\\draw (";
      if (axioms.count(*it)) {
        out << "axiom" << uint64_t(&c) << "d" << i+1;
      }
      else {
        out << "lemma" << uint64_t(*it);
      }
      out << ") to (" << lemma_names[max(i,0)] << ");" << endl;
    }
    while(not lines.empty()) {
      out << lines.top() << endl;
      lines.pop();
    }
    out << "\\end{scope}" << endl;
    if (beamer) out << "}" << endl; // only
    previous_lemma = lemma_names.back();
    // cluster
  }
  out << "\\end{tikzpicture}" << endl;
  if (beamer) out << "\\end{frame}" << endl;
  out << "\\end{document}" << endl;
}

void asy(std::ostream& out, const proof& proof) {
  pretty.mode = pretty.LATEX;
  unordered_set<const proof_clause*> axioms;
  for (const proof_clause& c:proof.formula) axioms.insert(&c);
  out << "import node;" << endl;
  out << "real dx = 7.5cm;" << endl;
  out << "real dy = 1cm;" << endl;
  out << "nodestyle lemmastyle=nodestyle(xmargin=3pt, ymargin=3pt, drawfn=FillDrawer(lightgreen,black));" << endl;
  out << "nodestyle axiomstyle=nodestyle(xmargin=3pt, ymargin=3pt, drawfn=FillDrawer(lightblue,black));" << endl;
  string previous_lemma;
  vector<string> stuff;
  for (const proof_clause& c:proof.resolution) {
    vector<string> lemma_names;
    vector<string> axiom_names;
    for (size_t i=0;i<c.derivation.size()-1;++i) {
      stringstream ss;
      ss << "lemma" << uint64_t(&c);
      if (i<c.derivation.size()-2) ss << "d" << i;
      lemma_names.push_back(ss.str());
    }
    // Declare nodes
    int i=-1;
    clause d = c.derivation.front()->c;
    for (auto it=c.derivation.begin();it!=c.derivation.end();++it,++i) {
      if (i>=0) {
        d = resolve(d,(*it)->c);
        out << "node " << lemma_names[i] << " = sroundbox(\"" << d << "\", lemmastyle);" << endl;
      }
      if (axioms.count(*it)) {
        stringstream ss;
        ss << "axiom" << uint64_t(&c) << "d" << i+1;
        out << "node " << ss.str() << " = sroundbox(\"" << (*it)->c << "\",axiomstyle);" << endl;
        axiom_names.push_back(ss.str());
      }
    }
    // Position nodes
    if(not previous_lemma.empty()) out << previous_lemma << " << eright(dx) << " << lemma_names.back() << ";" << endl;    
    for (auto it = lemma_names.rbegin();it+1!=lemma_names.rend();) {
      out << *it;
      ++it;
      out << " << edown(dy) << " << *it << ";" << endl;
    }
    i=-1;
    for (auto it=c.derivation.begin();it!=c.derivation.end();++it,++i) {
      if (axioms.count(*it)) {
        out << lemma_names[max(i,0)];
        if (i==-1) out << " << edown(dy) << ";
        else out << " << edir(-160,dx/3) << ";
        out << "axiom" << uint64_t(&c) << "d" << i+1 << ";" << endl;
      }
    }
    // Draw edges
    for (auto it = lemma_names.begin();it+1!=lemma_names.end();) {
      out << "draw(" << *it;
      ++it;
      out << "--" << *it << ",Arrow);" << endl;
    }
    i=-1;
    for (auto it=c.derivation.begin();it!=c.derivation.end();++it,++i) {
      out << "draw(";
      if (axioms.count(*it)) {
        out << "axiom" << uint64_t(&c) << "d" << i+1;
      }
      else out << "lemma" << uint64_t(*it);
      out << "--" << lemma_names[max(i,0)] << ",Arrow);" << endl;
    }

    for (auto& s : lemma_names) stuff.push_back(s);
    for (auto& s : axiom_names) stuff.push_back(s);
    previous_lemma = lemma_names.back();
  }
  // Draw nodes
  for (auto& s : stuff) out << "draw(" << s << ");" << endl;
}

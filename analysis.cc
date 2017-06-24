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

class drawer {
protected:
  std::ostream& out;
  const struct proof& proof;
  unordered_set<const proof_clause*> axioms;
  vector<string> lemma_names;
  int conflict;
  string previous_lemma;
  void make_lemma_names(const proof_clause& c) {
    if(not lemma_names.empty()) previous_lemma = lemma_names.back();
    lemma_names.clear();
    for (size_t i=0;i<c.derivation.size()-1;++i) {
      stringstream ss;
      ss << "lemma" << uint64_t(&c);
      if (i<c.derivation.size()-2) ss << "d" << i;
      lemma_names.push_back(ss.str());
    }    
  }
  string axiom_name(const proof_clause& c, int i) {
    stringstream ss;
    ss << "axiom" << uint64_t(&c) << "d" << i+1;
    return ss.str();
  }
  string lemma_name(const proof_clause& c) {
    stringstream ss;
    ss << "lemma" << uint64_t(&c);
    return ss.str();
  }
  string box_name(const proof_clause& c) {
    stringstream ss;
    ss << "dc" << uint64_t(&c);
    return ss.str();
  }
  virtual void begin() {}
  virtual void end() {}
  virtual void begin_conflict() {}
  virtual void end_conflict() {}
  virtual void new_axiom(const clause& c, const string& name, const string& dest, bool first) {}
  virtual void new_lemma(const clause& c, const string& name, const string& dest) {}
  virtual void new_learned(const clause& c, const string& name) {}
  virtual void begin_trail() {}
  virtual void end_trail() {}
  virtual void new_assignment(stringstream& trail, literal l, bool propagation) {}
  virtual void begin_edges() {}
  virtual void end_edges() {}
  virtual void trail_edge(const string& source, const string& dest) {}
  virtual void new_box(const string& name, const string& dest, const string& trail) {}
  virtual void new_edge(const string& source, const string& dest) {}

  void draw_clauses(const proof_clause& c) {
    int i=-1;
    clause d = c.derivation.front()->c;
    stack<string> lines;
    for (auto it=c.derivation.begin();it!=c.derivation.end();++it,++i) {
      if (axioms.count(*it)) {
        new_axiom((*it)->c,axiom_name(c,i),lemma_names[max(i,0)],i==-1);
      }
      if (i>=0) {
        d = resolve(d,(*it)->c);
        if (i+1<int(lemma_names.size())) {
          new_lemma(d,lemma_names[i],lemma_names[i+1]);
        }
        else {
          new_learned(d,lemma_names[i]);
        }
      }
    }
  }
  
  void draw_box(const proof_clause& c) {
    stringstream trail;
    for (const branch& b : c.trail) {
      new_assignment(trail, b.to, b.reason);
      if (b.reason and not axioms.count(b.reason)) {
        trail_edge(lemma_name(*b.reason), box_name(c));
      }
    }
    new_box(box_name(c), lemma_name(c), trail.str());
  }

  void draw_edges(const proof_clause& c) {
    for (size_t i=1;i<c.derivation.size()-1;++i) {
      new_edge(lemma_names[i-1], lemma_names[i]);
    }
    int i=-1;
    for (auto it=c.derivation.begin();it!=c.derivation.end();++it,++i) {
      string source = axioms.count(*it)?axiom_name(c, i):lemma_name(**it);
      new_edge(source, lemma_names[max(i,0)]);
    }
  }

public:
  drawer(std::ostream& from_out, const struct proof& from_proof) : out(from_out), proof(from_proof) {
    for (const proof_clause& c:proof.formula) axioms.insert(&c);
  }
  void draw() {
    begin();
    conflict=0;
    for (const proof_clause& c:proof.resolution) {
      conflict++;
      make_lemma_names(c);
      begin_conflict();
      draw_clauses(c);
      end_conflict();
      begin_trail();
      draw_box(c);
      end_trail();
      begin_edges();
      draw_edges(c);
      end_edges();
    }
    end();
  }
  virtual ~drawer() {}
};

class graphviz_drawer : public drawer {
protected:
  virtual void begin() override {
    out << "digraph G {" << endl;
  }
  virtual void end() override {
    out << "}" << endl; // digraph
  }
  virtual void begin_conflict() override {
    out << "subgraph cluster" << lemma_names.back() << " {" << endl;
    out << "style=filled;" << endl;
    out << "color=lightgrey;" << endl;
    out << "node [style=filled,color=white];" << endl;
  }
  virtual void end_conflict() override {
    out << "}" << endl; // cluster
  }
  virtual void new_edge(const string& source, const string& dest) override {
    out << source << " -> " << dest << endl;
  }
  virtual void new_axiom(const clause& c, const string& name, const string& dest, bool first) {
    out << name << " [label=\"" << c << "\"];" << endl;
  }
  virtual void new_lemma(const clause& c, const string& name, const string& dest) {
    out << name << " [label=\"" << c << "\"];" << endl;
  }
  virtual void new_learned(const clause& c, const string& name) {
    out << name << " [label=\"" << c << "\"];" << endl;
  }
public:
  graphviz_drawer(std::ostream& from_out, const struct proof& from_proof) :
    drawer(from_out, from_proof) {}
};

void draw(std::ostream& out, const proof& proof) {
  graphviz_drawer(out,proof).draw();
}

class tikz_drawer : public drawer {
protected:
  bool beamer;
  stack<string> lines;
  void flush() {
    while(not lines.empty()) {
      out << lines.top() << endl;
      lines.pop();
    }
  }
  virtual void begin() override {
    pretty.mode = pretty.LATEX;
    pretty.lor = " \\lor ";
    pretty.bot = "\\bot";
    if (beamer) out << "\\documentclass{beamer}" << endl;
    else out << "\\documentclass{standalone}" << endl;
    out << "\\usepackage{tikz}" << endl;
    out << "\\usetikzlibrary{graphs,positioning,backgrounds}" << endl;
    out << "\\begin{document}" << endl;
    if (beamer) out << "\\begin{frame}" << endl;
    out << "\\begin{tikzpicture}[on grid,axiom/.style={rectangle,fill=blue!20},lemma/.style={rectangle,fill=green!20},learned/.style={rectangle,fill=green!40,draw},dc/.style={rectangle,fill=gray!20}]" << endl;
    out << "\\tikzset{arrows=->}" << endl;
  }
  virtual void end() override {
    out << "\\end{tikzpicture}" << endl;
    if (beamer) out << "\\end{frame}" << endl;
    out << "\\end{document}" << endl;
  }
  virtual void new_axiom(const clause& c, const string& name, const string& dest, bool first) override {
    stringstream ss;
    ss << "\\node (" << name << ") [axiom,below";
    if (not first) ss << " left";
    ss << "=of " << dest;
    if (first) ss << ",yshift=-1cm";
    ss << "]  {" << c << "};";
    lines.push(ss.str());
  }
  virtual void new_lemma(const clause& c, const string& name, const string& dest) override {
    stringstream ss;
    ss << "\\node (" << name << ") [";
    ss << "lemma,below=of " << dest << ",yshift=-1cm";
    ss <<"] {" << c << "};";
    lines.push(ss.str());
  }
  virtual void new_learned(const clause& c, const string& name) override {
    stringstream ss;
    ss << "\\node (" << name << ") [";
    ss << "learned";
    if (not previous_lemma.empty()) ss << ",right=of " << previous_lemma << ",xshift=3cm";
    ss <<"] {" << c << "};";
    lines.push(ss.str());
  }
  virtual void begin_conflict() override {
    if (beamer) out << "\\visible<" << conflict << "->{" << endl;
  }
  virtual void end_conflict() override {
    flush();
  }
  virtual void end_trail() override {
    if (beamer) out << "}" << endl; // visible
  }
  virtual void trail_edge(const string& source, const string& dest) override {
    stringstream ss;
    ss << "\\draw [dashed] (" << source << ") to (" << dest << ");";
    lines.push(ss.str());
  }
  virtual void new_assignment(stringstream& trail, literal l, bool propagation) override {
    trail << pretty << variable(l);
    if (propagation) trail << "{=}";
    else trail << "\\stackrel{d}{=}";
    trail << l.polarity();
    trail << "\\;";      
  }
  virtual void new_box(const string& name, const string& dest, const string& trail) override {
    out << "\\node (" << name << ") [dc,above=of " << dest << ",yshift=0.5cm] {\\footnotesize{$" << trail << "$}};" << endl;
  }
  virtual void begin_edges() override {
    if (beamer) out << "\\only<" << conflict << "->{" << endl;
    out << "\\begin{scope}[on background layer]" << endl;
    flush();
  }
  virtual void end_edges() override {
    out << "\\end{scope}" << endl;
    if (beamer) out << "}" << endl; // only
  }
  virtual void new_edge(const string& source, const string& dest) override {
    out << "\\draw[bend left=5] (" << source << ") to (" << dest << ");" << endl;
  }
public:
  tikz_drawer(std::ostream& from_out, const struct proof& from_proof, bool from_beamer) :
    drawer(from_out, from_proof), beamer(from_beamer) {}
};

void tikz(std::ostream& out, const proof& proof, bool beamer) {
  tikz_drawer(out, proof, beamer).draw();
}

class asy_drawer : public drawer {
protected:
  stack<string> lines;
  vector<string> stuff;
  virtual void begin() override {
    pretty.mode = pretty.LATEX;
    out << "import node;" << endl;
    out << "real dx = 7.5cm;" << endl;
    out << "real dy = 1cm;" << endl;
    out << "nodestyle lemmastyle=nodestyle(xmargin=3pt, ymargin=3pt, drawfn=FillDrawer(lightgreen,black));" << endl;
    out << "nodestyle axiomstyle=nodestyle(xmargin=3pt, ymargin=3pt, drawfn=FillDrawer(lightblue,black));" << endl;
  }
  virtual void new_axiom(const clause& c, const string& name, const string& dest, bool first) override {
    stuff.push_back(name);
    out << "node " << name << " = sroundbox(\"" << c << "\", axiomstyle);" << endl;
    stringstream ss;
    ss << dest;
    if (first) ss << " << edown(dy) << ";
    else ss << " << edir(-160,dx/3) << ";
    ss << name;
    lines.push(ss.str());
  }
  virtual void new_lemma(const clause& c, const string& name, const string& dest) override {
    stuff.push_back(name);
    out << "node " << name << " = sroundbox(\"" << c << "\", lemmastyle);" << endl;
    stringstream ss;
    ss << dest << " << edown(dy) << " << name;
    lines.push(ss.str());
  }
  virtual void new_learned(const clause& c, const string& name) override {
    stuff.push_back(name);
    out << "node " << name << " = sroundbox(\"" << c << "\", lemmastyle);" << endl;
    if(not previous_lemma.empty()) {
      stringstream ss;
      ss << previous_lemma << " << eright(dx) << " << lemma_names.back();
      lines.push(ss.str());
    }
  }
  virtual void new_edge(const string& source, const string& dest) override {
    out << "draw(" << source << "--" << dest << ",Arrow);" << endl;
  }
  virtual void end_conflict() override {
    while(not lines.empty()) {
      out << lines.top() << ";" << endl;
      lines.pop();
    }
  }
  virtual void end() override {
    for (auto& s : stuff) out << "draw(" << s << ");" << endl;
  }
public:
  asy_drawer(std::ostream& from_out, const struct proof& from_proof) :
    drawer(from_out, from_proof) {}
};

void asy(std::ostream& out, const proof& proof) {
  asy_drawer(out, proof).draw();
}

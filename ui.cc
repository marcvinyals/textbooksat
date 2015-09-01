#include "ui.h"

#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include "cdcl.h"
#include "formatting.h"

using std::cout;
using std::cin;
using std::cerr;
using std::getline;
using std::endl;
using std::string;

namespace qi = boost::spirit::qi;
namespace ph = boost::phoenix;
using qi::int_;
using qi::eps;
using qi::lit;
using qi::_1;
using ph::ref;

std::ostream& operator << (std::ostream& o, const std::vector<restricted_clause>& v) {
  for (size_t i = 0; i<v.size(); ++i) {
    o << std::setw(5) << i << ":";
    if (v[i].satisfied) o << std::setw(35+8);
    else o << std::setw(35);
    std::stringstream ss;
    ss << v[i];
    o << ss.str();
    o << " | " << v[i].source->c << std::endl;
  }
  return o;
}

void ui::show_state() {
  cout << "Branching sequence: " << solver.branching_seq << endl;
  cout << "Learnt clauses:" << endl << solver.learnt_clauses << endl;
  cout << "Restricted clauses:" << endl << solver.working_clauses << endl;

}

void ui::usage() {
  if (not batch) {
    cout << "Please input either of:" << endl;
    cout << " * <literal in dimacs format>" << endl;
    cout << " * [assign] <varname> {0,1}" << endl;
    cout << " * restart" << endl;
    cout << " * forget <restricted clause number>" << endl;
    cout << " * forget wide <width>" << endl;
    cout << " * forget domain <variable>*" << endl;
    cout << " * forget touches all <variable>*" << endl;
    cout << " * forget touches any <variable>*" << endl;
    cout << " * state" << endl;
    cout << " * save <file>" << endl;
    cout << " * batch {0,1}" << endl;
  }
}

literal_or_restart ui::get_decision() {
  qi::symbols<char, variable> variable_;
  for (const auto& it : pretty.variable_names) variable_.add(it.second, it.first);
  literal decision = from_dimacs(0);
  while (true) {
    string line;
    getline(cin, line);
    if (not cin) {
      cerr << "No more input?" << endl;
      exit(1);
    }
    if (line.empty()) return solver.decide_fixed();
    string action, file;
    int m, polarity, w = 2, dimacs;
    variable var;
    std::vector<variable> domain;
    auto it = line.begin();
    auto token = qi::as_string[qi::lexeme[+~qi::space]];
    bool parse = qi::phrase_parse(it, line.end(),
        qi::string("#")[ph::ref(action) = _1]
      | qi::string("state")[ph::ref(action) = _1]
      | qi::string("save")[ph::ref(action) = _1] >> token[ph::ref(file) = _1]
      | qi::string("batch")[ph::ref(action) = _1] >> int_[ref(batch) = _1]
      | qi::string("restart")[ph::ref(action) = _1]
      | qi::string("forget")[ph::ref(action) = _1] >> int_[ref(m) = _1]
      | qi::string("forget wide")[ph::ref(action) = _1] >> -int_[ref(w) = _1]
      | qi::string("forget domain")[ph::ref(action) = _1] >> (*variable_)[ph::ref(domain) = _1]
      | qi::string("forget touches any")[ph::ref(action) = _1] >> (*variable_)[ph::ref(domain) = _1]
      | qi::string("forget touches all")[ph::ref(action) = _1] >> (*variable_)[ph::ref(domain) = _1]
      | -lit("assign") >> variable_[ph::ref(var) = _1] >> int_[ref(polarity) = _1] >> eps[ph::ref(action) = "assign"]
      | int_[ref(dimacs) = _1] >> eps[ph::ref(action) = "dimacs"]
                                  , qi::space);
    if (not parse) {usage(); continue;}
    if (action=="#") {
      history.push_back(line);
      continue;
    }
    if (it != line.end()) {usage(); continue;}
    if (action=="state") {
      show_state();
      continue;
    }
    if (action=="restart") {
      history.push_back(line);
      return true;
    }
    else if (action=="forget") {
      if (m < solver.formula.size()) {
        cerr << "Refusing to forget an axiom" << endl;
        continue;
      }
      if (m >= solver.working_clauses.size()) {
        cerr << "I cannot forget something I have not learnt yet" << endl;
        continue;
      }
      history.push_back(line);
      solver.forget(m);
      return solver.decide_plugin(solver);
    }
    else if (action=="forget wide") {
      history.push_back(line);
      solver.forget_wide(w);
      return solver.decide_plugin(solver);
    }
    else if (action=="forget domain") {
      history.push_back(line);
      solver.forget_domain(domain);
      return solver.decide_plugin(solver);
    }
    else if (action=="forget touches any") {
      history.push_back(line);
      solver.forget_touches_any(domain);
      return solver.decide_plugin(solver);
    }
    else if (action=="forget touches all") {
      history.push_back(line);
      solver.forget_touches_all(domain);
      return solver.decide_plugin(solver);
    }
    else if (action == "assign") {
      if (polarity<0 or polarity>1) {
        cerr << "Polarity should be 0 or 1" << endl;
        continue;
      }
      decision = literal(var, polarity);
    }
    else if (action == "dimacs") {
      decision = from_dimacs(dimacs);
    }
    else if (action == "save") {
      std::ofstream out(file);
      for (const auto& line : history) out << line << endl;
      continue;
    }
    else if (action == "batch") return solver.decide_plugin(solver);
    else assert(false);
    history.push_back(line);
    break;
  }
  variable x(decision);
  if (x >= solver.assignment.size()) {
    cerr << "Variable not found" << endl;
    return get_decision();
  }
  if (solver.assignment[x]) {
    cerr << "Refusing to decide an assigned variable" << endl;
    return get_decision();
  }
  return decision;
}

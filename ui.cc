#include "ui.h"

#include <iostream>
#include <fstream>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include "cdcl.h"

using std::cout;
using std::cin;
using std::getline;

literal_or_restart ui::get_decision() {
  cout << "Good day oracle, would you mind giving me some advice?" << endl;
cout << "This is the branching sequence so far: " << solver.build_branching_seq() << endl;
  cout << "I learned the following clauses:" << endl << solver.learnt_clauses << endl;
  cout << "Therefore these are the restricted clauses I have in mind:" << endl << solver.working_clauses << endl;
  int dimacs_decision = 0;
  while (not dimacs_decision) {
    cout << "Please input either of:" << endl;
    cout << " * a literal in dimacs format" << endl;
    cout << " * an assignment <varname> {0,1}" << endl;
    cout << " * the keyword 'restart'" << endl;
    cout << " * the keyword 'forget' and a restricted clause number" << endl;
    string line;
    getline(cin, line);
    if (not cin) {
      cerr << "No more input?" << endl;
      exit(1);
    }
    string action, var, file;
    int m, polarity;
    namespace qi = boost::spirit::qi;
    namespace ph = boost::phoenix;
    auto it = line.begin();
    bool parse = qi::phrase_parse(it, line.end(),
        qi::string("#")[ph::ref(action) = qi::_1]
      | qi::string("restart")[ph::ref(action) = qi::_1]
      | qi::string("forget")[ph::ref(action) = qi::_1] >> qi::int_[ph::ref(m) = qi::_1]
      | -qi::lit("assign") >> qi::as_string[qi::lexeme[+~qi::space]][ph::ref(var) = qi::_1] >> qi::int_[ph::ref(polarity) = qi::_1] >> qi::eps[ph::ref(action) = "assign"]
      | qi::int_[ph::ref(dimacs_decision) = qi::_1] >> qi::eps[ph::ref(action) = "dimacs"]
      | qi::string("save")[ph::ref(action) = qi::_1] >> qi::as_string[qi::lexeme[+~qi::space]][ph::ref(file) = qi::_1]
                                  , qi::space);
    if (not parse) continue;
    if (action=="#") {
      history.push_back(line);
      continue;
    }
    if (it != line.end()) continue;
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
    else if (action == "assign") {
      auto it= pretty.name_variables.find(var);
      if (it == pretty.name_variables.end()) continue;
      polarity = polarity*2-1;
      if (abs(polarity)>1) continue;
      dimacs_decision = polarity*( it->second + 1 );
    }
    else if (action == "dimacs");
    else if (action == "save") {
      ofstream out(file);
      for (const auto& line : history) out << line << endl;
      continue;
    }
    else assert(false);
    history.push_back(line);
  }
  return from_dimacs(dimacs_decision);
}

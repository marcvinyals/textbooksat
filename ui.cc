#include "ui.h"

#include <iostream>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

using std::cout;
using std::cin;
using std::getline;

literal_or_restart ui::get_decision(cdcl& solver) {
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
                                  , qi::space);
    if (not parse) continue;
    if (action=="#") continue;
    if (it != line.end()) continue;
    if (action=="restart") return true;
    else if (action=="forget") {
      cerr << m << endl;
      if (m < solver.formula.size()) {
        cerr << "Refusing to forget an axiom" << endl;
        continue;
      }
      if (m >= solver.working_clauses.size()) {
        cerr << "I cannot forget something I have not learnt yet" << endl;
        continue;
      }
      solver.forget(m);
      return solver.decide_ask();
    }
    else if (action == "assign") {
      cerr << polarity << endl;
      auto it= pretty.name_variables.find(var);
      if (it == pretty.name_variables.end()) continue;
      polarity = polarity*2-1;
      if (abs(polarity)>1) continue;
      dimacs_decision = polarity*( it->second + 1 );
    }
    else if (action == "dimacs");
    else assert(false);
  }
  return from_dimacs(dimacs_decision);
}

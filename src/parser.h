#include <boost/algorithm/string/replace.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>

namespace qi    = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phx   = boost::phoenix;


namespace functionTable
{
  int or_(int a, int b)
    {
      //std::cout << "OR.... a = " << a << " b = " << b << "\n";
      return a | b;
    }

  int and_(int a, int b)
    {
      //std::cout << "AND.... a = " << a << " b = " << b << "\n";
      return a & b;
    }

  int not_(int a)
    {
      //std::cout << "NOT = " << a <<"\n";
      return !a;
    }

  int imp_(int a, int b)
    {
      return ~a | b;
    }

  int equiv_(int a, int b)
    {
      return a == b;
    }

  int xor_(int a, int b)
  {
    return a ^ b;
  }

}
//end namespace functionTable

namespace expEval
{
  //http://www.boost.org/doc/libs/1_60_0/libs/phoenix/doc/html/phoenix/modules/function.html
  //http://www.boost.org/doc/libs/1_61_0/libs/phoenix/doc/html/phoenix/starter_kit/lazy_functions.html
  struct unaryFunction_
  {
    template <typename F, typename Arg1>
    struct result { typedef Arg1 type; };
    template <typename F, typename Arg1>
    Arg1 operator()(F f, Arg1 a) const
    {
      return f(a);
    }
  };

  struct binaryFunction_
  {
    template <typename F, typename Arg1, typename Arg2>
    struct result { typedef Arg1 type; };
    template <typename F, typename Arg1, typename Arg2>
    Arg1 operator()(F f, Arg1 a, Arg2 b) const
    {
      return f(a, b);
    }
  };

  template <typename functionPtr, typename Iterator>
  struct grammar: qi::grammar<Iterator, functionPtr(), ascii::space_type>
  {
    //using namespace qi::symbols;
    //Symbol list for grammar
    struct zeroOrOne_ : qi::symbols<typename std::iterator_traits<Iterator>::value_type, functionPtr>
    {
      zeroOrOne_()
      {
	this->add("digits", std::numeric_limits<functionPtr>::digits)
	("digits10", std::numeric_limits<functionPtr>::digits10);
      }
    }zeroOrOne;

    struct unaryFunctionSymbol_ : qi::symbols<typename std::iterator_traits<Iterator>::value_type, functionPtr (*)(functionPtr)>
    {
      unaryFunctionSymbol_()
      {
	this->add("not", static_cast<functionPtr (*)(functionPtr)>(&functionTable::not_));
      }
    }unaryFunctionSymbol;

    //using namespace qi::symbols;
    //qi::symbols<typename std::iterator_traits<Iterator>::value_type, functionPtr (*)(functionPtr)> ufunc;
    //ufunc;
    //ufunc.add("not"  , static_cast<functionPtr (*)(functionPtr)>(&not_));
    //ufunc.add("not"  , static_cast<functionPtr (*)(functionPtr)>(&not_));

    struct binaryFunctionSymbol_ : qi::symbols<typename std::iterator_traits<Iterator>::value_type, functionPtr (*)(functionPtr, functionPtr)>
    {
      binaryFunctionSymbol_()
      {
	this->add("or", static_cast<functionPtr (*)(functionPtr, functionPtr)>(&functionTable::or_))
	  ("and", static_cast<functionPtr (*)(functionPtr, functionPtr)>(&functionTable::and_))
	  ("imp", static_cast<functionPtr (*)(functionPtr, functionPtr)>(&functionTable::imp_))
	  ("equiv", static_cast<functionPtr (*)(functionPtr, functionPtr)>(&functionTable::equiv_))
	  ("xor", static_cast<functionPtr (*)(functionPtr, functionPtr)>(&functionTable::xor_));

      }
    }binaryFunctionSymbol;

    //Rule to recognise literals, brackets, func(a), func(a,b) and nested functions
    qi::rule<Iterator, functionPtr(), ascii::space_type>			ruleDef;

    //grammar constructor
    grammar() : grammar::base_type(ruleDef)
    {
      using namespace qi;
      real_parser<functionPtr, real_policies<functionPtr>>		x;
      phx::function<unaryFunction_>					ufuncPhx;
      phx::function<binaryFunction_>					bfuncPhx;

      ruleDef =      x              							[_val =  _1]
		 |   '(' >> ruleDef                                                     [_val =  _1] >> ')'
		 |   (no_case[unaryFunctionSymbol] >> '(' >> ruleDef >> ')')   			[_val = ufuncPhx(_1, _2)]
		 |   (no_case[binaryFunctionSymbol] >> '(' >> ruleDef >> ',' >> ruleDef >> ')')        [_val = bfuncPhx(_1, _2, _3)]
	   	 |   no_case[zeroOrOne]                                                  [_val =  _1];

	    }
  };

}
//end namespace expEval


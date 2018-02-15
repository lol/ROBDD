#include <iostream>
#include <unordered_map>
#include <vector>
#include <cmath>
#include "parser.h"

class ROBDD {
  std::vector<std::vector<int> > T;
  std::unordered_map<int, int> H;
  int nodes;
  int n;

  //for apply()
  std::vector<std::vector<int> > T1;
  std::vector<std::vector<int> > T2;
  std::vector<std::vector<int> > G;
  std::string op;

  //for restrict()
  std::vector<std::vector<int> > TR;
  int j, b;
  int nodes_;

  //for anySat()
  int isSat;
  std::vector<int> satVars;
  
  int app(int l, int h)
  {
    int u;
    //std::cout << " l = " << l << " h = " << h << "\n\n";
    if(G[l][h] != -1)
      return G[l][h];
    else if( ((l == 0) || (l == 1)) && ((h == 0) || (h == 1)) )
      u = evaluate(op + "(" + std::to_string(l) + "," + std::to_string(h) + ")");
    else if(T1[l][0] == T2[h][0])
      u = mk(T1[l][0], app(T1[l][1], T2[h][1]), app(T1[l][2], T2[h][2]));
    else if(T1[l][0] < T2[h][0])
      u = mk(T1[l][0], app(T1[l][1], h), app(T1[l][2], h));
    else
      u = mk(T1[h][0], app(l, T2[h][1]), app(l, T2[h][2]));
    G[l][h] = u;
    return u;    
  }

public:
  ROBDD(int x)
  {
    n = x;
    T.resize(2, std::vector<int>(3));
    T[0][0] = T[1][0] = n + 1;
    T[0][1] = T[0][2] = T[1][1] = T[1][2] = -1;
    //There are two end nodes denoting 0 and 1.
    nodes = 2;
  }

  int getNumNodes()
  {
    return nodes;
  }

  void printT()
  {
    std::cout << "u\tvar\tlow\thigh\n----------------------------\n";
    for(unsigned int i = 0; i<T.size(); i++)
      {
	std::cout << i << "\t";
	for(unsigned int j = 0; j<T[i].size(); j++)
	  {
	    if(j == 0)
	      std::cout << "x";
	    std::cout << T[i][j] << "\t";
	  }
	std::cout << "\n";
      }
  }

  // for debugging restrict
  void printTR()
  {
    std::cout << "u\tvar\tlow\thigh\n----------------------------\n";
    for(unsigned int i = 0; i<TR.size(); i++)
      {
	std::cout << i << "\t";
	for(unsigned int j = 0; j<TR[i].size(); j++)
	  {
	    if(j == 0)
	      std::cout << "x";
	    std::cout << TR[i][j] << "\t";
	  }
	std::cout << "\n";
      }
  }

  //Hash function for std::unordered_map<int, int> H; (i, l, h) get mapped to first three
  //bytes of integer. Obvious downside is, restriction on the values i, l, h can take.
  int hash(int i, int l, int h)
  {
    return 0 | (i << 16) | (l << 8) | h;
  }
	
  int mk(int i, int l, int h)
  {
    if(l == h)
      {
	//std::cout << "\n\n if(l == h) \n\n"; printT();
	return l;
      }
    else if(H.end() != H.find(hash(i, l, h)))   //if unordered_map::find is unable to find the key, it returns an iterator for unordered_map::end
      {
	//std::cout << "\n\n if(member(H, i, l, h)) \n\n"; printT();
	return H.at( { hash(i, l, h) } );
      }
    else
      {
	//std::cout << "\n\n Add node to T... u <-- add(T, i, l, h)\n\n";
	// add (i, l, h) to T			
	T.push_back(std::vector<int>());
	T[nodes].push_back(i);
	T[nodes].push_back(l);
	T[nodes].push_back(h);
	//printT();
	// add to Hash map
	H.insert( { hash(i, l, h), nodes } );
	return nodes++;
      }
  }


  int build(std::string exp, int i)
  {
    //std::cout << " i = " << i << " n = " << n << std::endl;
    if(i > n)
      {
	if(evaluate(exp))
	  return 1;
	else
	  return 0;
      }
    else
	{
	  //std::cout << "gotta replace : " << exp << " i = " << i << ", i + 1 = " << i + 1 <<std::endl;
	  int l = build(expressionReplace(exp, i, 0), i + 1);
	  int h = build(expressionReplace(exp, i, 1), i + 1);
	  return mk(i, l, h);
	}
  }

  int evaluate(std::string expression)
  {
    //std::cout << "\n\n evaluate(expression) --- " << expression << "\n\n";
    int result;
    const expEval::grammar<int, std::string::const_iterator> parser;
    std::string::const_iterator start = expression.begin();  
    std::string::const_iterator end  = expression.end();
    //std::cout << expression << std::endl;
    bool valid = qi::phrase_parse(start, end, parser, ascii::space, result);
    if(valid)
	return result;
    else
      {
	std::cout << "Error in expression. Please check brackets or constructor.\n";
	exit(0);
      }
  }
			
  std::string expressionReplace(std::string exp, int var, int val)
  {
    boost::replace_all(exp, "x" + std::to_string(var), std::to_string(val));
    //std::cout << exp << std::endl;
    std::string variablesReplacedByValue = exp;
    return variablesReplacedByValue;
  }

  void apply(std::string operand, ROBDD u1, ROBDD u2)
  {
    T1 = u1.T;
    T2 = u2.T;
    std::cout << u1.nodes << " " << u2.nodes << "\n";
   
    //G = nodes in u1 X nodes in u2
    G.resize(u1.getNumNodes(), std::vector<int>(u2.getNumNodes()));
    op = operand;

    //fill G with -1
    for(auto &i : G)
      std::fill(i.begin(), i.end(), -1);

    //call app()
    app(u1.getNumNodes() - 1, u2.getNumNodes() - 1);

    //clear T1, T2, G and op. They are not required anymore.
    T1.clear();
    T2.clear();
    G.clear();
    op.clear();
  }

  void restrict(int j_, int b_)
  {
    //Setting up for new T and H. Store old T and previous number of nodes. Clear H and T.
    j = j_;
    b = b_;
    TR = T;
    T.clear();
    T.resize(2, std::vector<int>(3));
    T[0][0] = T[1][0] = n + 1;
    T[0][1] = T[0][2] = T[1][1] = T[1][2] = -1;
    //std::cout <<"About to begin restrict -- first T and then TR\n";
    //printT();
    //printTR();
    //There are two end nodes denoting 0 and 1.
    //Save previous number of nodes.
    nodes_ = nodes;
    nodes = 2;
    H.clear();
    res(nodes_ - 1);
 }

  void append(int u)
  {
    if(u == 0 || u == 1)
      return;
    append(TR[u][1]);
    append(TR[u][2]);
    mk(TR[u][0], TR[u][1], TR[u][2]);
  }
    
  int res(int u)
  {
    //std::cout <<"\t\t res(u)...u = " << u << " .. details " << TR[u][0] << " " << TR[u][1] << " " << TR[u][2]  << "\n";
    if(TR[u][0] > j)
      {
	//std::cout << "<=> Time to return u (node #) = " << u << "\n";
	append(u);
	return u;
      }
    else if (TR[u][0] < j)
      {
	//std::cout << "I am here mk( " << TR[u][0] << " , " << TR[u][1] << " , " << TR[u][2] << "\n";
	return mk(TR[u][0], res(TR[u][1]), res(TR[u][2]));
      }
    else if(TR[u][0] == j && b == 0)
      {
	//std::cout << "I am at TR[u][0] == j & b == 0... " << TR[u][1] << "\n";
	return res(TR[u][1]);
      }
    else
      {
	//std::cout << "I am at TR[u][0] == j & b == 1... " << TR[u][2] << "\n";
	return res(TR[u][2]);
      }
  }

  int satCount()
  {
    return pow(2, T[nodes - 1][0] - 1) * count(nodes - 1);
  }

  int count(int u)
  {
    if(u == 0)
      return 0;
    else if(u == 1)
      return 1;
    else
      return ( pow(2, T[ T[u][1] ][0] - T[u][0] - 1) * count(T[u][1]) ) + ( pow(2, T[ T[u][2] ][0] - T[u][0] - 1) * count(T[u][2]) );
  }

  void anySat()
  {
    //isSat = 1;
    satVars.resize(n);
    std::fill(satVars.begin(), satVars.end(), -1);

    isSat = findSat(nodes - 1);
    int flag = 0;
    if(isSat == 1)
      {
	//remaining -1s are don't cares
	for(auto i = satVars.begin(); i != satVars.end(); ++i)
	  {
	    if(*i != -1)
	      {
		flag = 1;
		break;
	      }
	  }
	std::replace(satVars.begin(), satVars.end(), -1, 0);
      }

    std::cout << "\n\nResult: ";

    if(flag == 0)
      std::cout << "Expression is not satisfiable.\n";
    else
      for(auto i = satVars.begin(); i != satVars.end(); ++i)
	std::cout << *i;

    std::cout << "\n\n";
    satVars.clear();

  }

  int findSat(int u)
  {
    if(u == 0)
      return -1;
    else if(u == 1)
      return 1;
    else
      if(T[u][1] == 0)
	{
	  satVars[ T[u][0] - 1 ] = 1;
	  findSat(T[u][2]);
	  return 1;
	}
      else
	{
	  satVars[ T[u][0] - 1 ] = 0;
	  findSat(T[u][1]);
	  return 1;
	}
  }

  
};





int main()
{
  //ROBDD u(2);
  //std::string exp = "and(not(or(and(x1,not(x2)),and(not(x1),x2))),not(or(and(x3,not(x4)),and(not(x3),x4))))";
  //std::string expEquiv = "equiv(and(not(or(and(x1,not(x2)),and(not(x1),x2))),not(or(and(x3,not(x4)),and(not(x3),x4))))" + "and(not(or(and(x1,not(x2)),and(not(x1),x2))),not(or(and(x3,not(x4)),and(not(x3),x4)))))";
  //std::string exp = "or(or(and(and(not(x1),not(x2)),not(x3)),and(x1,x2)),and(x2,x3))";
  //std::string exp = "or(or(x1,x2),and(x1,x2))";

  //u.build(exp, 1);
  //u.build(exp, 1);
  //std::cout << "The boolean expression:\t" << exp << "\n\n\n";
  //u.printT();

  //ROBDD u(3);
  //std::string exp = "or(or(not(x3),and(not(x1),not(x2))),and(x1,x2))";  //"and(x1,x2)";
  //std::string exp = "xor(xor(xor(xor(xor(xor(xor(xor(x1,x2),x3),x4),x5),x6),x7),x8),x9)";
  // std::string test =  "xor(a,xor(b,xor(c,xor(and(a, b),xor(and(a, c),xor(and(b, c),and(and(a,b),c)))))))";
  //std::string exp2 = "equiv(" + exp + "," + "x5" + ")";

  /*
  //Build Example 1
  std::string exp = "xor(xor(xor(xor(xor(xor(xor(xor(x1,x2),x3),x4),x5),x6),x7),x8),x9)";
  //std::string exp = "or(or(not(x3),and(not(x1),not(x2))),and(x1,x2))";
  //std::string exp = "and(not(or(and(x1,not(x2)),and(not(x1),x2))),not(or(and(x3,not(x4)),and(not(x3),x4))))";
  ROBDD u(9);
  u.build(exp, 1);
  u.printT();
  */
  
  /* Apply example 
  ROBDD u(2), u1(2);
  std::string exp = "not(or(and(x1,not(x2)),and(not(x1),x2)))";
  u1.build(exp, 1);
  u.apply("equiv", u1, u1);
  u.printT();

  */

  /* Apply example 2
  ROBDD u1(2), u2(2);
  std::string exp1 = "and(x1,not(x2))";
  u1.build(exp1, 1);

  std::string exp2 = "and(not(x1),x2)";
  u2.build(exp2, 1);

  ROBDD u(4);
  u.apply("or", u1, u2);
  u.printT();
  */


  /* Apply example 3
  ROBDD u1(3), u2(3);
  std::string exp1 = "and(and(not(and(not(x1),x2)),not(and(x1,not(x2)))),x3)";
  u1.build(exp1, 1);

  //std::string exp2 = "and(and(and(not(x1),x2),and(x1,not(x2))),x3)";
  std::string exp2 = "and(or(and(not(x1),x2),and(x1,not(x2))),not(x3))";
  u2.build(exp2, 1);

  ROBDD u(3);
  u.apply("or", u1, u2);
  u.printT();
  */

  // Apply example 4
  /*
  ROBDD u1(5), u2(5);
  std::string exp = "and(and(equiv(x1,x2),equiv(x3,x4)),not(x5))";
  std::string exp2 = "and(equiv(x1,x3),not(x5))";
  u1.build(exp, 1);
  u2.build(exp2, 1);
  u1.printT();
  u2.printT();
  ROBDD u(5);
  u.apply("and", u1, u2);
  u.printT();
  */
  
  /* Restrict Example 
  ROBDD u(3);
  std::string exp = "or(or(x3,and(not(x1),not(x2))),and(x1,x2))";  //"and(x1,x2)";
  u.build(exp, 1);
  u.printT();
  u.restrict(2, 0);
  u.printT();
  */
  

  /* Restrict Example 2
  ROBDD u(4);
  std::string exp = "and(not(or(and(x1,not(x2)),and(not(x1),x2))),not(or(and(x3,not(x4)),and(not(x3),x4))))";
  u.build(exp, 1);
  u.printT();
  u.restrict(3, 0);
  u.printT();
  */
  

  /*
  ROBDD u(3);
  std::string exp = "or(or(and(and(not(x1),not(x2)),not(x3)),and(x1,x2)),and(x2,x3))";
  u.build(exp, 1);
  u.printT();
  u.restrict(3, 0);
  u.printT();
  */

  //std::string exp = "and(not(x1),and(not(x2),x3))";
  //std::string exp = "or(or(or(x1,x2),x3),x4)";
  //std::string exp = "and(and(and(and(and(and(and(x1,x2),x3),x4),x5),x6),x7),x8)";
  //std::string exp = "xor(xor(xor(xor(xor(xor(xor(xor(x1,x2),x3),x4),x5),x6),x7),x8),x9)";
  //ROBDD u(9);
  //u.build(exp, 1);
  //u.printT();
  //std::cout << "satCount = " << u.satCount() << "\n";


  // std::string exp = "xor(xor(xor(xor(xor(xor(xor(xor(xor(xor(xor(xor(xor(xor(xor(xor(xor(xor(xor(xor(xor(x1,x2),x3),x4),x5),x6),x7),x8),x9),x10),x11),x12),x13),x14),x15),x16),x17),18),19),20),21),22)";
  //std::string exp = "xor(xor(xor(xor(xor(xor(xor(xor(xor(xor(xor(xor(xor(xor(x1,x2),x3),x4),x5),x6),x7),x8),x9),x10),x11),x12),x13),x14),x15)";
  //ROBDD u(15);
  //u.build(exp, 1);
  //u.printT();

  ROBDD u(9);
  //std::string exp = "and(and(and(x1,x2),x3),x4)";
  //std::string exp = "and(and(and(and(x1,x2),x3),x4),0)";
  std::string exp = "xor(xor(xor(xor(xor(xor(xor(xor(x1,x2),x3),x4),x5),x6),x7),x8),x9)";
  
  u.build(exp, 1);
  u.printT();
  u.anySat();
/*

const expEval::grammar<int, std::string::const_iterator> parser;

for(auto &expression : std::list<std::string> {
	// ~a.b + a.~b = (a + b).(~a + ~b)
	// XOR
	//"or(0,1)",
	//"and(1,0)",
	//"not(0)",
	//"a(",
	//"and(or(0,0),or(not(0),not(0)))",
	//"and(or(0,1),or(not(0),not(1)))",
	//"and(or(1,0),or(not(1),not(0)))",
	//"and(or(1,1),or(not(1),not(1)))",
	//"equiv(and(or(1,1),or(not(1),not(1))),and(or(1,1),or(not(1),not(1))))",   //equiv(exp1 == exp1)
	//"equiv(and(or(1,0),or(not(1),not(0))),0)"	  //equiv(exp1 != exp2)
	//"not(and(0,not(0)))",
	//"and(and(not(0),0),not(and(0,not(0))))",
	//"and(not(0),0)"
	} )
	{
		std::string::const_iterator start = expression.begin();  
		std::string::const_iterator end  = expression.end();
		int result;

		bool valid = qi::phrase_parse(start, end, parser, ascii::space, result);

		if(valid)
			std::cout <<result << "\n";
		else
			std::cout << "Error in expression\n";
	}
*/

  return 0;
}


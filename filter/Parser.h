

#if !defined(COCO_PARSER_H__)
#define COCO_PARSER_H__

#include <vector>
#include <string>

#include "FilterParser.h"


#include "Scanner.h"



class Errors {
public:
	int count;			// number of errors detected
	std::vector<std::wstring> errors;

	Errors();
	void SynErr(int n);
	void Error();
	void Warning();
	void Exception();

}; // Errors

class Parser {
private:
	enum {
		_EOF=0,
		_openblock=1,
		_closeblock=2,
		_ident=3,
		_stringliteral=4,
		_numliteral=5,
		_rangeliteral=6
	};
	int maxT;

	Token *dummyToken;
	int errDist;
	int minErrDist;

	void SynErr(int n);
	void Get();
	void Expect(int n);
	bool StartOf(int s);
	void ExpectWeak(int n, int follow);
	bool WeakSeparator(int n, int syFol, int repFol);

public:
	Scanner *scanner;
	Errors  *errors;

	Token *t;			// last recognized token
	Token *la;			// lookahead token

FilterParser * gen;



	Parser(Scanner *scanner);
	~Parser();
	void SemErr(const wchar_t* msg);

	void stringlit(char* &lit);
	void matchattrib(char* &name);
	void matchop(int &op);
	void logop(int &lop);
	void matchexpr();
	void blockexpr();
	void expr();
	void Filter();

	void Parse();

}; // end Parser



#endif




#include <wchar.h>
#include "Parser.h"
#include "Scanner.h"




void Parser::SynErr(int n) {
	if (errDist >= minErrDist) errors->SynErr(n);
	errDist = 0;
}

void Parser::SemErr(wchar_t* msg) {
	msg = msg;
	if (errDist >= minErrDist) errors->Error();
	errDist = 0;
}

void Parser::Get() {
	for (;;) {
		t = la;
		la = scanner->Scan();
		if (la->kind <= maxT) { ++errDist; break; }

		if (dummyToken != t) {
			dummyToken->kind = t->kind;
			dummyToken->pos = t->pos;
			dummyToken->col = t->col;
			dummyToken->line = t->line;
			dummyToken->next = NULL;
			coco_string_delete(dummyToken->val);
			dummyToken->val = coco_string_create(t->val);
			t = dummyToken;
		}
		la = t;
	}
}

void Parser::Expect(int n) {
	if (la->kind==n) Get(); else { SynErr(n); }
}

void Parser::ExpectWeak(int n, int follow) {
	if (la->kind == n) Get();
	else {
		SynErr(n);
		while (!StartOf(follow)) Get();
	}
}

bool Parser::WeakSeparator(int n, int syFol, int repFol) {
	if (la->kind == n) {Get(); return true;}
	else if (StartOf(repFol)) {return false;}
	else {
		SynErr(n);
		while (!(StartOf(syFol) || StartOf(repFol) || StartOf(0))) {
			Get();
		}
		return StartOf(syFol);
	}
}

void Parser::stringlit(char* &lit) {
		if (la->kind == 4) {
			Get();
		} else if (la->kind == 5) {
			Get();
		} else if (la->kind == 6) {
			Get();
		} else SynErr(22);
		lit = coco_string_create_char(t->val); 
}

void Parser::matchattrib(char* &name) {
		Expect(3);
		name = coco_string_create_char(t->val); 
}

void Parser::matchop(int &op) {
		switch (la->kind) {
		case 7: {
			Get();
			op = MATCHOP_EQ; 
			break;
		}
		case 8: {
			Get();
			op = MATCHOP_EQ; 
			break;
		}
		case 9: {
			Get();
			op = MATCHOP_NE; 
			break;
		}
		case 10: {
			Get();
			op = MATCHOP_RXEQ; 
			break;
		}
		case 11: {
			Get();
			op = MATCHOP_RXNE; 
			break;
		}
		case 12: {
			Get();
			op = MATCHOP_LT; 
			break;
		}
		case 13: {
			Get();
			op = MATCHOP_GT; 
			break;
		}
		case 14: {
			Get();
			op = MATCHOP_LE; 
			break;
		}
		case 15: {
			Get();
			op = MATCHOP_GE; 
			break;
		}
		case 16: {
			Get();
			op = MATCHOP_CONTAINS; 
			break;
		}
		case 17: {
			Get();
			op = MATCHOP_CONTAINSNOT; 
			break;
		}
		case 18: {
			Get();
			op = MATCHOP_BETWEEN; 
			break;
		}
		default: SynErr(23); break;
		}
}

void Parser::logop(int &lop) {
		if (la->kind == 19) {
			Get();
			lop = LOGOP_AND; 
		} else if (la->kind == 20) {
			Get();
			lop = LOGOP_OR; 
		} else SynErr(24);
}

void Parser::matchexpr() {
		char * name, * lit; int op; 
		matchattrib(name);
		matchop(op);
		stringlit(lit);
		gen->add_matchexpr(name, op, lit); 
}

void Parser::blockexpr() {
		Expect(1);
		gen->open_block(); 
		expr();
		Expect(2);
		gen->close_block(); 
}

void Parser::expr() {
		int lop; 
		if (la->kind == 3) {
			matchexpr();
		} else if (la->kind == 1) {
			blockexpr();
		} else SynErr(25);
		while (la->kind == 19 || la->kind == 20) {
			logop(lop);
			gen->add_logop(lop); 
			if (la->kind == 3) {
				matchexpr();
			} else if (la->kind == 1) {
				blockexpr();
			} else SynErr(26);
		}
}

void Parser::Filter() {
		expr();
}



void Parser::Parse() {
	t = NULL;
	la = dummyToken = new Token();
	la->val = coco_string_create(L"Dummy Token");
	Get();
	Filter();

	Expect(0);
}

Parser::Parser(Scanner *scanner) {
	maxT = 21;

	minErrDist = 2;
	errDist = minErrDist;
	this->scanner = scanner;
	errors = new Errors();
}

bool Parser::StartOf(int s) {
	const bool T = true;
	const bool x = false;

	static bool set[1][23] = {
		{T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x}
	};



	return set[s][la->kind];
}

Parser::~Parser() {
	delete errors;
	delete dummyToken;
}

Errors::Errors() {
	count = 0;
}

void Errors::SynErr(int n) {
	wchar_t* s;
	switch (n) {
			case 0: s = coco_string_create(L"EOF expected"); break;
			case 1: s = coco_string_create(L"openblock expected"); break;
			case 2: s = coco_string_create(L"closeblock expected"); break;
			case 3: s = coco_string_create(L"ident expected"); break;
			case 4: s = coco_string_create(L"stringliteral expected"); break;
			case 5: s = coco_string_create(L"numliteral expected"); break;
			case 6: s = coco_string_create(L"rangeliteral expected"); break;
			case 7: s = coco_string_create(L"\"==\" expected"); break;
			case 8: s = coco_string_create(L"\"=\" expected"); break;
			case 9: s = coco_string_create(L"\"!=\" expected"); break;
			case 10: s = coco_string_create(L"\"=~\" expected"); break;
			case 11: s = coco_string_create(L"\"!~\" expected"); break;
			case 12: s = coco_string_create(L"\"<\" expected"); break;
			case 13: s = coco_string_create(L"\">\" expected"); break;
			case 14: s = coco_string_create(L"\"<=\" expected"); break;
			case 15: s = coco_string_create(L"\">=\" expected"); break;
			case 16: s = coco_string_create(L"\"#\" expected"); break;
			case 17: s = coco_string_create(L"\"!#\" expected"); break;
			case 18: s = coco_string_create(L"\"between\" expected"); break;
			case 19: s = coco_string_create(L"\"and\" expected"); break;
			case 20: s = coco_string_create(L"\"or\" expected"); break;
			case 21: s = coco_string_create(L"??? expected"); break;
			case 22: s = coco_string_create(L"invalid stringlit"); break;
			case 23: s = coco_string_create(L"invalid matchop"); break;
			case 24: s = coco_string_create(L"invalid logop"); break;
			case 25: s = coco_string_create(L"invalid expr"); break;
			case 26: s = coco_string_create(L"invalid expr"); break;

		default:
		{
			wchar_t format[20];
			coco_swprintf(format, 20, L"error %d", n);
			s = coco_string_create(format);
		}
		break;
	}
	errors.push_back(std::wstring(s));
	coco_string_delete(s);
	count++;
}

void Errors::Error() {
	count++;
}

void Errors::Warning() {
}

void Errors::Exception() {
}






#include <wchar.h>
#include "Parser.h"
#include "Scanner.h"




void Parser::SynErr(int n) {
	if (errDist >= minErrDist) errors->SynErr(n);
	errDist = 0;
}

void Parser::SemErr(const wchar_t* msg) {
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
		if (la->kind == _stringliteral) {
			Get();
		} else if (la->kind == _numliteral) {
			Get();
		} else if (la->kind == _rangeliteral) {
			Get();
		} else SynErr(22);
		lit = coco_string_create_char(t->val); 
}

void Parser::matchattrib(char* &name) {
		Expect(_ident);
		name = coco_string_create_char(t->val); 
}

void Parser::matchop(int &op) {
		switch (la->kind) {
		case 7 /* "==" */: {
			Get();
			op = MATCHOP_EQ; 
			break;
		}
		case 8 /* "=" */: {
			Get();
			op = MATCHOP_EQ; 
			break;
		}
		case 9 /* "!=" */: {
			Get();
			op = MATCHOP_NE; 
			break;
		}
		case 10 /* "=~" */: {
			Get();
			op = MATCHOP_RXEQ; 
			break;
		}
		case 11 /* "!~" */: {
			Get();
			op = MATCHOP_RXNE; 
			break;
		}
		case 12 /* "<" */: {
			Get();
			op = MATCHOP_LT; 
			break;
		}
		case 13 /* ">" */: {
			Get();
			op = MATCHOP_GT; 
			break;
		}
		case 14 /* "<=" */: {
			Get();
			op = MATCHOP_LE; 
			break;
		}
		case 15 /* ">=" */: {
			Get();
			op = MATCHOP_GE; 
			break;
		}
		case 16 /* "#" */: {
			Get();
			op = MATCHOP_CONTAINS; 
			break;
		}
		case 17 /* "!#" */: {
			Get();
			op = MATCHOP_CONTAINSNOT; 
			break;
		}
		case 18 /* "between" */: {
			Get();
			op = MATCHOP_BETWEEN; 
			break;
		}
		default: SynErr(23); break;
		}
}

void Parser::logop(int &lop) {
		if (la->kind == 19 /* "and" */) {
			Get();
			lop = LOGOP_AND; 
		} else if (la->kind == 20 /* "or" */) {
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
		Expect(_openblock);
		gen->open_block(); 
		expr();
		Expect(_closeblock);
		gen->close_block(); 
}

void Parser::expr() {
		int lop; 
		if (la->kind == _ident) {
			matchexpr();
		} else if (la->kind == _openblock) {
			blockexpr();
		} else SynErr(25);
		while (la->kind == 19 /* "and" */ || la->kind == 20 /* "or" */) {
			logop(lop);
			gen->add_logop(lop); 
			if (la->kind == _ident) {
				matchexpr();
			} else if (la->kind == _openblock) {
				blockexpr();
			} else SynErr(26);
		}
}

void Parser::Filter() {
		expr();
}




// If the user declared a method Init and a mehtod Destroy they should
// be called in the contructur and the destructor respctively.
//
// The following templates are used to recognize if the user declared
// the methods Init and Destroy.

template<typename T>
struct ParserInitExistsRecognizer {
	template<typename U, void (U::*)() = &U::Init>
	struct ExistsIfInitIsDefinedMarker{};

	struct InitIsMissingType {
		char dummy1;
	};
	
	struct InitExistsType {
		char dummy1; char dummy2;
	};

	// exists always
	template<typename U>
	static InitIsMissingType is_here(...);

	// exist only if ExistsIfInitIsDefinedMarker is defined
	template<typename U>
	static InitExistsType is_here(ExistsIfInitIsDefinedMarker<U>*);

	enum { InitExists = (sizeof(is_here<T>(NULL)) == sizeof(InitExistsType)) };
};

template<typename T>
struct ParserDestroyExistsRecognizer {
	template<typename U, void (U::*)() = &U::Destroy>
	struct ExistsIfDestroyIsDefinedMarker{};

	struct DestroyIsMissingType {
		char dummy1;
	};
	
	struct DestroyExistsType {
		char dummy1; char dummy2;
	};

	// exists always
	template<typename U>
	static DestroyIsMissingType is_here(...);

	// exist only if ExistsIfDestroyIsDefinedMarker is defined
	template<typename U>
	static DestroyExistsType is_here(ExistsIfDestroyIsDefinedMarker<U>*);

	enum { DestroyExists = (sizeof(is_here<T>(NULL)) == sizeof(DestroyExistsType)) };
};

// The folloing templates are used to call the Init and Destroy methods if they exist.

// Generic case of the ParserInitCaller, gets used if the Init method is missing
template<typename T, bool = ParserInitExistsRecognizer<T>::InitExists>
struct ParserInitCaller {
	static void CallInit(T *t) {
		// nothing to do
	}
};

// True case of the ParserInitCaller, gets used if the Init method exists
template<typename T>
struct ParserInitCaller<T, true> {
	static void CallInit(T *t) {
		t->Init();
	}
};

// Generic case of the ParserDestroyCaller, gets used if the Destroy method is missing
template<typename T, bool = ParserDestroyExistsRecognizer<T>::DestroyExists>
struct ParserDestroyCaller {
	static void CallDestroy(T *t) {
		// nothing to do
	}
};

// True case of the ParserDestroyCaller, gets used if the Destroy method exists
template<typename T>
struct ParserDestroyCaller<T, true> {
	static void CallDestroy(T *t) {
		t->Destroy();
	}
};

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

	ParserInitCaller<Parser>::CallInit(this);
	dummyToken = NULL;
	t = la = NULL;
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
	ParserDestroyCaller<Parser>::CallDestroy(this);
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
}

void Errors::Warning() {
}

void Errors::Exception() {
}



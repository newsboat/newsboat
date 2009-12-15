

#include <memory.h>
#include <string.h>
#include "Scanner.h"

// string handling, wide character

wchar_t* coco_string_create(const wchar_t* value) {
	wchar_t* data;
	int len = 0;
	if (value) { len = wcslen(value); }
	data = new wchar_t[len + 1];
	wcsncpy(data, value, len);
	data[len] = 0;
	return data;
}

wchar_t* coco_string_create(const wchar_t *value , int startIndex, int length) {
	int len = 0;
	wchar_t* data;

	if (value) { len = length; }
	data = new wchar_t[len + 1];
	wcsncpy(data, &(value[startIndex]), len);
	data[len] = 0;

	return data;
}

wchar_t* coco_string_create_upper(wchar_t* data) {
	if (!data) { return NULL; }

	int dataLen = 0;
	if (data) { dataLen = wcslen(data); }

	wchar_t *newData = new wchar_t[dataLen + 1];

	for (int i = 0; i <= dataLen; i++) {
		if ((L'a' <= data[i]) && (data[i] <= L'z')) {
			newData[i] = data[i] + (L'A' - L'a');
		}
		else { newData[i] = data[i]; }
	}

	newData[dataLen] = L'\0';
	return newData;
}

wchar_t* coco_string_create_lower(wchar_t* data) {
	if (!data) { return NULL; }

	int dataLen = 0;
	if (data) { dataLen = wcslen(data); }

	wchar_t* newData = new wchar_t[dataLen + 1];

	for (int i = 0; i <= dataLen; i++) {
		if ((L'A' <= data[i]) && (data[i] <= L'Z')) {
			newData[i] = data[i] - (L'A'- L'a');
		}
		else { newData[i] = data[i]; }
	}
	newData[dataLen] = L'\0';
	return newData;
}

wchar_t* coco_string_create_append(const wchar_t* data1, const wchar_t* data2) {
	wchar_t* data;
	int data1Len = 0;
	int data2Len = 0;

	if (data1) { data1Len = wcslen(data1); }
	if (data2) {data2Len = wcslen(data2); }

	data = new wchar_t[data1Len + data2Len + 1];

	if (data1) { wcscpy(data, data1); }
	if (data2) { wcscpy(data + data1Len, data2); }

	data[data1Len + data2Len] = 0;

	return data;
}

wchar_t* coco_string_create_append(const wchar_t *target, const wchar_t appendix) {
	int targetLen = coco_string_length(target);
	wchar_t* data = new wchar_t[targetLen + 2];
	wcsncpy(data, target, targetLen);
	data[targetLen] = appendix;
	data[targetLen + 1] = 0;
	return data;
}

void coco_string_delete(wchar_t* &data) {
	delete [] data;
	data = NULL;
}

int coco_string_length(const wchar_t* data) {
	if (data) { return wcslen(data); }
	return 0;
}

bool coco_string_endswith(wchar_t* data, wchar_t *end) {
	int dataLen = wcslen(data);
	int endLen = wcslen(end);
	return (endLen <= dataLen) && (wcscmp(data + dataLen - endLen, end) == 0);
}

int coco_string_indexof(wchar_t* data, wchar_t value) {
	wchar_t* chr = wcschr(data, value);

	if (chr) { return (chr-data); }
	return -1;
}

int coco_string_lastindexof(wchar_t* data, wchar_t value) {
	wchar_t* chr = wcsrchr(data, value);

	if (chr) { return (chr-data); }
	return -1;
}

void coco_string_merge(wchar_t* &target, wchar_t* appendix) {
	if (!appendix) { return; }
	wchar_t* data = coco_string_create_append(target, appendix);
	delete [] target;
	target = data;
}

bool coco_string_equal(wchar_t* data1, wchar_t* data2) {
	return wcscmp( data1, data2 ) == 0;
}

int coco_string_compareto(wchar_t* data1, wchar_t* data2) {
	return wcscmp(data1, data2);
}

int coco_string_hash(const wchar_t *data) {
	int h = 0;
	if (!data) { return 0; }
	while (*data != 0) {
		h = (h * 7) ^ *data;
		++data;
	}
	if (h < 0) { h = -h; }
	return h;
}

// string handling, ascii character

wchar_t* coco_string_create(const char* value) {
	int len = 0;
	if (value) { len = strlen(value); }
	wchar_t* data = new wchar_t[len + 1];
	for (int i = 0; i < len; ++i) { data[i] = (wchar_t) value[i]; }
	data[len] = 0;
	return data;
}

char* coco_string_create_char(const wchar_t *value) {
	int len = coco_string_length(value);
	char *res = new char[len + 1];
	for (int i = 0; i < len; ++i) { res[i] = (char) value[i]; }
	res[len] = 0;
	return res;
}

void coco_string_delete(char* &data) {
	delete [] data;
	data = NULL;
}




Token::Token() {
	kind = 0;
	pos  = 0;
	col  = 0;
	line = 0;
	val  = NULL;
	next = NULL;
}

Token::~Token() {
	coco_string_delete(val);
}


Buffer::Buffer(std::istream* s, bool isUserStream) {
	stream = s; this->isUserStream = isUserStream;
	s->seekg(0, std::ios_base::end);
	fileLen = bufLen = s->tellg();
	s->seekg(0, std::ios_base::beg);
	buf = new char[MAX_BUFFER_LENGTH];
	bufStart = INT_MAX; // nothing in the buffer so far
	SetPos(0);          // setup  buffer to position 0 (start)
	if (bufLen == fileLen) Close();
}

Buffer::Buffer(Buffer *b) {
	buf = b->buf;
	b->buf = NULL;
	bufStart = b->bufStart;
	bufLen = b->bufLen;
	fileLen = b->fileLen;
	pos = b->pos;
	stream = b->stream;
	b->stream = NULL;
	isUserStream = b->isUserStream;
}

Buffer::~Buffer() {
	Close(); 
	if (buf != NULL) {
		delete [] buf;
		buf = NULL;
	}
}

void Buffer::Close() {
	if (!isUserStream && stream != NULL) {
		std::ifstream * ifs = dynamic_cast<std::ifstream*>(stream);
		if (ifs) {
			ifs->close();
			delete ifs;
		}
		stream = NULL;
	}
}

int Buffer::Read() {
	if (pos < bufLen) {
		return buf[pos++];
	} else if (GetPos() < fileLen) {
		SetPos(GetPos()); // shift buffer start to Pos
		return buf[pos++];
	} else {
		return EoF;
	}
}

int Buffer::Peek() {
	int curPos = GetPos();
	int ch = Read();
	SetPos(curPos);
	return ch;
}

char* Buffer::GetString(int beg, int end) {
	int len = end - beg;
	char *buf = new char[len];
	int oldPos = GetPos();
	SetPos(beg);
	for (int i = 0; i < len; ++i) buf[i] = (char) Read();
	SetPos(oldPos);
	return buf;
}

int Buffer::GetPos() {
	return pos + bufStart;
}

void Buffer::SetPos(int value) {
	if (value < 0) value = 0;
	else if (value > fileLen) value = fileLen;
	if (value >= bufStart && value < bufStart + bufLen) { // already in buffer
		pos = value - bufStart;
	} else if (stream != NULL) { // must be swapped in
		stream->seekg(0, std::ios_base::beg);
		stream->get(buf, MAX_BUFFER_LENGTH);
		bufLen = stream->gcount();
		bufStart = value; pos = 0;
	} else {
		pos = fileLen - bufStart; // make Pos return fileLen
	}
}

int UTF8Buffer::Read() {
	int ch;
	do {
		ch = Buffer::Read();
		// until we find a uft8 start (0xxxxxxx or 11xxxxxx)
	} while ((ch >= 128) && ((ch & 0xC0) != 0xC0) && (ch != EOF));
	if (ch < 128 || ch == EOF) {
		// nothing to do, first 127 chars are the same in ascii and utf8
		// 0xxxxxxx or end of file character
	} else if ((ch & 0xF0) == 0xF0) {
		// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		int c1 = ch & 0x07; ch = Buffer::Read();
		int c2 = ch & 0x3F; ch = Buffer::Read();
		int c3 = ch & 0x3F; ch = Buffer::Read();
		int c4 = ch & 0x3F;
		ch = (((((c1 << 6) | c2) << 6) | c3) << 6) | c4;
	} else if ((ch & 0xE0) == 0xE0) {
		// 1110xxxx 10xxxxxx 10xxxxxx
		int c1 = ch & 0x0F; ch = Buffer::Read();
		int c2 = ch & 0x3F; ch = Buffer::Read();
		int c3 = ch & 0x3F;
		ch = (((c1 << 6) | c2) << 6) | c3;
	} else if ((ch & 0xC0) == 0xC0) {
		// 110xxxxx 10xxxxxx
		int c1 = ch & 0x1F; ch = Buffer::Read();
		int c2 = ch & 0x3F;
		ch = (c1 << 6) | c2;
	}
	return ch;
}

Scanner::Scanner(const wchar_t* fileName) {
	char *chFileName = coco_string_create_char(fileName);
	std::ifstream* ifs = new std::ifstream(chFileName);
	if (!ifs || !ifs->is_open()) {
		wprintf(L"--- Cannot open file %ls\n", fileName);
		exit(1);
	}
	coco_string_delete(chFileName);
	buffer = new Buffer(ifs, false);
	Init();
}
	
Scanner::Scanner(std::istream& s) {
	buffer = new Buffer(&s, true);
	Init();
}

Scanner::~Scanner() {
	char* cur = (char*) firstHeap;

	while(cur != NULL) {
		cur = *(char**) (cur + HEAP_BLOCK_SIZE);
		free(firstHeap);
		firstHeap = cur;
	}
	delete [] tval;
	delete buffer;
}

void Scanner::Init() {
	EOL    = '\n';
	eofSym = 0;
	maxT = 21;
	noSym = 21;
	int i;
	for (i = 46; i <= 46; ++i) start.set(i, 3);
	for (i = 65; i <= 90; ++i) start.set(i, 3);
	for (i = 95; i <= 95; ++i) start.set(i, 3);
	for (i = 97; i <= 122; ++i) start.set(i, 3);
	for (i = 48; i <= 57; ++i) start.set(i, 9);
	start.set(40, 1);
	start.set(41, 2);
	start.set(34, 4);
	start.set(45, 10);
	start.set(61, 19);
	start.set(33, 20);
	start.set(60, 21);
	start.set(62, 22);
	start.set(35, 17);
		start.set(Buffer::EoF, -1);
	keywords.set(L"between", 18);
	keywords.set(L"and", 19);
	keywords.set(L"or", 20);


	tvalLength = 128;
	tval = new wchar_t[tvalLength]; // text of current token

	// HEAP_BLOCK_SIZE byte heap + pointer to next heap block
	heap = malloc(HEAP_BLOCK_SIZE + sizeof(void*));
	firstHeap = heap;
	heapEnd = (void**) (((char*) heap) + HEAP_BLOCK_SIZE);
	*heapEnd = 0;
	heapTop = heap;
	if (sizeof(Token) > HEAP_BLOCK_SIZE) {
		wprintf(L"--- Too small HEAP_BLOCK_SIZE\n");
		exit(1);
	}

	pos = -1; line = 1; col = 0;
	oldEols = 0;
	NextCh();
	if (ch == 0xEF) { // check optional byte order mark for UTF-8
		NextCh(); int ch1 = ch;
		NextCh(); int ch2 = ch;
		if (ch1 != 0xBB || ch2 != 0xBF) {
			wprintf(L"Illegal byte order mark at start of file");
			exit(1);
		}
		Buffer *oldBuf = buffer;
		buffer = new UTF8Buffer(buffer); col = 0;
		delete oldBuf; oldBuf = NULL;
		NextCh();
	}


	pt = tokens = CreateToken(); // first token is a dummy
}

void Scanner::NextCh() {
	if (oldEols > 0) { ch = EOL; oldEols--; } 
	else {
		pos = buffer->GetPos();
		ch = buffer->Read(); col++;
		// replace isolated '\r' by '\n' in order to make
		// eol handling uniform across Windows, Unix and Mac
		if (ch == L'\r' && buffer->Peek() != L'\n') ch = EOL;
		if (ch == EOL) { line++; col = 0; }
	}

}

void Scanner::AddCh() {
	if (tlen >= tvalLength) {
		tvalLength *= 2;
		wchar_t *newBuf = new wchar_t[tvalLength];
		memcpy(newBuf, tval, tlen*sizeof(wchar_t));
		delete tval;
		tval = newBuf;
	}
		tval[tlen++] = ch;
	NextCh();
}



void Scanner::CreateHeapBlock() {
	void* newHeap;
	char* cur = (char*) firstHeap;

	while(((char*) tokens < cur) || ((char*) tokens > (cur + HEAP_BLOCK_SIZE))) {
		cur = *((char**) (cur + HEAP_BLOCK_SIZE));
		free(firstHeap);
		firstHeap = cur;
	}

	// HEAP_BLOCK_SIZE byte heap + pointer to next heap block
	newHeap = malloc(HEAP_BLOCK_SIZE + sizeof(void*));
	*heapEnd = newHeap;
	heapEnd = (void**) (((char*) newHeap) + HEAP_BLOCK_SIZE);
	*heapEnd = 0;
	heap = newHeap;
	heapTop = heap;
}

Token* Scanner::CreateToken() {
	Token *t;
	if (((char*) heapTop + (int) sizeof(Token)) >= (char*) heapEnd) {
		CreateHeapBlock();
	}
	t = (Token*) heapTop;
	heapTop = (void*) ((char*) heapTop + sizeof(Token));
	t->val = NULL;
	t->next = NULL;
	return t;
}

void Scanner::AppendVal(Token *t) {
	int reqMem = (tlen + 1) * sizeof(wchar_t);
	if (((char*) heapTop + reqMem) >= (char*) heapEnd) {
		if (reqMem > HEAP_BLOCK_SIZE) {
			wprintf(L"--- Too long token value\n");
			exit(1);
		}
		CreateHeapBlock();
	}
	t->val = (wchar_t*) heapTop;
	heapTop = (void*) ((char*) heapTop + reqMem);

	wcsncpy(t->val, tval, tlen);
	t->val[tlen] = L'\0';
}

Token* Scanner::NextToken() {
	while (ch == ' ' ||
			false
	) NextCh();

	t = CreateToken();
	t->pos = pos; t->col = col; t->line = line; 
	int state = start.state(ch);
	tlen = 0; AddCh();

	switch (state) {
		case -1: { t->kind = eofSym; break; } // NextCh already done
		case 0: { t->kind = noSym; break; }   // NextCh already done
		case 1:
			{t->kind = 1; break;}
		case 2:
			{t->kind = 2; break;}
		case 3:
			case_3:
			if ((ch >= L'-' && ch <= L'.') || (ch >= L'A' && ch <= L'Z') || ch == L'_' || (ch >= L'a' && ch <= L'z')) {AddCh(); goto case_3;}
			else {t->kind = 3; wchar_t *literal = coco_string_create(tval, 0, tlen); t->kind = keywords.get(literal, t->kind); coco_string_delete(literal); break;}
		case 4:
			case_4:
			if (ch <= L'!' || (ch >= L'#' && ch <= 65535)) {AddCh(); goto case_4;}
			else if (ch == L'"') {AddCh(); goto case_5;}
			else {t->kind = noSym; break;}
		case 5:
			case_5:
			{t->kind = 4; break;}
		case 6:
			case_6:
			if ((ch >= L'0' && ch <= L'9')) {AddCh(); goto case_6;}
			else {t->kind = 5; break;}
		case 7:
			case_7:
			if ((ch >= L'0' && ch <= L'9')) {AddCh(); goto case_8;}
			else {t->kind = noSym; break;}
		case 8:
			case_8:
			if ((ch >= L'0' && ch <= L'9')) {AddCh(); goto case_8;}
			else {t->kind = 6; break;}
		case 9:
			case_9:
			if ((ch >= L'0' && ch <= L'9')) {AddCh(); goto case_9;}
			else if (ch == L':') {AddCh(); goto case_7;}
			else {t->kind = 5; break;}
		case 10:
			if ((ch >= L'-' && ch <= L'.') || (ch >= L'A' && ch <= L'Z') || ch == L'_' || (ch >= L'a' && ch <= L'z')) {AddCh(); goto case_3;}
			else if ((ch >= L'0' && ch <= L'9')) {AddCh(); goto case_6;}
			else {t->kind = 3; wchar_t *literal = coco_string_create(tval, 0, tlen); t->kind = keywords.get(literal, t->kind); coco_string_delete(literal); break;}
		case 11:
			case_11:
			{t->kind = 7; break;}
		case 12:
			case_12:
			{t->kind = 9; break;}
		case 13:
			case_13:
			{t->kind = 10; break;}
		case 14:
			case_14:
			{t->kind = 11; break;}
		case 15:
			case_15:
			{t->kind = 14; break;}
		case 16:
			case_16:
			{t->kind = 15; break;}
		case 17:
			{t->kind = 16; break;}
		case 18:
			case_18:
			{t->kind = 17; break;}
		case 19:
			if (ch == L'=') {AddCh(); goto case_11;}
			else if (ch == L'~') {AddCh(); goto case_13;}
			else {t->kind = 8; break;}
		case 20:
			if (ch == L'=') {AddCh(); goto case_12;}
			else if (ch == L'~') {AddCh(); goto case_14;}
			else if (ch == L'#') {AddCh(); goto case_18;}
			else {t->kind = noSym; break;}
		case 21:
			if (ch == L'=') {AddCh(); goto case_15;}
			else {t->kind = 12; break;}
		case 22:
			if (ch == L'=') {AddCh(); goto case_16;}
			else {t->kind = 13; break;}

	}
	AppendVal(t);
	return t;
}

// get the next token (possibly a token already seen during peeking)
Token* Scanner::Scan() {
	if (tokens->next == NULL) {
		return pt = tokens = NextToken();
	} else {
		pt = tokens = tokens->next;
		return tokens;
	}
}

// peek for the next token, ignore pragmas
Token* Scanner::Peek() {
	if (pt->next == NULL) {
		do {
			pt = pt->next = NextToken();
		} while (pt->kind > maxT); // skip pragmas
	} else {
		do {
			pt = pt->next; 
		} while (pt->kind > maxT);
	}
	return pt;
}

// make sure that peeking starts at the current scan position
void Scanner::ResetPeek() {
	pt = tokens;
}




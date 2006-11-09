#include <rss.h>
#include <raptor.h>

using namespace noos;

static void do_handle(void* user_data, const raptor_statement* triple) {
	rss_handler * h = static_cast<rss_handler *>(user_data);
	if (h) {
		h->handle(triple);
	}
}

rss_uri::rss_uri(const char * uri_string) : uri_str(uri_string) {
	unsigned char * uris = raptor_uri_filename_to_uri_string(uri_str.c_str());
	uri = raptor_new_uri(uris);
	raptor_free_memory(uris);
}

rss_uri::~rss_uri() {
	raptor_free_uri(uri);
}

rss_parser::rss_parser(const rss_uri& uri) : my_uri(uri), handler(0) {
	parser = raptor_new_parser("guess"); // or rdfxml?
}

rss_parser::~rss_parser() {
	raptor_free_parser(parser);
}

void rss_parser::set_handler(rss_handler * h) {
	handler = h;
}

void rss_parser::parse() {
	raptor_set_statement_handler(parser, handler, do_handle);
	raptor_parse_file(parser, my_uri.uri, NULL);
}

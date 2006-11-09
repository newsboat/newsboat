#ifndef NOOS_RSS__H
#define NOOS_RSS__H

#include <string>

#include <raptor.h>

namespace noos {

	class rss_parser;
	class rss_handler;

	class rss_uri {
		public:
			rss_uri(const char * uri_string = NULL);
			~rss_uri();
		private:
			raptor_uri *uri;
			std::string uri_str;

		friend class rss_parser;
	};

	class rss_parser {
		public:
			rss_parser(const rss_uri& uri);
			~rss_parser();
			void parse();
			void set_handler(rss_handler * h);
		private:
			raptor_parser * parser;
			const rss_uri& my_uri;
			rss_handler * handler;
	};

	class rss_handler {
		public:
			rss_handler() { }
			virtual void handle(const raptor_statement* ) = 0;
	};

}


#endif

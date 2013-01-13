/*================
* copy the BasicDocStream class
* NAME DATE - COMMENTS
* wy  02/2012 - created
*/
#ifndef _BASICLINKDOCSTREAM_HPP
#define _BASICLINKDOCSTREAM_HPP


#include "common_headers.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include "DocStream.hpp"
#include "Exception.hpp"
#include "Link.hpp"

namespace link
{
	namespace parse
	{
#define MAXLINE 65536


		/// doc representation for BasicDocStream

		class BasicLinkDoc : public lemur::api::Document 
		{
		public:
			BasicLinkDoc() {}
			BasicLinkDoc(ifstream *stream): docStr(stream) {}
			void startTermIteration() const;  

			const char *getID() const { return id;}

			bool hasMore() const{ return (strcmp(curWord, "</DOC>") != 0);}

			const link::api::Link * nextLink() const;
			const lemur::api::Term * nextTerm() const{return NULL;}

			void skipToEnd() const;
			friend class BasicLinkDocStream;

		private:
			void readID(); 
			mutable char *curWord;
			mutable char buf1[20000];
			mutable char buf2[20000];
			char id[2000];
			ifstream *docStr;
			streampos startPos; // starting position of the terms in the file
			//replace  static BasicTokenTerm t; with attribute
			mutable link::api::Link li;
		};


		/// A DocStream handler of a stream with the basic lemur format
		class BasicLinkDocStream : public lemur::api::DocStream
		{
		public:
			BasicLinkDocStream() {}
			BasicLinkDocStream (const string &inputFile);

			virtual ~BasicLinkDocStream() {  delete ifs;}

		public:

			bool hasMore(); 

			void startDocIteration();

			lemur::api::Document *nextDoc();

		private:
			char file[1024];
			ifstream *ifs;
			char buf[2000];
			bool nextTokenRead;
			// replace static BasicTokenDoc doc;  with attribute
			BasicLinkDoc doc;
		};
	}
}

#endif
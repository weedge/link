/**======================================
/*
 * NAME DATE - COMMENTS
 * wy 05/2011 - created
 *========================================================================*/
#ifndef CANDIDATELINK_FILE_HPP
#define CANDIDATELINK_FILE_HPP

#include "common_headers.hpp"
#include "Link.hpp"
#include "IndexTypes.hpp"
#include "Index.hpp"
#include "LinkTypes.hpp"
#include <algorithm>

namespace link{
	namespace file{
		class CandidateLinkFile{
		// write Candidated Link to FileStream, and read the LinkFile to build the candidatelink index ， U can see the ParseToFile
		public:

			CandidateLinkFile(bool LINKFormat = false);

			~CandidateLinkFile(){}

			/// Open and associate an input stream for reading
			void openForRead(istream &is, lemur::api::Index *index=NULL){
				inStr = &is;
				inStr->seekg(0,ios::beg);
				ind = index;
				eof = !readLine();
			}
			/// Open and associate an input stream for reading to a vector.
			void openForReadtoVec(istream &is, lemur::api::Index *index=NULL){
				inStr = &is;
				inStr->seekg(0,ios::beg);
				ind = index;
				readToVector();
			}

			/// 给定一个link,查找文件中是否是候选link.
			bool findLink(const string& exceptedLink);


			/// Associate an output stream for writing results
			void openForWrite( ostream &os, lemur::api::Index *index) {
				outStr = &os;
				ind = index;
			}

			//parse to DocStream format to read
			const char * beginDoc(const char *docID)
			{
				// begin the doc
				*outStr << "<DOC " << docID << ">" << endl;
				return docID;
			}
			//parse to DocStream format to read
			void endDoc()
			{
				*outStr<<"</DOC>"<<endl;
			}

			/// writing the results(CandlinkInfo)(stored in <tt> results</tt>) into the associated output stream.
			void writeResults(const link::api::Link &li);

		private:
			link::api::Link candLink;

			bool readLine();

			/// read the pairID into the vector
			void readToVector();

			/*
			 * if true,format:"termid-termid term-term ";
			 * if false,format:"termid-termid ".
			*/
			bool linkFmt;

			link::api::LINK_STR curPairID;//termID-termID

			std::set<link::api::LINK_STR> PairIDs;//set容器存放pairID(termID-termID)

			lemur::api::Index *ind;
			istream *inStr;
			ostream *outStr;
			bool eof;
		

		};
	}
}

#endif //CANDIDATELINK_FILE_HPP
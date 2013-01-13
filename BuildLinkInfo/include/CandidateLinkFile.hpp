/**======================================
/*
 * NAME DATE - COMMENTS
 * wy 05/2011 - created for write/read the candidate link  to/from the candidate link file~!
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
		// write Candidated Link to FileStream, and read the LinkFile to build the candidatelink index 
		class CandidateLinkFile{
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

			/// writing the results(CandlinkInfo)(stored in <tt> results</tt>) into the associated output stream.
			void writeResults(const link::api::Link &li);

			void startIterator()
			{
				
				//iter = PairIDs.begin();
			}
			bool hasMore()
			{
				
				return !eof;
				//return (iter!=PairIDs.end());
			}
			link::api::LINK_STR nextLink()
			{
				
				return curPairID;
				//return (*iter++);
			}
			void nextLine()
			{
				eof = !readLine();
			}

			/// parse to DocStream format to read
			const char * beginDoc(const char *docID)
			{
				// begin the doc
				*outStr << "<DOC " << docID << ">" << endl;
				return docID;
			}
			/// parse to DocStream format to read
			void endDoc()
			{
				*outStr<<"</DOC>"<<endl;
			}

		protected:

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

			std::set<link::api::LINK_STR> PairIDs;//set insert the pairID(termID-termID) to read the candidate link from file
			std::set<link::api::LINK_STR>::const_iterator iter;

			lemur::api::Index *ind;
			istream *inStr;
			ostream *outStr;
			bool eof;
		

		};
	}
}

#endif //CANDIDATELINK_FILE_HPP
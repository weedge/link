/**======================================
/*
 * NAME DATE - COMMENTS
 * wy 03/2011 - created
 *========================================================================*/
#ifndef INV_LINKINFOLIST_HPP
#define INV_LINKINFOLIST_HPP


#include "LinkTypes.hpp"
#include "IndexTypes.hpp"
#include "LinkInfo.hpp"
#include "File.hpp"
#include <vector>

namespace link{
	namespace api{
		class InvLinkInfoList{
		public:
			InvLinkInfoList();
			InvLinkInfoList(lemur::api::DOCID_T docid, int linkCount, std::vector<LinkedTerm> &lts);
			~InvLinkInfoList();

			/// Write a compressed LinkInfoList object to a file
			void binWriteC( lemur::file::File& outfile );
			/// delta encode linkids and distances from begin through end. call before write
			virtual void deltaEncode();
			/// delta decode linkids and distances from begin through end. call after read
			virtual void deltaDecode();

			/// Read a compressed LinkInfoList object to decompress from a file
			bool binReadC(lemur::file::File& infile);

		protected:

			lemur::api::DOCID_T uid; // this doc's id
			lemur::api::COUNT_T linkcount;  // the number of link of this document (terms + stopwords,if parameters countStopWords is true)
			LinkedTerm* linklist; // list of linkIDs and dists
			LDTerm* linklistcounted; // list of linkIDs and dist lists
			lemur::api::COUNT_T linklistlen; // number of items we have in list  (same as number of linkIDs)
			mutable int index;   // index for iterator
			lemur::api::LOC_T* counts; // keep track of counts of terms for bag of word
			mutable LinkInfo entry;
			mutable std::vector<link::api::DIST_T> distlist; //list of distances to return
		
		};
	}
}


#endif //INV_LINKINFOLIST_HPP
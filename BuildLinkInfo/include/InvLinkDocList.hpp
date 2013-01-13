/**======================================
/*
 * NAME DATE - COMMENTS
 * wy 02/2011 - created
 *========================================================================*/
#ifndef INV_LINKDOCLIST_HPP
#define INV_LINKDOCLIST_HPP

#include "InvDocList.hpp"
#include "LinkTypes.hpp"
#include "IndexTypes.hpp"
#include "LinkInfo.hpp"
#include "InvLinkDocInfo.hpp"
#include <vector>
#include <cmath>

namespace link{
	namespace api{

		class InvLinkDocList : public lemur::index::InvDocList
		{	
		public:
			InvLinkDocList();
			InvLinkDocList(const link::api::LINKID_T &lid, const std::vector<int> &winsizes);

			/// copy from byteVector to decompress linkdoclist;
			InvLinkDocList(lemur::api::LOC_T *vec,const std::vector<int> &winsizes);
			InvLinkDocList(lemur::api::LOC_T *vec);

			~InvLinkDocList();

			link::api::LINKID_T linkID() const{ return ulid; };

			virtual bool addFrequence(lemur::api::DOCID_T docid, link::api::DIST_T dist);
				
			lemur::api::LOC_T* compInvLinkDocList(lemur::api::COUNT_T &vecLength);

			///get the counts of the link in the collection
			link::api::LINKCOUNT_T linkCF() const;
			///get the counts of the link in the collection under the winSize
			link::api::LINKCOUNT_T linkCF(int winSize) const;

			lemur::api::DocInfo* nextEntry() const;
			//void nextEntry(lemur::api::DocInfo* info) const;
			//void nextEntry(InvFPDocInfo* info) const;

			///append the invLinkDocList in its tail.
			bool append(lemur::index::InvDocList* tail);

		protected:
			bool getMoreMemory();
			/// delta encode docids and frequncies from begin through end
			/// call before write
			void deltaEncode();
			/// delta decode docids and positions from begin through end
			/// call after read
			void deltaDecode();

	

		private:	
			int winCount;					  //the num of the windows

			int* winSizes;        //size for each windows
			
			//lemur::api::LOC_T* _begin;         // pointer to the beginning of this list
			//lemur::api::LOC_T* _lastid;        // pointer to the most recent DocID added
			//lemur::api::LOC_T* _freq;          // pointer to the frequency of the last DocID
			//lemur::api::LOC_T * _end;            // pointer to the next free memory
			//mutable lemur::api::LOC_T* _iter;    // pointer tells us where we are in iteration
			//int  size;                // how big are we, increment in powers of 2, start at 16K
			//int LOC_Tsize;
			//lemur::api::COUNT_T  df;    // the document frequency for current link

			link::api::LINKID_T ulid;   // a unique ID for our link 

			mutable link::api::InvLinkDocInfo entry;
			

		};

	}

}



#endif
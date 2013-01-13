/**======================================
/*
 * NAME DATE - COMMENTS
 * wy 02/2011 - created 
 *========================================================================*/

#ifndef INV_LINKINDEX_HPP
#define INV_LINKINDEX_HPP

#include "KeyfileIncIndex.hpp"
#include "LinkTypes.hpp"
#include "InvLinkDocList.hpp"
#include "LinkInfo.hpp"
#include "LinkCache.hpp"
#include "CandidateLinkFile.hpp"
#include "InvLinkInfoList.hpp"

#include <vector>
#include <fstream>


namespace link{

	namespace api{

#define LINK_KEYFILE_MAX_SEGMENTS (2)

// keyref.h -- 512
#define MAX_LINK_LENGTH 512

// the Version of the Link Index
#define LINKINDEX_VERSION "1.00"

// suffixes for filenames about link info
#define INVLIKINDEX ".pl"
#define INVLINKLOOKUP  ".pllookup"
#define DLINDEX  ".dl"
#define DLLOOKUP  ".dllookup"
#define LINKIDMAP  ".lid"
#define LINKIDPAIRMAP ".tidpair"

// for linknames array
#define INVLINK_INDEX    0
#define INVLINK_LOOKUP   1
#define DOCLINK_INDEX    2
#define DOCLINK_LOOKUP   3
#define LINK_IDS     4
#define LINK_IDSTRS      5

// size of linknames arrray
#define LINKNAMES_SIZE 6

// name for parameters about link info
#define LINKVERSION_PAR "LINK_VERSION"
#define NUMLINKS_PAR "NUM_LINKS"
#define NUMULINKS_PAR "NUM_UNIQUE_LINKS"
#define INVLINKINDEX_PAR  "INV_LINK_INDEX"
#define INVLINKLOOKUP_PAR  "INV_LINK_LOOKUP"
#define DLINDEX_PAR  "DL_INDEX"
#define DLLOOKUP_PAR  "DL_LOOKUP"
#define LINKIDMAP_PAR  "LINKIDS"
#define LINKIDPAIRMAP_PAR "LINKIDPAIRS"
//窗口信息
#define NUMWINDOWS_PAR "NUM_WINDOWS"//<=MAXNUM_WIN
#define SIZEWINDOWS_PAR "SIZES_WINDOWS"

// for linkcounts array
#define UNIQUE_LINKS 0
#define TOTAL_LINKS  1

		class InvLinkIndex : public lemur::index::KeyfileIncIndex
		{
		public:
		  /// principle record
		  class linkrecord {
		  public:
			/// file offset
			lemur::file::File::offset_type offset;
			/// number of link in its doc
			int linkCount;
			/// mgrid for terminfolist, df for docinfolist
			int num;     
		  };
		    /// individual link data
		  struct LinkData {//按最大数目字节对齐，不能出现填充的现象，keyfile文件(btree)读取数据时会出现bug.		
			/// total number of documents this link occurs in
			lemur::api::COUNT_T documentCount;

			/// total number of times this link occurs in the corpus
			link::api::LINKCOUNT_T totallinkCount;

			/// total number of times this link occurs in the size of X windows in the corpus 
			link::api::LINKCOUNT_T totallinkCountX[MAXNUM_WIN];

			/// segments containing the data associated with the the term
			SegmentOffset segments[ KEYFILE_MAX_SEGMENTS ];
		  };


		public:
		    /// Instantiate with index name without extension. Optionally pass in
			/// cachesize and starting document id number.
			InvLinkIndex(const string &prefix, std::vector<int> &winSizes, const string &clFile,
					bool usedLink = false, bool usedCandLink = false,
					int cachesize=256000000, lemur::api::DOCID_T startdocid=1);

			/// read linkindex to use.
			InvLinkIndex();

			///close the files about link, free memory
			~InvLinkIndex();
			
			///add link info to the index
			bool addLink(const link::api::Link &l);

			/// adding a term to the current document. Override~!
			bool addTerm(const lemur::api::Term &t);

			/// signify the end of current document. override~!
			virtual void endDoc(const lemur::parse::DocumentProps* dp);

			/// signify the end of this collection. override~!
			virtual void endCollection(const lemur::parse::CollectionProps* cp);

		protected:
			///initialization of the linkindex
			//void InitLinkIndex(const string &prefix, int cachesize=256000000, lemur::api::DOCID_T startdocid=1);
			void InitLinkIndex();

			 /// update data for an already seen link
			void addKnownLink( link::api::LINKID_T linkID, link::api::DIST_T dist );

			/// initialize data for a previously unseen link.
			link::api::LINKID_T addUnknownLink( const LinkInfo* linkinfo );

			/// update data for a link that is not cached in the link cache.
			link::api::LINKID_T addUncachedLink( const LinkInfo* linkinfo );

			/// store a link record
			void addLinkLookup( link::api::LINKID_T linkKey, const char* linkSpelling );

			/// add a distance to a LikedTermList
			void _updateLinkedTermlist( InvLinkDocList* curlist, link::api::DIST_T dist );
			

			/// Approximate how many updates to collect before flushing the cache.
			void _resetLinkEstimatePoint();
			
			/// total memory used by cache
			virtual int _cacheLinkSize();

			/// create the database files about link info
			void createLinkDBs();	

			/// add a position to a TermList. OverLoad~!
			void _updateTermlist( lemur::api::TERMID_T tid, lemur::api::LOC_T position );

			/// handle end of document token.
			virtual void doendLinkDoc(const lemur::parse::DocumentProps* dp, int mgrid);

			 /// write InvFPDocList or InvLinkDocList out the cache.override~!
			void writeCache( bool lastRun = false );

			/// write InvLinkDocList out the cache
			virtual void writeLinkCache(bool finalRun = false);
			/// final run write InvLinkDocList out of cache
			virtual void writeLinkCacheSegment();

			/// out-of-tree cache management combine segments into single segment
			virtual void mergeLinkCacheSegments();

			/// final run write out of cache
			void lastWriteLinkCache();
			
			/// write out the table of contents file about linkinfo.
			void writeLinkTOC(const lemur::parse::CollectionProps* cp);

//===========================read Link Index=======================================
		public:
			/// @name Open link index to load
			//@{
			/// Open previously created Index with given prefix
			bool open(const string &indexName);

		protected:
			/// readin all link stoc
			void fullLinkToc();

			/// open the database files about link
			void openLinkDBs();

			/// open the segment files
			void openLinkSegments();
			//@}

		public:   
			/// @name Spelling and link index conversion
			//@{
			/// Convert a linkTerm spelling<termID-termID> to a linkTermID
			link::api::LINKID_T linkTerm(const link::api::LINK_STR &pairWords) const;

			/// Convert a linkTermID to a linkTerm spelling<termID-termID>
			link::api::LINK_STR linkTerm(const link::api::LINKID_T linkID) const;
			//@}
	
			/// @name Summary counts about linkinfo
			//@{
			/// Total count of unique links in collection
			link::api::LINKCOUNT_T linkCountUnique() const {return linkcounts[UNIQUE_LINKS];};

			/// Total counts of a link in collection
			//link::api::LINKCOUNT_T linkCount(link::api::LINKID_T linkID) const;

			/// Total counts of all links in collection
			link::api::LINKCOUNT_T linkCount() const {return linkcounts[TOTAL_LINKS];};

			/// Total counts of all windows
			int winCount()const {return windowCount;}

			/// size of windows.
			std::vector<int> winSizes()const {return winSize;}

			lemur::api::COUNT_T docCount(link::api::LINKID_T linkID) const; // int64/int32 bug?
			//@}

			/// @name linkIndex entry access
			//@{
			/// doc entries in a link index, @see DocList @see InvFPDocList 
			lemur::api::DocInfoList* InvlinkDocList(link::api::LINKID_T linkID) const;

		protected:
			/// retrieve and construct the DocInfoList for a link.
			link::api::InvLinkDocList* internalLinkDocInfoList(link::api::LINKID_T linkID) const;

			/// linkwords entries in a document index (sequence of words), @see TermList
			link::api::InvLinkInfoList* linkInfoListSeq(lemur::api::DOCID_T docID) const;
		public:
			/// linkwords entries in a document index (bag of words), @see TermList
			link::api::InvLinkInfoList* linkInfoList(lemur::api::DOCID_T docID) const;


			//@}

		protected:

			///if use link index this value is true.else false
			bool isLinkIndex;

			///if use candlink this value is true;else false
			bool isCandLinkIndex;
			link::file::CandidateLinkFile* candLinkFile;//需要读取的*.cl文件类，候选link文件格式:<link,linkDF>

			///size of the window.
			std::vector<int> winSize;
			///num of the window.
			int windowCount;

			///linkcounts array to count number of the link info
			link::api::LINKID_T* linkcounts;
			/// array to hold all the names for files aboute link we need for this db
			std::vector<std::string> linknames;

			/// array of pointers to linkdoclists
			vector<InvLinkDocList*> invlinklists; 

			// All database handles are marked mutable since they sometimes
			// must be used to fetch values during const methods

			/// filestream for writing the list of located terms
			/// mutable for index access mode of Index API (not PushIndex)
			mutable lemur::file::File writelinklist; //*.dl文件，格式为<docid,linkCount,length,linklist>

			/// link statistics in the document(linkCount, etc.)
			mutable lemur::file::File dllookup; //*.dllookup 
			/// read buffer for dllookup
			lemur::file::ReadBuffer* dllookupReadBuffer; 

			/// linkID -> LinkData (link statistics and inverted linkdoc list segment offsets)
			mutable lemur::file::Keyfile pllookup;//*.pllookup

			// int <-> string mappings for links
			/// linkName<termID,termID> -> linkID 
			mutable lemur::file::Keyfile lIDs;//*.lid
			/// linkID -> linkName<termID,termID>
			mutable lemur::file::Keyfile lSTRs;//*.tidpair

			/// out-of-tree segments for data
			std::vector<lemur::file::File*> _linksegments;
			
			std::vector<link::api::LinkInfo *> linkinfolist;

			std::vector<link::api::LinkedTerm> linkedtermlist;

			/// cache of link entries
			link::utility::LinkCache _linkCache;

		    /// how long all the linklists are
			link::api::LINKCOUNT_T link_listlengths;

			/// highest link id flushed to disk.
			link::api::LINKID_T _largestFlushedLinkID;	

			/// are we in a bad document about link state?
			bool ignoreLink; 

			/// buffers for linkterm() lookup functions
			mutable char linkKey[MAX_TERM_LENGTH];

		private:

			///open files about link.
			bool tryOpenLinkfile();

			///create link info for a document
			bool createLinkInfo(const lemur::parse::DocumentProps* dp);
		};
	
	}
}

#endif // INV_LINKINDEX_HPP
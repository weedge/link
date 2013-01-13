/**======================================
/*
 * NAME DATE - COMMENTS
 * wy 02/2011 - created 
 *========================================================================*/

#ifndef KEYFILE_LINKINDEX_HPP
#define KEYFILE_LINKINDEX_HPP

#include "common_headers.hpp"
#include "LinkTypes.hpp"
#include "InvLinkDocList.hpp"
#include "LinkInfo.hpp"
#include "LinkCache.hpp"
#include "CandidateLinkFile.hpp"
#include "InvLinkInfoList.hpp"
#include "Index.hpp"
#include "Keyfile.hpp"
#include "KeyfileDocMgr.hpp"
#include "ReadBuffer.hpp"
#include "WriteBuffer.hpp"
#include "Param.hpp"
#include "DocumentProps.hpp"

#include <vector>
#include <fstream>
#include <cstring>
#include <queue>

namespace link{

	namespace api{

#define LINK_KEYFILE_MAX_SEGMENTS (2)

// keyref.h -- 512
#define MAX_LINK_LENGTH 512

// the Version of the Link Index
#define LINKINDEX_VERSION "1.00"

// suffixes for filenames about link info
#define INVLIKINDEX ".invlink"
#define INVLINKLOOKUP  ".invlinklookup"
#define LINKIDMAP  ".lid"
#define LINKIDPAIRMAP ".tidpair"
#define DLINDEX  ".dl"
#define DLLOOKUP  ".dllookup"

// for linknames array
#define INVLINK_INDEX    0
#define INVLINK_LOOKUP   1
#define LINK_IDS     2
#define LINK_IDSTRS      3
#define DOCLINK_INDEX    4
#define DOCLINK_LOOKUP   5

// size of linknames arrray
#define LINKNAMES_SIZE 6

// name for parameters about link info
#define LINKVERSION_PAR "LINK_VERSION"
#define NUMLINKS_PAR "NUM_LINKS"
#define NUMULINKS_PAR "NUM_UNIQUE_LINKS"
#define INVLINKINDEX_PAR  "INV_LINK_INDEX"
#define INVLINKLOOKUP_PAR  "INV_LINK_LOOKUP"
#define LINKIDMAP_PAR  "LINKIDS"
#define LINKIDPAIRMAP_PAR "LINKIDPAIRS"
#define DLINDEX_PAR  "DL_INDEX"
#define DLLOOKUP_PAR  "DL_LOOKUP"

//窗口信息
#define NUMWINDOWS_PAR "NUM_WINDOWS"//<=MAXNUM_WIN
#define SIZEWINDOWS_PAR "SIZES_WINDOWS"

// for linkcounts array
#define UNIQUE_LINKS 0
#define TOTAL_LINKS  1

		class KeyfileLinkIndex 
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

		  /// offset within an individual file segment
		  struct SegmentOffset 
		  {
			  /// segment number
			  unsigned int segment;
			  /// length of data
			  unsigned int length;
			  /// file offset
			  lemur::file::File::offset_type offset;
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
			SegmentOffset segments[ LINK_KEYFILE_MAX_SEGMENTS ];
		  };


		public:
		    /// Instantiate with index name without extension. Optionally pass in
			/// cachesize and starting document id number.
			KeyfileLinkIndex(const string &prefix, std::vector<int> &winSizes, const string &clFile,
					 bool usedCandLink = false, int cachesize=256000000, lemur::api::DOCID_T startdocid=1);

			/// read linkindex to use.
			KeyfileLinkIndex();

			///close the files about link, free memory
			~KeyfileLinkIndex();
			
			///add link info to the index
			bool addLink(const link::api::Link &l);

			//====add by wy in 2012-02-11.
			/// add linkinfo to the linkDB
			void addLinkInfotoLinkDB(lemur::api::Index *index);
			void addCandLinkInfotoLinkDB(lemur::api::Index *index);
			link::api::LINKCOUNT_T getCandLinkCount();

			/// signify the end of current document. override~!
			virtual void endDoc(const link::api::LinkInfo *linkinfo);

			/// signify the end of this collection. override~!
			virtual void endCollection(const lemur::parse::CollectionProps* cp);


			void setMesgStream(ostream * lemStream);

			void setName(const string &prefix);

		protected:
			/// estimate point to flush the date in the cache to the disk.
			void estimatePointToWriteCahce();
			/// cache size limits based on cachesize parameter to constructor
			void _computeMemoryBounds( int memorySize );
			/// write out document manager ids
			void writeDocMgrIDs();



			///initialization of the linkindex
			//void InitLinkIndex(const string &prefix, int cachesize=256000000, lemur::api::DOCID_T startdocid=1);
			void InitLinkIndex();

			 /// update data for an already seen link
			void addKnownLink( link::api::LINKID_T linkID, const link::api::LinkInfo* dist );

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

			/// handle end of document token.
			virtual void doendLinkDoc(const link::api::LinkInfo *linkinfo, int mgrid);

			/// write InvLinkDocList out the cache
			virtual void writeLinkCache(bool finalRun = false);
			/// final run write InvLinkDocList out of cache
			virtual void writeLinkCacheSegment();

			/// out-of-tree cache management combine segments into single segment
			virtual void mergeLinkCacheSegments();

			/// final run write out of cache
			void lastWriteLinkCache();
			
			/// write out the table of contents file about linkinfo.
			void writeLinkTOC();

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

			/// DF(link)
			lemur::api::COUNT_T docCount(link::api::LINKID_T linkID) const; // int64/int32 bug?
			
			/// total linkcount
			link::api::LINKCOUNT_T linkCount(link::api::LINKID_T linkID) const;
			/// total linkcount in windows Xs
			link::api::LINKCOUNT_T* linkCountX(link::api::LINKID_T linkID) const;
			/// total linkcount in a doc & windows Xs
			lemur::api::LOC_T* linkCountX(link::api::LINKID_T linkID, lemur::api::DOCID_T docID) const;
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
			
			/// retrieve a document record.
			linkrecord fetchDocumentRecord( lemur::api::DOCID_T key ) const;
			
			//@}

		protected:

			///if use candlink this value is true;else false
			bool isCandLinkIndex;
			link::file::CandidateLinkFile* candLinkFile;//需要读取的*.cl文件类，候选link文件格式:<link,linkDF>
			std::ifstream candFile ;

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
			mutable lemur::file::Keyfile invlinklookup;//*.invlinklookup

			// int <-> string mappings for links
			/// linkName<termID,termID> -> linkID 
			mutable lemur::file::Keyfile lIDs;//*.lid
			/// linkID -> linkName<termID,termID>
			mutable lemur::file::Keyfile lSTRs;//*.tidpair

			/// out-of-tree segments for data
			std::vector<lemur::file::File* > _linksegments;
			
			std::vector<link::api::LinkInfo* > linkinfolist;

			std::vector<link::api::LinkedTerm> linkedtermlist;

			/// cache of link entries
			link::utility::LinkCache _linkCache;

		    /// how long all the linklists are
			link::api::LINKCOUNT_T link_listlengths;

			/// highest link id flushed to disk.
			link::api::LINKID_T _largestFlushedLinkID;	

			/// invertedlinklists point where we should next check on the cache size
			int _estimatePoint; 
			/// are we in a bad document about link state?
			bool ignoreDoc;  
			/// buffers for linkterm() lookup functions
			mutable char linkKey[MAX_LINK_LENGTH];

			/// Lemur code messages stream            
			ostream* msgstream;
			/// list of document managers
			vector<std::string> docmgrs;

			/// the prefix name
			std::string name;
			/// memory for use by inverted list buffers
			int _listsSize;
			/// upper bound for memory use
			int _memorySize;
			/// are we read only
			bool _readOnly;

			/// the current docmanager to use
			int curdocmgr; 
		private:

			///open files about link.
			bool tryOpenLinkfile();

			///create link info for a document
			bool createLinkInfo(const lemur::parse::DocumentProps* dp);
		};
	
	}
}

#endif // KEYFILE_LINKINDEX_HPP
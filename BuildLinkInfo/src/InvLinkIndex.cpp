/*===========================================
 NAME	DATE			CONMENTS
 wy		02/2011			created
 wy     12/2011         try to find a good job , 
                        so lost many time~~~~.
						now add  interface to load linkindex~! 
 =================================================
 *
 */

#include "InvLinkIndex.hpp"
#include "InvFPTypes.hpp"
#include "LinkInfo.hpp"
#include "Link.hpp"
#include "InvLinkInfoList.hpp"
#include "LinkDocListSegmentReader.hpp"
#include "CandidateLinkFile.hpp"
#include "minmax.hpp"
#include "Indri/Path.hpp"
#include "indri/IndriTimer.hpp"
#include <vector>
#include <sstream>
#include <queue>
#include <assert.h>

// Keyfiles get 1mb each, and there are 6 of them
// dtlookup gets 10MB read buffer
// 16MB goes to the TermCache
// 1MB for the output buffer at flush time
// 1MB for general data structures
// 4MB for the invlists vector (4 bytes * 1M terms)
// 10MB for heap overhead, general breathing room
// Total base RAM usage: 50MB

#define KEYFILE_BASE_MEMORY_USAGE       (50*1024*1024)
#define KEYFILE_WRITEBUFFER_SIZE           (1024*1024)
#define KEYFILE_DOCLISTREADER_SIZE         (1024*1024)

#define KEYFILE_MINIMUM_LISTS_SIZE          (512*1024)
#define KEYFILE_EPSILON_FLUSH_POSTINGS      (512*1024)

link::api::InvLinkIndex::InvLinkIndex(const std::string &prefix, std::vector<int> &winSizes,
									  const string &clFile, bool usedLink, bool usedCandLink, int cachesize, 
									  lemur::api::DOCID_T startdocid):lemur::index::KeyfileIncIndex(prefix,cachesize,startdocid)
{
	assert( winSizes.size()>0 );

	this->isLinkIndex = usedLink;
	this->isCandLinkIndex = usedCandLink;

	if(this->isLinkIndex)
	{
		this->ignoreLink = false;
		this->winSize.resize(winSizes.size());
		this->winSize = winSizes;

		this->linkcounts = new link::api::LINKID_T[2];
		InitLinkIndex();

		if(this->isCandLinkIndex)
		{
			/*候选link文件格式有2种：true为：termid-termid term-term linkDF;false为：termid-termid linkDF*/
			this->candLinkFile = new link::file::CandidateLinkFile(true);
			std::ifstream candFile;
			candFile.open(clFile.c_str(),std::ios::in|std::ios::binary);
			if(candFile.is_open()){
				this->candLinkFile->openForReadtoVec(candFile);
				candFile.close();
			}
		}
	}
}

/** 
 * 创建*.dl,*.dllookup,*.pllookup,*.lid,*.tidpair文件 
 * 向keyfile类型文件*.dllookup,*.pllookup,*.lid,*.tidpair
 * 中写入未经压缩的文件头信息，共 4096 Byte
 */
void link::api::InvLinkIndex::InitLinkIndex()
{
	this->_resetLinkEstimatePoint();

	this->link_listlengths = 0;
	//std::string filename;
	//filename = this->name + ".link_key";
	if(!tryOpenLinkfile())
	{
		*msgstream<<"Creating new linkindex "<<std::endl;
		linknames.resize(LINKNAMES_SIZE);

		linkcounts[TOTAL_LINKS] = linkcounts[UNIQUE_LINKS] = 0;
		std::stringstream docfname;
		docfname<<name<<DLINDEX;
		this->writelinklist.open(docfname.str(),ios::binary | ios::out | ios::ate);//open *.dl file

		linknames[INVLINK_LOOKUP] = name + INVLINKLOOKUP;//*.pllookup
		linknames[INVLINK_INDEX] = name + INVLIKINDEX;//*.pl
		linknames[DOCLINK_INDEX] = name + DLINDEX;//*.dl
		linknames[DOCLINK_LOOKUP] = name + DLLOOKUP;//*.dllookup

		// create link ids trees
		linknames[LINK_IDS] = name + LINKIDMAP;//*.lid
		linknames[LINK_IDSTRS] = name + LINKIDPAIRMAP;//*.tidpair

		this->createLinkDBs();
		this->_largestFlushedLinkID = 0;
	}
}

/**
* 打开索引读时用到该构造函数，调用父类构造函数初始化信息。
*/
link::api::InvLinkIndex::InvLinkIndex()
{
	this->isLinkIndex = true;
	this->isCandLinkIndex = false;
	this->linkcounts = new link::api::LINKID_T[2];
}

/**
* 析构函数，释放类所用到的内存空间。
*/
link::api::InvLinkIndex::~InvLinkIndex()
{
	if(this->isLinkIndex){
		for(unsigned int i = 0; i< _linksegments.size(); i++ ) 
		{
			_linksegments[i]->close();
			delete _linksegments[i];
		}

		writelinklist.close();
		dllookup.close();
		pllookup.close();
		lIDs.close();
		lSTRs.close();

		delete dllookupReadBuffer;
		delete[](linkcounts);

		if(this->isCandLinkIndex){
			//this->candLinkFile.close();
			delete this->candLinkFile;
		}
	}

}


int link::api::InvLinkIndex::_cacheLinkSize()
{
	int totalsize = 0;
	for(unsigned int i=0; i< this->invlinklists.size(); i++)
	{
		link::api::InvLinkDocList *list = this->invlinklists[i];
		if(list){	
			totalsize += sizeof(InvLinkDocList) + list->memorySize();
		}
	}
	
	return totalsize;	
}

void link::api::InvLinkIndex::_resetLinkEstimatePoint()
{
	// how many postings should we queue up before
	// trying to estimate a flush time?
	// pick the worst case--one unique term per document
	// that would give us one InvLinkDocList plus a few bytes
	// for the list buffer:
	//std::cerr<<"sizeof(InvLinkDocList):"<<sizeof(InvLinkDocList)<<std::endl;
	_estimatePoint = _listsSize / (sizeof(InvLinkDocList) + 16);//120+16

}

bool link::api::InvLinkIndex::tryOpenLinkfile()
{
	std::string filename;
	filename = this->name + ".link_key";
	return open(filename);
}
/**
* 继承父类Index的open函数,对外使用的接口，用于打开加载连接索引。
* 打开在*.key文件中写明的*.dl,*.dllookup,*.pllookup,*.lid,*.tidpair文件。
*/
bool link::api::InvLinkIndex::open(const std::string &indexName)
{
	// stupid (maybe just use cout to put the stream in cache to a file). 
	lemur::utility::String streamSelect = lemur::api::ParamGetString("stream", "cerr");
	if (streamSelect == "cout") 
	{
		setMesgStream(&cout);
	} 
	else 
	{
		setMesgStream(&cerr);
	}

	*msgstream << "Trying to open toc: " << indexName << endl;
	if (! indri::file::Path::exists(indexName) || ! lemur::api::ParamPushFile(indexName)) 
	{
		*msgstream << "Couldn't open toc file for reading" << endl;
		return false;
	}
	
	this->ignoreLink = true;

	fullLinkToc();

	invlinklists.resize(linkcounts[UNIQUE_LINKS], 0 );//Bug:(link数目多时;vector 溢出的情况)。invlinklist.max_size()测试。
	_largestFlushedLinkID = linkcounts[UNIQUE_LINKS];

	std::string prefix = indexName.substr( 0, indexName.rfind('.'));
	setName(prefix);

	openLinkDBs();//打开 *.lid, *.lidstr, *.pllookup, *.dllookup文件, 以及打开*.did, *.didstr,
	openLinkSegments();//打开*.pl文件

	std::stringstream docfname;
	docfname << name << DLINDEX;
	if (_readOnly){
		writelinklist.open( docfname.str(), ios::in | ios::binary );//打开*.dl文件
	}else{ 
		writelinklist.open( docfname.str(), 
					 ios::in | ios::out | ios::binary | ios::ate );
		writelinklist.seekg( 0, std::ios::beg );
	}
	lemur::api::ParamPopFile();

	*msgstream << "Load link index complete." << endl;
	return true;
}

void link::api::InvLinkIndex::fullLinkToc()
{
	linkcounts[TOTAL_LINKS] = lemur::api::ParamGetInt(NUMLINKS_PAR, 0);
	linkcounts[UNIQUE_LINKS] = lemur::api::ParamGetInt(NUMULINKS_PAR, 0);

	windowCount = lemur::api::ParamGetInt(NUMWINDOWS_PAR, 1);
	std::string temp = lemur::api::ParamGetString(SIZEWINDOWS_PAR, "40");
	std::istringstream stream(temp);
	winSize.resize(windowCount);
	for(int i = 0; i != windowCount; ++i)
	{
		stream >> winSize[i];
		if(winSize[i] <= 0) {
			std::cerr<<"window size is invalid~!"<<std::endl;
			exit(-1);
		}
	}//for
	std::sort(winSize.begin(),winSize.end());

	linknames.resize( LINKNAMES_SIZE, std::string() );
	linknames[INVLINK_INDEX] = lemur::api::ParamGetString(INVLINKINDEX_PAR);
	linknames[INVLINK_LOOKUP] = lemur::api::ParamGetString(INVLINKLOOKUP_PAR);
	linknames[DOCLINK_INDEX] = lemur::api::ParamGetString(DLINDEX_PAR);
	linknames[DOCLINK_LOOKUP] = lemur::api::ParamGetString(DLLOOKUP_PAR);
	linknames[LINK_IDS] = lemur::api::ParamGetString(LINKIDMAP_PAR);
	linknames[LINK_IDSTRS] = lemur::api::ParamGetString(LINKIDPAIRMAP_PAR);


}

/**
* 打开 *.lid, *.lidstr, *.pllookup, *.dllookup文件
*/
void link::api::InvLinkIndex::openLinkDBs()
{
	if (_readOnly) 
	{
		dllookup.open( linknames[DOCLINK_LOOKUP], std::ios::binary | std::ios::in );
	} 
	else 
	{
		dllookup.open( linknames[DOCLINK_LOOKUP], 
				   std::ios::binary | std::ios::in | std::ios::out );
		dllookup.seekp( 0, std::ios::end );
	}
	dllookup.seekg( 0, std::ios::beg );
	dllookupReadBuffer = new lemur::file::ReadBuffer( dllookup, 10*1024*1024 );
	int cacheSize = 1024 * 1024;
	pllookup.open( linknames[INVLINK_LOOKUP], cacheSize, _readOnly );
	lIDs.open( linknames[LINK_IDS], cacheSize, _readOnly  );
	lSTRs.open( linknames[LINK_IDSTRS], cacheSize, _readOnly  );
}

/**
* 打开 *.pl文件
*/
void link::api::InvLinkIndex::openLinkSegments()
{
	// open existing inverted list segments
	for( int i=0; i<LINK_KEYFILE_MAX_SEGMENTS; i++ ) 
	{
		lemur::file::File* in = new lemur::file::File();
		std::stringstream name;
		name << linknames[INVLINK_INDEX] << i;

		in->open( name.str().c_str(), std::fstream::in | std::fstream::binary );//以读和二进制的方式打开*.pl文件

		if( in->rdstate() & std::fstream::failbit ) 
		{
			delete in;
			break;
		} 
		else 
		{
			_linksegments.push_back( in );
		}
	}//for
}

/**
* 创建*.dllookup,*.pllookup,*.lid,*.tidpair文件。
*/
void link::api::InvLinkIndex::createLinkDBs()
{
	this->dllookup.open(linknames[DOCLINK_LOOKUP],std::ios::binary | std::ios::out | std::ios::in);
	this->dllookup.seekg(0, std::ios::beg);
	this->dllookup.seekp(0, std::ios::end);
	this->dllookupReadBuffer = new lemur::file::ReadBuffer(dllookup, 10*1024*1024);

	this->pllookup.create(linknames[INVLINK_LOOKUP] );
	this->lIDs.create(linknames[LINK_IDS] );
	this->lSTRs.create(linknames[LINK_IDSTRS] );
	
}

/**
* 重写（覆盖）addTerm,是否使用link的情况下，添加term.
*/
bool link::api::InvLinkIndex::addTerm(const lemur::api::Term &t)
{
	if (ignoreDoc) return false; // skipping this document.
	const lemur::index::InvFPTerm* term = dynamic_cast< const lemur::index::InvFPTerm* >(&t);
	assert( term->strLength() > 0 );
	assert( term->spelling() != NULL );
	int len = term->strLength();
	if (len > (MAX_TERM_LENGTH - 1)) {
		//term is too big to be a key.
		cerr << "lemur::index::KeyfileIncIndex::addTerm: ignoring " << term->spelling()
			 << " which is too long ("
			 << len << " > " << (MAX_TERM_LENGTH - 1) << ")" << endl;
		return false;
	}

	lemur::api::TERMID_T id = _cache.find( term->spelling() );

	if( id > 0 ) {
		if(this->isLinkIndex)
		{
			_updateTermlist( id, term->position() );
		}
		else
		{
			addKnownTerm( id, term->position() );
		}
	} 
	else 
	{
		if(this->isLinkIndex){
			int actual = 0;
			if( tIDs.get( term->spelling(), &id, actual, sizeof id ) ) 
			{
				_updateTermlist( id, term->position() );
			}
			else 
			{
				counts[UNIQUE_TERMS]++;
				id = counts[UNIQUE_TERMS];
				 // store the term in the keyfiles (tIDs, tSTRs)->tID
				addTermLookup( id, term->spelling() );
				_updateTermlist( id, term->position() );
			}
		}
		else
		{
			id = addUncachedTerm( term );
		}
		_cache.add( term->spelling(), id );
	}
	return true;
}

/**
* 重载_updateTermList，将位置termid和position写入locatedtermList中。
*/
void link::api::InvLinkIndex::_updateTermlist(lemur::api::TERMID_T tid, lemur::api::LOC_T position)
{
	lemur::index::LocatedTerm lt;
	lt.loc = position;
	lt.term = tid;
	termlist.push_back(lt);
	listlengths++;	
}

/**
* 重写writeCache，考虑到是否生成link的情况（便于以后扩展）。
*/
void link::api::InvLinkIndex::writeCache(bool lastRun)
{
	if(!this->isLinkIndex)
	{
		writeCacheSegment();
		if( _segments.size() >= LINK_KEYFILE_MAX_SEGMENTS || 
			(lastRun && _segments.size() > 1) )
		{
			mergeCacheSegments();
		}
	}
}


/**
* 一篇文档结束后，将生成的link信息写入相关文件中。
*/
void link::api::InvLinkIndex::endDoc(const lemur::parse::DocumentProps *dp)
{
	//std::cerr<<"docID:"<<dp->stringID()<<std::endl;
	if(this->isLinkIndex)
	{
		if(this->createLinkInfo(dp))
		{
			if(!this->ignoreDoc)
			{//未知文档
				this->doendDoc(dp,this->curdocmgr);	
			}
			this->doendLinkDoc(dp,this->curdocmgr);
		}
	}else
	{
		if(!this->ignoreDoc)
		{
			this->doendDoc(dp,this->curdocmgr);
		}
	}
	this->ignoreDoc = false;
}

/**
* 创建Link<termID,termID>相关信息，并统计相关信息:Fre(X)
*/
bool link::api::InvLinkIndex::createLinkInfo(const lemur::parse::DocumentProps *dp)
{
	std::cerr<<"docID:"<<dp->stringID()<<std::endl;
	//link::api::Link link_item;
	if(dp->length() <= 0) return false;

	//std::map< link::api::LINK_STR, std::vector<link::api::DIST_T> > link_dists;

	vector<lemur::index::LocatedTerm>::iterator left_iter = this->termlist.begin();
	for(; left_iter != this->termlist.end(); left_iter++)
	{
		//link_item.setLeftID( (*left_iter).term );

		vector<lemur::index::LocatedTerm>::iterator right_iter = left_iter+1;
		for(; right_iter != this->termlist.end(); right_iter++)
		{
			if((*left_iter).term != (*right_iter).term)
			{
				//linkcounts[TOTAL_LINKS]++;

				//link_item.setRightID( (*right_iter).term );	
				//link_item.linkProcess();
				//link_item.setDistace(abs( (*right_iter).loc - (*left_iter).loc ));
				
				link::api::LinkInfo linkinfo((*left_iter).term, (*right_iter).term);
				linkinfo.setDistace(abs( (*right_iter).loc - (*left_iter).loc ));

				if(this->isCandLinkIndex)
				{/*如果有候选link,添加候选link信息到相关文件中,而无需添加全部link信息到相关文件中。*/

					/*候选link文件格式有2种：true为：termid-termid term-term; false为：termid-termid */
					//link::file::CandidateLinkFile clFile(true);
					//clFile.openForRead(this->candLinkFile);
					//lemur::api::COUNT_T linkDF;
					if(candLinkFile->findLink(linkinfo.toString()))
					{
						//std::cerr<<"the link:"<<linkinfo.toString()<<" the linkDF:"<<linkDF<<std::endl;
						this->addLink(linkinfo);
					}
				}
				else
				{
					this->addLink(linkinfo);//添加link信息到相关文件中。
				}
	 
				//link_dists[linkinfo.toString()].push_back(linkedterm.dist);  
			}else{
				continue;
			}
			
		}	
		
	}
	return true;
}

/**
* 将创建好的link相关信息加入InvLinkDocList中，
* 并且添加映射linkID<->(termID,termID)到keyfile类型文件中：*.lid,*.tidpair文件 
*/
bool link::api::InvLinkIndex::addLink(const link::api::Link &l)
{
  if (ignoreDoc) return false; // skipping this document.
  const LinkInfo* linkinfo = dynamic_cast< const LinkInfo* >(&l);
  
  int len = linkinfo->toString().length();
  if (len > (MAX_LINK_LENGTH - 1)) //link is too big to be a key.
  {
	  cerr << "lemur::index::KeyfileIncIndex::addLink: ignoring " << linkinfo->toString()
         << " which is too long ("
         << len << " > " << (MAX_LINK_LENGTH - 1) << ")" << endl;
    return false;
  }


  link::api::LINKID_T lid = _linkCache.find(linkinfo->toString().c_str());//查找link是否已经出现在cache中

  if( lid > 0 ) //link已经被缓存，直接加入到索引中
  {
	  addKnownLink( lid, linkinfo->getDistance() );
  } 
  else //link未被缓存
  {
    lid = addUncachedLink( linkinfo );
	_linkCache.add( linkinfo->toString().c_str(), lid );//向cache中添加新link
  }

  return true;

}


link::api::LINKID_T link::api::InvLinkIndex::addUncachedLink(const link::api::LinkInfo* linkinfo)
{
	link::api::LINKID_T lid;
	int actual = 0;
    
	if( lIDs.get( linkinfo->toString().c_str(), &lid, actual, sizeof lid ) ) //该link已经出现过
	{
		addKnownLink( lid, linkinfo->getDistance() );
	} 
	else //添加新term
	{
		lid = addUnknownLink( linkinfo );
	}

	return lid;

}

link::api::LINKID_T link::api::InvLinkIndex::addUnknownLink(const link::api::LinkInfo* linkinfo)
{

	// update unique link counter
	linkcounts[UNIQUE_LINKS]++;
	link::api::LINKID_T linkID = linkcounts[UNIQUE_LINKS];//设置link的ID

	InvLinkDocList* curlist = new InvLinkDocList(linkID, this->winSize);//link对应的反转文档列表
	
	/*LinkDocumentList* curlist = new LinkDocumentList(termID, term->strLength());*/
	invlinklists.push_back( curlist );  // link is added at index linkID-1

	// store the link in the keyfiles (lIDs, lSTRs)
	addLinkLookup( linkID, linkinfo->toString().c_str() );

	addKnownLink( linkID, linkinfo->getDistance() );
	return linkID;
}

void link::api::InvLinkIndex::addLinkLookup(link::api::LINKID_T linkKey, const char *linkSpelling)
{
	lSTRs.put(linkKey, (void*) linkSpelling, strlen(linkSpelling));
	lIDs.put(linkSpelling, &linkKey, sizeof linkKey );
}


void link::api::InvLinkIndex::addKnownLink(link::api::LINKID_T linkID, link::api::DIST_T dist)
{
	InvLinkDocList* curlist;

	curlist = invlinklists[linkID - 1];
	if (curlist == NULL) 
	{
		// append it to old list when flushed
		// tds - removing term length; shouldn't really be there anyway
		curlist = new InvLinkDocList(linkID, this->winSize);
		invlinklists[linkID-1] = curlist;
	}

	curlist->addFrequence(counts[DOCS], dist );//向link对应的反转文档列表添加文档
	_updateLinkedTermlist( curlist, dist );//向link列表中添加link，该列表中存储文档中的全部link	

}

/**
* 更新LinkedTermList. |linkID,distance|.........|
*/
void link::api::InvLinkIndex::_updateLinkedTermlist(InvLinkDocList *curlist, link::api::DIST_T dist)
{
	LinkedTerm lt;
	lt.dist = dist;
	lt.linkID = curlist->linkID();
	this->linkedtermlist.push_back(lt);
	this->link_listlengths++;
}

/**
* 将已加入LinkedTerm的LinkList信息写入外存，并生成*.dl和*.dllookup文件。
*/
void link::api::InvLinkIndex::doendLinkDoc(const lemur::parse::DocumentProps *dp, int mgrid)
{
	//flush list and write to lookup table
	if (dp->length()>0) 
	{
		int linkcount;
		if(linkedtermlist.size()>0){
			linkcount = this->linkedtermlist.size();//获取文档中link的数目。
		}else{
			linkcount = 0;
		}

		linkrecord rec;
		rec.offset = writelinklist.tellp();//获取*.dl文件偏移
		rec.linkCount = linkcount;
		rec.num = mgrid;

		lemur::api::DOCID_T docID = counts[DOCS];

		dllookup.write( (char*) &rec, sizeof rec );//写入*.dllookup文件

		InvLinkInfoList *llist = new InvLinkInfoList(docID, linkcount, linkedtermlist);//动态分配内存
		llist->binWriteC(writelinklist);//写入*.dl文件

		delete llist;//释放内存
		linkcounts[TOTAL_LINKS] += linkcount;
	}//if
	linkedtermlist.clear();

	bool shouldWriteCache = (link_listlengths == -1);

	if( link_listlengths > _estimatePoint ) 
	{
		int cacheSize = _cacheLinkSize();//获取内存大小
		float bytesPerPosting = (float)cacheSize / (float)link_listlengths;
		_estimatePoint = (int) (_listsSize / bytesPerPosting);

		// if we're within a few postings of an optimal flush point, just do it now
		shouldWriteCache = _estimatePoint < (link_listlengths + KEYFILE_EPSILON_FLUSH_POSTINGS);//+0.5M
	}//if
	/*shouldWriteCache = true;*/
	if( shouldWriteCache ) 
	{
		writeLinkCache();
		link_listlengths = 0;
		_resetLinkEstimatePoint();
	}//if
}


void link::api::InvLinkIndex::writeLinkCache(bool finalRun)
{
	writeLinkCacheSegment();
	if( (_linksegments.size() >= LINK_KEYFILE_MAX_SEGMENTS) ||
		(_linksegments.size() > 1 && finalRun) )
	{
		mergeLinkCacheSegments();
	}

}

/**
* 将内存中的 inverted linkdocument list 写入 *.pl文件中。
*/
void link::api::InvLinkIndex::writeLinkCacheSegment()
{
	indri::utility::IndriTimer timer;
	timer.start();
  *msgstream << "writing inverted linkDocument list to disk\t";
  std::stringstream name;
  lemur::file::File outFile;

  // name of the file is <indexName>.ivl<segmentNumber>
  name << linknames[INVLINK_INDEX] << _linksegments.size();//创建*.plN

  // open an output stream, and wrap it in a file buffer that
  // gives us a megabyte of output cache -- seems to make writing
  // faster with these very small writes (system call overhead?)
  outFile.open( name.str().c_str(), 
                std::ofstream::out | std::ofstream::binary );//打开*.pl文件
  lemur::file::WriteBuffer out(outFile, KEYFILE_WRITEBUFFER_SIZE);

  int writes = 0;
  int totalSegments = _linksegments.size();

  for( int i = 0; i < linkcounts[UNIQUE_LINKS]; i++ ) //遍历当前的link列表
  {
	  InvLinkDocList* list = this->invlinklists[i];//获取link对应的反转文档列表
  
    if( list != NULL ) 
	{

      LinkData linkData;

	  linkData.documentCount = 0;
	  linkData.totallinkCount = 0;
	  
	  for(int j=0; j<this->winSize.size(); j++){
		  linkData.totallinkCountX[j] = 0;
	  }


      // update link statistics
      int actual = 0;
      
	  if( list->linkID() <= _largestFlushedLinkID ) 
	  {         
		  pllookup.get( list->linkID(), &linkData, actual, sizeof LinkData);
	  }


      linkData.documentCount += list->docFreq();
	  linkData.totallinkCount += list->linkCF();
	  for(int k=0; k<winSize.size(); k++){
		  linkData.totallinkCountX[k] += list->linkCF(winSize[k]);
	  }

	  lemur::api::COUNT_T vecLen;
	  lemur::api::LOC_T *byteVec = list->compInvLinkDocList(vecLen) ;//压缩数据，使用后必须释放内存

      // we need to add the location of the new inverted list
      int missingSegments = _MINIM<unsigned>( (sizeof(LinkData) - actual) / sizeof(SegmentOffset), 
                                              LINK_KEYFILE_MAX_SEGMENTS);
      int termSegments = LINK_KEYFILE_MAX_SEGMENTS - missingSegments;

      linkData.segments[ termSegments ].segment = totalSegments;
      linkData.segments[ termSegments ].offset = out.tellp();
      linkData.segments[ termSegments ].length = vecLen;
      pllookup.put( list->linkID(), &linkData, 
                     sizeof(LinkData) - sizeof(SegmentOffset)*(missingSegments-1) );//写入*.pllookup

      out.write( (char*) byteVec, vecLen );//先把压缩数据写入写缓存writebuffer中，如果超过1M则flush缓存数据，并并将缓存数据写入*.pl文件中。
      delete[](byteVec);
      delete list;
      invlinklists[i] = NULL;

      writes++;
    }
  }//for

  // flush the file buffer and close the segment stream
  out.flush();
  outFile.close();

	timer.stop();
	*msgstream << "execution time(minute:second.microsecond)";
	timer.printElapsedMicroseconds(*msgstream);
	*msgstream << std::endl;


  // reopen the output file as an input stream for queries:
  lemur::file::File* in = new lemur::file::File();
  in->open( name.str().c_str(), std::ifstream::in | std::ifstream::binary );
  _linksegments.push_back( in );

  // record the largest flushed linkID
  _largestFlushedLinkID = linkcounts[UNIQUE_LINKS];
  *msgstream << "[linkWrite]-[done]" << std::endl;

}

class reader_less
{
public:
	bool operator () ( const link::file::LinkDocListSegmentReader* one, 
					 const link::file::LinkDocListSegmentReader* two ) 
	{
		// uses operator< but priority queue expects the test to be >
		return (*two) < (*one);
	}
};

void link::api::InvLinkIndex::mergeLinkCacheSegments()
{
	indri::utility::IndriTimer mergeTime;
	mergeTime.start();
  *msgstream << "merging [link] segments";
  unsigned int firstMergeSegment = 0;

  if( firstMergeSegment == 0 ) 
  {
    pllookup.close();
    // g++ doesn't like this
    // need to move this ifdef out of here
#ifdef WIN32
    ::unlink( linknames[INVLINK_LOOKUP].c_str() );//删除*.pllookup文件
#else
    std::remove( linknames[INVLINK_LOOKUP].c_str() );
#endif
    pllookup.create( linknames[INVLINK_LOOKUP].c_str() );//创建*.pllookup文件
  }

  /** 在优先队列中，优先级高的元素先出队列。标准库默认使用元素类型的<操作符来确定它们之间的优先级关系,最小堆形式top */
  std::priority_queue<link::file::LinkDocListSegmentReader*,
						std::vector<link::file::LinkDocListSegmentReader*>,reader_less> readers;
  lemur::file::File outFile;
  std::vector<std::string> listNames;

  std::stringstream oldName;
  oldName << linknames[INVLINK_INDEX]<< _linksegments.size();
  outFile.open( oldName.str(), std::ofstream::out | std::ofstream::binary );//创建*.plN文件
  lemur::file::WriteBuffer out(outFile, KEYFILE_WRITEBUFFER_SIZE);  //为文件流创建缓冲区，大小为1M

  string nameToOpen(linknames[INVLINK_INDEX]);//*.pl
  lemur::file::File* fileToOpen;
  // open segments
  for( unsigned int i = firstMergeSegment; i < _linksegments.size(); i++ ) //遍历全部segment，分别从中读取第一条记录
  {
    //    g++ doesn't like this.
    fileToOpen =  _linksegments[i];//*.pl(0...N-1)
    link::file::LinkDocListSegmentReader* reader =  
      new link::file::LinkDocListSegmentReader(i, nameToOpen, fileToOpen, 
											KEYFILE_DOCLISTREADER_SIZE, this->winSize );//动态申请内存，创建segment 对应的 Reader 对象
    reader->pop(); // 从文件中读取第一条记录到内存中top
    // if it is empty, don't push it.
    if( reader->top() ) 
	{
      readers.push(reader);//保存到队列
    } 
	else //释放内存
	{
		reader->getFile()->unlink();//删除segment文件
      delete reader->getFile();//释放segment占用的内存
      delete reader;
    }
  }//for

  while( readers.size() ) //遍历队列中的每个元素
  {
    // find the minimum link id
    link::file::LinkDocListSegmentReader* reader = readers.top();//获取队列中linkid最小的一个元素
	// reindex the reader
    readers.pop();//从队列中删除termid最小的元素
	link::api::InvLinkDocList* list = reader->top();//获取当前doclist记录
	link::api::LINKID_T linkID = list->linkID();//获取linkid

    reader->pop();//从文件中读取下一条记录到内存中
    
    if( reader->top() ) 
	{
      readers.push(reader);//保存到队列
    } 
	else 
	{
		reader->getFile()->unlink();
      delete reader->getFile();
      delete reader;
    }

    // now, find all other readers with similar link lists
    while( readers.size() && readers.top()->top()->linkID() == linkID ) //从剩余元素中取出linkid最小的与当前linkid比较
	{
      reader = readers.top();//获取队列中linkid最小的一个元素

      // fetch the list, append it to the current list
      list->append( reader->top() );
	  // re-index the reader
      readers.pop();//已添加到列表中，从队列中删除该元素

      delete reader->top();

      reader->pop();//从文件中的读取下一条记录到内存中
      if( reader->top() ) 
	  {//如果SegmentReader中还有DocinfoList,则将SegmentReader再次加入已删除了该SegmentReader的容器中。
        readers.push(reader);
      } 
	  else 
	  {//如果没有，则删除该segment的pl文件。
        reader->getFile()->unlink();
        delete reader->getFile();
        delete reader;
      }
    }//while

    LinkData linkData;

	linkData.documentCount = list->docFreq();
	linkData.totallinkCount = list->linkCF();
	for(int i=0; i<winSize.size(); i++){
	  linkData.totallinkCountX[i] = list->linkCF(winSize[i]);
	}

    // now the list is complete, so write it out
    lemur::api::COUNT_T listLength;
	lemur::api::LOC_T* listBuffer = list->compInvLinkDocList( listLength );//动态分配内存，已释放
    lemur::file::File::offset_type offset = out.tellp();
    out.write( (char*) listBuffer, listLength );//写入*.plN文件
    delete[](listBuffer);//释放内存
    delete list;//释放内存

    // fetch the offset data from the lookup structure
    int actual = 0;

    if( firstMergeSegment > 0 ) 
	{
      invlookup.get( linkID, &linkData, actual, sizeof(LinkData) );
    }

    // we need to add the location of the new inverted list
    int missingSegments = _MINIM<unsigned>( (sizeof(LinkData) - actual) / sizeof(SegmentOffset), LINK_KEYFILE_MAX_SEGMENTS);
    int linkSegments = LINK_KEYFILE_MAX_SEGMENTS - missingSegments;

    // find the first segment offset that was merged in this pass
    int segIndex;
    for( segIndex = 0; segIndex < linkSegments; segIndex++ ) 
	{
      if( linkData.segments[segIndex].segment >= firstMergeSegment )
		  break;
    }

    linkData.segments[segIndex].segment = firstMergeSegment;
    linkData.segments[segIndex].offset = offset;
    linkData.segments[segIndex].length = listLength;

    pllookup.put( linkID, &linkData, sizeof(LinkData) - sizeof(SegmentOffset)*(LINK_KEYFILE_MAX_SEGMENTS-(segIndex+1)) );//写入*.pllookup文件
  }//while

  out.flush();
  outFile.close();

	mergeTime.stop();
  	*msgstream << "execution time(minute:second.microsecond)";
	mergeTime.printElapsedMicroseconds(*msgstream);
	*msgstream << std::endl;
	
  // all the old files are deleted now, so we can rename the 
  // merged segment to be segment <firstMergeSegment>.
  std::stringstream newName;
  newName << linknames[INVLINK_INDEX] << firstMergeSegment;
  lemur::file::File::rename( oldName.str(), newName.str() );

  // clear out the _segment vector (all the ifstreams have
  // already been deleted)
  _linksegments.erase( _linksegments.begin() + firstMergeSegment, _linksegments.end() );
  
  // open the final segment again for reading
  lemur::file::File* in = new lemur::file::File();
  in->open( newName.str(), std::ifstream::in | std::ifstream::binary );
  _linksegments.push_back( in );
  *msgstream << "[linkMerge][done]" << std::endl;	

}

/**
* 集合结束后将缓存中的数据写入文件，并且进行合并操作，以及生成文档管理文件和目录文件.key
*/
void link::api::InvLinkIndex::endCollection(const lemur::parse::CollectionProps *cp)
{
	// flush everything in the cache
	if (_readOnly) return;
	
	lastWriteCache();
	if(this->isLinkIndex){
		this->lastWriteLinkCache();
	}
	// write list of document managers
	writeDocMgrIDs();
	//write out the main toc file
	writeTOC(cp);	
	if(this->isLinkIndex){
		this->writeLinkTOC(cp);
	}
}

void link::api::InvLinkIndex::lastWriteLinkCache()
{
	this->link_listlengths = -1;
	this->writeLinkCache(true);
}

void link::api::InvLinkIndex::writeLinkTOC(const lemur::parse::CollectionProps *cp)
{
	// change format to match indri parameters xml format
	std::string fname = name + ".link_key";
	*msgstream << "Writing out main [link] stats table" << std::endl;

	ofstream toc(fname.c_str(),std::fstream::out);

	if (! toc.is_open()) 
	{
		fprintf(stderr, "Could not open .toc file for writing.\n");
		return;
	}

	toc << "<link-parameters>\n";
	toc << "<" << LINKVERSION_PAR << ">" << LINKINDEX_VERSION << "</" << LINKVERSION_PAR << ">\n";
	toc << "<" << NUMWINDOWS_PAR << ">" << winSize.size() << "</" << NUMWINDOWS_PAR<< ">\n";
	toc << "<" << SIZEWINDOWS_PAR << ">";
	for (int i=0;i<winSize.size();i++)
	{
		toc<<winSize[i]<<" ";
	}
	toc << "</" << SIZEWINDOWS_PAR<< ">\n";
	toc << "<" << NUMLINKS_PAR << ">" << linkcounts[TOTAL_LINKS] << "</" << NUMLINKS_PAR<< ">\n";
	toc << "<" << NUMULINKS_PAR << ">" << linkcounts[UNIQUE_LINKS] << "</" << NUMULINKS_PAR<< ">\n";
	toc << "<" << INVLINKINDEX_PAR << ">" << name << INVLIKINDEX << "</" << INVLINKINDEX_PAR<< ">\n";
	toc << "<" << INVLINKLOOKUP_PAR << ">" << name << INVLINKLOOKUP << "</" << INVLINKLOOKUP_PAR << ">\n";
	toc << "<" << DLINDEX_PAR << ">" << name << DLINDEX << "</" << DLINDEX_PAR << ">\n"; 
	toc << "<" << DLLOOKUP_PAR << ">" << name << DLLOOKUP << "</" << DLLOOKUP_PAR << ">\n";
	toc << "<" << LINKIDMAP_PAR << ">" << name << LINKIDMAP << "</" << LINKIDMAP_PAR << ">\n";
	toc << "<" << LINKIDPAIRMAP_PAR << ">" << name << LINKIDPAIRMAP << "</" << LINKIDPAIRMAP_PAR << ">\n";
	toc << "</link-parameters>\n";


//	std::cerr<<"file write pointer:"<<toc.tellp()<<std::endl;

	toc.close();
}

/** 包含link<term-term>的文档数目，即df(link) */
lemur::api::COUNT_T link::api::InvLinkIndex::docCount(link::api::LINKID_T linkID) const
{
	if((linkID <= 0) || (linkID > this->linkcounts[UNIQUE_LINKS]))
	{
		return 0;
	}

	LinkData linkData;
	int actual;
	bool success = this->pllookup.get(linkID, (char*) &linkData, actual, sizeof(linkData));
	if(success)
	{
		return linkData.documentCount;
	}
	else
	{
		return 0;
	}

}

link::api::InvLinkDocList* link::api::InvLinkIndex::internalLinkDocInfoList(link::api::LINKID_T linkID) const
{
	if ((linkID < 0) || (linkID > linkcounts[UNIQUE_LINKS]) ) 
	{
		*msgstream << "Error:  Trying to get InvLinkDocList for invalid linkID" 
			<< endl;
		*msgstream << "LinkID: " << linkID << endl;
		return NULL;
	}
	if (linkID == 0)
		return NULL;

	// we have to build the doc info list from the segments on disk
	int actual = 0;   
	LinkData linkData;

	bool success = pllookup.get( linkID, (char**) &linkData, actual, 
		sizeof linkData );//根据linkID从*.pllookup文件中读取对应的linkdata
	int i;

	if( success ) 
	{
		int missingSegments = (sizeof(LinkData) - actual) / sizeof(SegmentOffset);
		int vectorSegments = LINK_KEYFILE_MAX_SEGMENTS - missingSegments;
		std::vector<InvLinkDocList*> lists;  
		InvLinkDocList* total = 0;

		for( i = 0; i < vectorSegments; i++ ) 
		{
			char* buffer = new char[linkData.segments[i].length];

			lemur::file::File* segment = _linksegments[ linkData.segments[i].segment ];
			segment->seekg( linkData.segments[i].offset, std::ifstream::beg );//重定位输入流的位置
			segment->read( buffer, linkData.segments[i].length );//从*.pl文件读取经过RVL算法压缩的数据到缓冲区中

			if( !total ) 
			{
				total = new InvLinkDocList( (lemur::api::LOC_T*) buffer, this->winSize);
			} 
			else 
			{
				InvLinkDocList* segmentList = new InvLinkDocList( (lemur::api::LOC_T*) buffer );
				total->append( segmentList );
				delete segmentList;
			}

			delete[](buffer);
		}//for

		if( invlinklists[linkID-1] ) 
		{
			// fold in any data that hasn't made it to disk yet
			total->append( invlinklists[i] );
		}

		return total;
	}
	else 
	{
		return NULL;
	}	
}

lemur::api::DocInfoList* link::api::InvLinkIndex::InvlinkDocList(link::api::LINKID_T linkID) const
{
	if((linkID <= 0) || (linkID > this->linkcounts[UNIQUE_LINKS]))
	{
		return NULL;
	}

	lemur::api::DocInfoList* result = internalLinkDocInfoList(linkID);

	if( !result ) 
	{
		throw lemur::api::Exception( "lemur::index::KeyfileIncIndex::docInfoList", 
			"Failed to retrieve docInfoList" );
	}

	return result;

}

link::api::LINKID_T link::api::InvLinkIndex::linkTerm(const link::api::LINK_STR &pairWords) const
{
	const char* linkword = pairWords.c_str();
	if(pairWords.length() > (MAX_LINK_LENGTH - 1)) return 0;
	
	link::api::LINKID_T lid = _linkCache.find( linkword );

	if(lid <= 0)
	{
		int actual = 0;
		bool success = lIDs.get(linkword, &lid, actual, sizeof(lid) );

		if(!success)
		{
			lid = 0;
		}
	}
	return lid;
}

link::api::LINK_STR link::api::InvLinkIndex::linkTerm(const link::api::LINKID_T linkID) const
{
	if ((linkID < 0) || (linkID > linkcounts[UNIQUE_LINKS]))
		return "";
	int actual = 0;
	lSTRs.get( linkID, this->linkKey, actual, sizeof this->linkKey );
	linkKey[actual] = 0;
	return this->linkKey;
}

link::api::InvLinkInfoList* link::api::InvLinkIndex::linkInfoList(lemur::api::DOCID_T docID) const
{
	if ((docID <= 0) || (docID > counts[DOCS])) 
	{
		return NULL;
	}
	//we have to count the terms to make it bag of words from sequence
	InvLinkInfoList* tlist = (InvLinkInfoList *)linkInfoListSeq(docID);
	//tlist->countTerms();
	return tlist;
}

link::api::InvLinkInfoList* link::api::InvLinkIndex::linkInfoListSeq(lemur::api::DOCID_T docID) const
{
	if ((docID <= 0) || (docID > counts[DOCS]))
	{
		return NULL;
	}
	record r = fetchDocumentRecord( docID );
	// shouldn't really be re-using this varname like this
	writetlist.seekg( r.offset, std::fstream::beg );//*.dt
	InvLinkInfoList* tlist = new InvLinkInfoList();//动态分配内存，由调用者释放
	if (!tlist->binReadC(writetlist)) 
	{
		return NULL;
	} 
	return tlist;
}
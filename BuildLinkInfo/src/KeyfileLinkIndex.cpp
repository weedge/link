/*===========================================
 NAME	DATE			CONMENTS
 wy		02/2011			created
 wy     12/2011         try to find a good job , 
                        so lost many time~~~~.
						now add  interface to load linkindex~! 
 =================================================
 *
 */

#include "KeyfileLinkIndex.hpp"
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

link::api::KeyfileLinkIndex::KeyfileLinkIndex(const std::string &prefix, std::vector<int> &winSizes,
									  const string &clFile, bool usedCandLink, int cachesize, 
									  lemur::api::DOCID_T startdocid)
{
	assert( winSizes.size()>0 );
	this->isCandLinkIndex = usedCandLink;

	this->winSize.resize(winSizes.size());
	this->winSize = winSizes;

	ignoreDoc = false;
	link_listlengths = 0;
	setName(prefix);
	_readOnly = false;
	this->linkcounts = new link::api::LINKID_T[2];
	_computeMemoryBounds(cachesize);
	InitLinkIndex();

	if(this->isCandLinkIndex)
	{
		/*��ѡlink�ļ���ʽ��2�֣�trueΪ��termid-termid term-term linkDF;falseΪ��termid-termid linkDF,Ĭ��Ϊ:false*/
		this->candLinkFile = new link::file::CandidateLinkFile((lemur::api::ParamGetString("candlinkFromat", "false")=="true"));

		candFile.open(clFile.c_str(),std::ios::in|std::ios::binary);
		candLinkFile->openForReadtoVec(candFile);
	}
}

/** 
 * ����*.dl,*.dllookup,*.invlinklookup,*.lid,*.tidpair�ļ� 
 * ��keyfile�����ļ�*.dllookup,*.invlinklookup,*.lid,*.tidpair
 * ��д��δ��ѹ�����ļ�ͷ��Ϣ���� 4096 Byte
 */
void link::api::KeyfileLinkIndex::InitLinkIndex()
{
	this->_resetLinkEstimatePoint();

	//std::string filename;
	//filename = this->name + ".link_key";
	if(!tryOpenLinkfile())
	{
		*msgstream<<"Creating new linkindex "<<std::endl;
		curdocmgr = -1;

		linknames.resize(LINKNAMES_SIZE);

		linkcounts[TOTAL_LINKS] = linkcounts[UNIQUE_LINKS] = 0;
		std::stringstream docfname;
		docfname<<name<<DLINDEX;
		this->writelinklist.open(docfname.str(),ios::binary | ios::out | ios::ate);//open *.dl file

		linknames[INVLINK_LOOKUP] = name + INVLINKLOOKUP;//*.invlinklookup
		linknames[INVLINK_INDEX] = name + INVLIKINDEX;//*.invlink
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
* ��������ʱ�õ��ù��캯�������ø��๹�캯����ʼ����Ϣ��
*/
link::api::KeyfileLinkIndex::KeyfileLinkIndex()
{
	this->isCandLinkIndex = false;
	this->linkcounts = new link::api::LINKID_T[2];
	ignoreDoc = false;
	link_listlengths = 0;
	curdocmgr = -1;
	_readOnly = true;
	_computeMemoryBounds( 128 * 1024 * 1024 );
	_largestFlushedLinkID = 0;
}

/**
* �����������ͷ������õ����ڴ�ռ䡣
*/
link::api::KeyfileLinkIndex::~KeyfileLinkIndex()
{
	for(unsigned int i = 0; i< _linksegments.size(); i++ ) 
	{
		_linksegments[i]->close();
		delete _linksegments[i];
	}

	writelinklist.close();
	dllookup.close();
	invlinklookup.close();
	lIDs.close();
	lSTRs.close();

	delete dllookupReadBuffer;
	delete[](linkcounts);

	if(this->isCandLinkIndex){
		//this->candLinkFile.close();
		delete this->candLinkFile;
		this->candFile.close();
	}

}


int link::api::KeyfileLinkIndex::_cacheLinkSize()
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

void link::api::KeyfileLinkIndex::_resetLinkEstimatePoint()
{
	// how many postings should we queue up before
	// trying to estimate a flush time?
	// pick the worst case--one unique term per document
	// that would give us one InvLinkDocList plus a few bytes
	// for the list buffer:
	//std::cerr<<"sizeof(InvLinkDocList):"<<sizeof(InvLinkDocList)<<std::endl;
	_estimatePoint = _listsSize / (sizeof(InvLinkDocList) + 16);//120+16

}

void link::api::KeyfileLinkIndex::_computeMemoryBounds( int memorySize ) 
{
	// The following is fixed allocation--in the future
	// we may want to try something parametric:
	_memorySize = memorySize;
	_listsSize = memorySize - KEYFILE_BASE_MEMORY_USAGE;

	if( _listsSize < KEYFILE_MINIMUM_LISTS_SIZE ) 
	{
		_listsSize = KEYFILE_MINIMUM_LISTS_SIZE;
	}

	_resetLinkEstimatePoint();
}

bool link::api::KeyfileLinkIndex::tryOpenLinkfile()
{
	std::string filename;
	filename = this->name + ".link_key";
	return open(filename);
}

void link::api::KeyfileLinkIndex::setMesgStream(ostream * lemStream) {
	msgstream = lemStream;
}

void link::api::KeyfileLinkIndex::setName(const string &prefix) {
	name = prefix;
}

/**
* �̳и���Index��open����,����ʹ�õĽӿڣ����ڴ򿪼�������������
* ����*.key�ļ���д����*.dl,*.dllookup,*.invlinklookup,*.lid,*.tidpair�ļ���
*/
bool link::api::KeyfileLinkIndex::open(const std::string &indexName)
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


	fullLinkToc();

	invlinklists.resize(linkcounts[UNIQUE_LINKS], 0 );//Bug:(link��Ŀ��ʱ;vector ��������)��invlinklist.max_size()���ԡ�
	_largestFlushedLinkID = linkcounts[UNIQUE_LINKS];

	std::string prefix = indexName.substr( 0, indexName.rfind('.'));
	setName(prefix);

	openLinkDBs();//�� *.lid, *.lidstr, *.invlinklookup, *.dllookup�ļ�, �Լ���*.did, *.didstr,
	openLinkSegments();//��*.invlink�ļ�

	std::stringstream docfname;
	docfname << name << DLINDEX;
	if (_readOnly){
		writelinklist.open( docfname.str(), ios::in | ios::binary );//��*.dl�ļ�
	}else{ 
		writelinklist.open( docfname.str(), 
					 ios::in | ios::out | ios::binary | ios::ate );
		writelinklist.seekg( 0, std::ios::beg );
	}
	lemur::api::ParamPopFile();

	*msgstream << "Load link index complete." << endl;
	return true;
}

void link::api::KeyfileLinkIndex::fullLinkToc()
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
* �� *.lid, *.lidstr, *.invlinklookup, *.dllookup�ļ�
*/
void link::api::KeyfileLinkIndex::openLinkDBs()
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
	invlinklookup.open( linknames[INVLINK_LOOKUP], cacheSize, _readOnly );
	lIDs.open( linknames[LINK_IDS], cacheSize, _readOnly  );
	lSTRs.open( linknames[LINK_IDSTRS], cacheSize, _readOnly  );
}

/**
* �� *.invlink�ļ�
*/
void link::api::KeyfileLinkIndex::openLinkSegments()
{
	// open existing inverted list segments
	for( int i=0; i<LINK_KEYFILE_MAX_SEGMENTS; i++ ) 
	{
		lemur::file::File* in = new lemur::file::File();
		std::stringstream name;
		name << linknames[INVLINK_INDEX] << i;

		in->open( name.str().c_str(), std::fstream::in | std::fstream::binary );//�Զ��Ͷ����Ƶķ�ʽ��*.invlink�ļ�

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
* ����*.dllookup,*.invlinklookup,*.lid,*.tidpair�ļ���
*/
void link::api::KeyfileLinkIndex::createLinkDBs()
{
	this->dllookup.open(linknames[DOCLINK_LOOKUP],std::ios::binary | std::ios::out | std::ios::in);
	this->dllookup.seekg(0, std::ios::beg);
	this->dllookup.seekp(0, std::ios::end);
	this->dllookupReadBuffer = new lemur::file::ReadBuffer(dllookup, 10*1024*1024);

	this->invlinklookup.create(linknames[INVLINK_LOOKUP] );
	this->lIDs.create(linknames[LINK_IDS] );
	this->lSTRs.create(linknames[LINK_IDSTRS] );
	
}

/**
* һƪ�ĵ������󣬽����ɵ�link��Ϣд������ļ��С�
*/
void link::api::KeyfileLinkIndex::endDoc(const link::api::LinkInfo *linkinfo)
{
	//std::cerr<<"docID:"<<dp->stringID()<<std::endl;
	if(!this->ignoreDoc)
	{//δ֪�ĵ�
		this->doendLinkDoc(linkinfo,this->curdocmgr);
	}
	this->ignoreDoc = false;
}

/**
* ����Link<termID,termID>�����Ϣ����ͳ�������Ϣ:Fre(X) 
* ��Ҫ�����ĵ�����������ʱ������һƪ�ĵ������ĵ��е�link��Ϣ,��Ҫͳ���ĵ���ÿ���ʵ�λ����Ϣ��
*/
bool link::api::KeyfileLinkIndex::createLinkInfo(const lemur::parse::DocumentProps *dp)
{
	////std::cerr<<"docID:"<<dp->stringID()<<std::endl;
	////link::api::Link link_item;
	//if(dp->length() <= 0) return false;

	////std::map< link::api::LINK_STR, std::vector<link::api::DIST_T> > link_dists;

	//vector<lemur::index::LocatedTerm>::iterator left_iter = this->termlist.begin();
	//for(; left_iter != this->termlist.end(); left_iter++)
	//{
	//	//link_item.setLeftID( (*left_iter).term );

	//	vector<lemur::index::LocatedTerm>::iterator right_iter = left_iter+1;
	//	for(; right_iter != this->termlist.end(); right_iter++)
	//	{
	//		if((*left_iter).term != (*right_iter).term)
	//		{
	//			//linkcounts[TOTAL_LINKS]++;

	//			//link_item.setRightID( (*right_iter).term );	
	//			//link_item.linkProcess();
	//			//link_item.setDistace(abs( (*right_iter).loc - (*left_iter).loc ));
	//			
	//			link::api::LinkInfo linkinfo((*left_iter).term, (*right_iter).term);
	//			linkinfo.setDistace(abs( (*right_iter).loc - (*left_iter).loc ));

	//			if(this->isCandLinkIndex)
	//			{/*����к�ѡlink,��Ӻ�ѡlink��Ϣ������ļ���,���������ȫ��link��Ϣ������ļ��С�*/

	//				/*��ѡlink�ļ���ʽ��2�֣�trueΪ��termid-termid term-term; falseΪ��termid-termid */
	//				//link::file::CandidateLinkFile clFile(true);
	//				//clFile.openForRead(this->candLinkFile);
	//				//lemur::api::COUNT_T linkDF;
	//				if(candLinkFile->findLink(linkinfo.toString()))
	//				{
	//					//std::cerr<<"the link:"<<linkinfo.toString()<<" the linkDF:"<<linkDF<<std::endl;
	//					this->addLink(linkinfo);
	//				}
	//			}
	//			else
	//			{
	//				this->addLink(linkinfo);//���link��Ϣ������ļ��С�
	//			}
	// 
	//			//link_dists[linkinfo.toString()].push_back(linkedterm.dist);  
	//		}else{
	//			continue;
	//		}
	//		
	//	}	
	//	
	//}
	return true;
}
void link::api::KeyfileLinkIndex::addLinkInfotoLinkDB(lemur::api::Index *index)
{

	lemur::api::TERMID_T uTerm = index->termCountUnique();
	for(int i=1; i<uTerm; i++)
	{
		lemur::api::TERMID_T leftTID = i;
		cerr<<index->term(i)<<"("<<leftTID<<")"<<endl;
		lemur::api::DocInfoList* leftTList = index->docInfoList(leftTID);
		for (int j=i+1; j<=uTerm; j++)
		{
			
			lemur::api::TERMID_T rightTID = j;
			link::api::LinkInfo li(leftTID,rightTID);
			//std::cerr<<"-"<<index->term(j)<<"("<<rightTID<<")"<<endl;


			lemur::api::DocInfoList* rightTList = index->docInfoList(rightTID);

			leftTList->startIteration();
			rightTList->startIteration();

			lemur::api::DocInfo* leftTdocInfo;
			lemur::api::DocInfo* rightTdocInfo;

			lemur::api::DOCID_T leftdocID;
			lemur::api::DOCID_T rightdocID;
			lemur::api::COUNT_T leftTF ;
			lemur::api::COUNT_T rightTF ;
			lemur::api::COUNT_T linkDF = 0 ;
			const lemur::api::LOC_T* leftPos ;
			const lemur::api::LOC_T* rightPos;


			if (leftTList->hasMore() && rightTList->hasMore())
			{
				leftTdocInfo = leftTList->nextEntry();
				rightTdocInfo = rightTList->nextEntry();
			}

			bool end = false;

			while (!end)
			{
				leftdocID = leftTdocInfo->docID();
				rightdocID = rightTdocInfo->docID();

				if (leftdocID == rightdocID)
				{
					//cerr<<endl<<leftdocID<<endl;
					li.setDocID(leftdocID);	
					std::vector<link::api::DIST_T> distances;
					leftTF = leftTdocInfo->termCount();
					rightTF = rightTdocInfo->termCount();
					leftPos = leftTdocInfo->positions();
					rightPos = rightTdocInfo->positions();
					if (leftPos != NULL && rightPos != NULL)
					{
						for (int i=0; i<leftTF; i++)
						{
							//cerr<<leftPos[i]<<" ";
							for (int j=0; j<rightTF; j++)
							{
								//cerr<<rightPos[j]<<" ";
								int dist = abs(leftPos[i]-rightPos[j]);
								li.setDistance(dist);
								addLink(li);
								distances.push_back(dist);
							}
						}
					}

					link_listlengths++;
					li.setDistances(distances);
					//addLink(li);

					if (leftTList->hasMore() && rightTList->hasMore())
					{
						leftTdocInfo = leftTList->nextEntry();
						rightTdocInfo = rightTList->nextEntry();
					}else{
						end = true;
					}
				}
				else if (leftdocID > rightdocID)
				{
					if(rightTList->hasMore())
					{
						rightTdocInfo = rightTList->nextEntry();
					}
					else
					{
						end = true;
					}	
				}
				else
				{
					if(leftTList->hasMore())
					{
						leftTdocInfo = leftTList->nextEntry();
					}
					else
					{
						end = true;
					}
				}
			}//end while

			delete rightTList;
			rightTList = NULL;

			estimatePointToWriteCahce();
		}
		delete leftTList;
		leftTList = NULL;

	}//end for
	lastWriteLinkCache();
	// write list of document managers
	//writeDocMgrIDs();
	//write out the main toc file
	this->writeLinkTOC();

}
link::api::LINKCOUNT_T link::api::KeyfileLinkIndex::getCandLinkCount()
{
	link::api::LINKCOUNT_T linkcount = 0;
	if(candFile.is_open()){
		this->candLinkFile->openForRead(candFile);
	}
	while (candLinkFile->hasMore())
	{
		link::api::LINK_STR strLink = candLinkFile->nextLink();
		candLinkFile->nextLine();
		linkcount++;
	}
	return linkcount;
}
void link::api::KeyfileLinkIndex::addCandLinkInfotoLinkDB(lemur::api::Index *index)
{
	link::api::LinkInfo li;
	if(candFile.is_open()){
		this->candLinkFile->openForRead(candFile);
	}
	while (candLinkFile->hasMore())
	{
		link::api::LINK_STR strLink = candLinkFile->nextLink();
		candLinkFile->nextLine();
		li.setLinkStr(strLink);
		li.linkToPair(strLink);
		lemur::api::TERMID_T leftTID = li.getLeftID();
		lemur::api::TERMID_T rightTID = li.getRightID();

		std::cerr<<leftTID<<"-"<<rightTID<<std::endl;

		lemur::api::DocInfoList* leftTList = index->docInfoList(leftTID);
		lemur::api::DocInfoList* rightTList = index->docInfoList(rightTID);

		leftTList->startIteration();
		rightTList->startIteration();

		lemur::api::DocInfo* leftTdocInfo;
		lemur::api::DocInfo* rightTdocInfo;

		lemur::api::DOCID_T leftdocID;
		lemur::api::DOCID_T rightdocID;
		lemur::api::COUNT_T leftTF ;
		lemur::api::COUNT_T rightTF ;
		lemur::api::COUNT_T linkDF = 0 ;
		const lemur::api::LOC_T* leftPos ;
		const lemur::api::LOC_T* rightPos;


		if (leftTList->hasMore() && rightTList->hasMore())
		{
			leftTdocInfo = leftTList->nextEntry();
			rightTdocInfo = rightTList->nextEntry();
		}

		bool end = false;

		while (!end)
		{
			leftdocID = leftTdocInfo->docID();
			rightdocID = rightTdocInfo->docID();
			
			if (leftdocID == rightdocID)
			{
				//cerr<<endl<<leftdocID<<endl;
				li.setDocID(leftdocID);	
				std::vector<link::api::DIST_T> distances;
				leftTF = leftTdocInfo->termCount();
				rightTF = rightTdocInfo->termCount();
				leftPos = leftTdocInfo->positions();
				rightPos = rightTdocInfo->positions();
				if (leftPos != NULL && rightPos != NULL)
				{
					for (int i=0; i<leftTF; i++)
					{
						//cerr<<leftPos[i]<<" ";
						for (int j=0; j<rightTF; j++)
						{
							//cerr<<rightPos[j]<<" ";
							int dist = abs(leftPos[i]-rightPos[j]);
							li.setDistance(dist);
							addLink(li);
							distances.push_back(dist);
						}
					}
				}

				link_listlengths++;
				li.setDistances(distances);
				//addLink(li);
				
				if (leftTList->hasMore() && rightTList->hasMore())
				{
					leftTdocInfo = leftTList->nextEntry();
					rightTdocInfo = rightTList->nextEntry();
				}else{
					end = true;
				}
			}
			else if (leftdocID > rightdocID)
			{
				if(rightTList->hasMore())
				{
					rightTdocInfo = rightTList->nextEntry();
				}
				else
				{
					end = true;
				}	
			}
			else
			{
				if(leftTList->hasMore())
				{
					leftTdocInfo = leftTList->nextEntry();
				}
				else
				{
					end = true;
				}
			}
		}//end while
		
		delete rightTList;
		rightTList = NULL;
		delete leftTList;
		leftTList = NULL;
		estimatePointToWriteCahce();
		
	}//end while
	lastWriteLinkCache();
	// write list of document managers
	//writeDocMgrIDs();
	//write out the main toc file
	this->writeLinkTOC();
	
}

/**
* �������õ�link�����Ϣ����InvLinkDocList�У�
* �������ӳ��linkID<->(termID,termID)��keyfile�����ļ��У�*.lid,*.tidpair�ļ� 
*/
bool link::api::KeyfileLinkIndex::addLink(const link::api::Link &l)
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


  link::api::LINKID_T lid = _linkCache.find(linkinfo->toString().c_str());//����link�Ƿ��Ѿ�������cache��

  if( lid > 0 ) //link�Ѿ������棬ֱ�Ӽ��뵽������
  {
	  addKnownLink( lid, linkinfo);
  } 
  else //linkδ������
  {
    lid = addUncachedLink( linkinfo );
	_linkCache.add( linkinfo->toString().c_str(), lid );//��cache�������link
  }

  return true;

}


link::api::LINKID_T link::api::KeyfileLinkIndex::addUncachedLink(const link::api::LinkInfo* linkinfo)
{
	link::api::LINKID_T lid;
	int actual = 0;
    
	if( lIDs.get( linkinfo->toString().c_str(), &lid, actual, sizeof lid ) ) //��link�Ѿ����ֹ�
	{
		addKnownLink( lid, linkinfo);
	} 
	else //�����term
	{
		lid = addUnknownLink( linkinfo );
	}

	return lid;

}

link::api::LINKID_T link::api::KeyfileLinkIndex::addUnknownLink(const link::api::LinkInfo* linkinfo)
{

	// update unique link counter
	linkcounts[UNIQUE_LINKS]++;
	link::api::LINKID_T linkID = linkcounts[UNIQUE_LINKS];//����link��ID

	InvLinkDocList* curlist = new InvLinkDocList(linkID, this->winSize);//link��Ӧ�ķ�ת�ĵ��б�
	
	/*LinkDocumentList* curlist = new LinkDocumentList(termID, term->strLength());*/
	invlinklists.push_back( curlist );  // link is added at index linkID-1

	// store the link in the keyfiles (lIDs, lSTRs)
	addLinkLookup( linkID, linkinfo->toString().c_str() );

	addKnownLink( linkID, linkinfo);
	return linkID;
}

void link::api::KeyfileLinkIndex::addLinkLookup(link::api::LINKID_T linkKey, const char *linkSpelling)
{
	lSTRs.put(linkKey, (void*) linkSpelling, strlen(linkSpelling));
	lIDs.put(linkSpelling, &linkKey, sizeof linkKey );
}


void link::api::KeyfileLinkIndex::addKnownLink(link::api::LINKID_T linkID, const link::api::LinkInfo* linkinfo)
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

	curlist->addFrequence(linkinfo->getDocID(), linkinfo->getDistance() );//��link��Ӧ�ķ�ת�ĵ��б�����ĵ�
	//_updateLinkedTermlist( curlist, linkinfo->getDistance() );//��link�б������link�����б��д洢�ĵ��е�ȫ��link	

}

/**
* ����LinkedTermList. |linkID,distance|.........|
*/
void link::api::KeyfileLinkIndex::_updateLinkedTermlist(InvLinkDocList *curlist, link::api::DIST_T dist)
{
	LinkedTerm lt;
	lt.dist = dist;
	lt.linkID = curlist->linkID();
	this->linkedtermlist.push_back(lt);
	this->link_listlengths++;
}

void link::api::KeyfileLinkIndex::estimatePointToWriteCahce()
{
	bool shouldWriteCache = (link_listlengths == -1);

	if( link_listlengths > _estimatePoint ) 
	{
		int cacheSize = _cacheLinkSize();//��ȡ�ڴ��С
		float bytesPerPosting = (float)cacheSize / (float)link_listlengths;
		_estimatePoint = (int) (_listsSize / bytesPerPosting);

		// if we're within a few postings of an optimal flush point, just do it now
		shouldWriteCache = _estimatePoint < (link_listlengths + KEYFILE_EPSILON_FLUSH_POSTINGS);//+0.5M
	}//if
	//shouldWriteCache = true;
	if( shouldWriteCache ) 
	{
		writeLinkCache();
		link_listlengths = 0;
		_resetLinkEstimatePoint();
	}//if
}

/**
* ���Ѽ���LinkedTerm��LinkList��Ϣд����棬������*.dl��*.dllookup�ļ���
*/
void link::api::KeyfileLinkIndex::doendLinkDoc(const link::api::LinkInfo *linkinfo, int mgrid)
{
	//flush list and write to lookup table
	//if (dp->length()>0) 
	//{
		int linkcount;
		if(linkedtermlist.size()>0){
			linkcount = this->linkedtermlist.size();//��ȡ�ĵ���link����Ŀ��
		}else{
			linkcount = 0;
		}

		linkrecord rec;
		rec.offset = writelinklist.tellp();//��ȡ*.dl�ļ�ƫ��
		rec.linkCount = linkcount;
		rec.num = mgrid;

		lemur::api::DOCID_T docID = linkinfo->getDocID();

		dllookup.write( (char*) &rec, sizeof rec );//д��*.dllookup�ļ�

		InvLinkInfoList *llist = new InvLinkInfoList(docID, linkcount, linkedtermlist);//��̬�����ڴ�
		llist->binWriteC(writelinklist);//д��*.dl�ļ�

		delete llist;//�ͷ��ڴ�
		linkcounts[TOTAL_LINKS] += linkcount;
	//}//if
	linkedtermlist.clear();

	estimatePointToWriteCahce();

}


void link::api::KeyfileLinkIndex::writeLinkCache(bool finalRun)
{
	writeLinkCacheSegment();
	if( (_linksegments.size() >= LINK_KEYFILE_MAX_SEGMENTS) ||
		(_linksegments.size() > 1 && finalRun) )
	{
		mergeLinkCacheSegments();
	}

}

/**
* ���ڴ��е� inverted linkdocument list д�� *.invlink�ļ��С�
*/
void link::api::KeyfileLinkIndex::writeLinkCacheSegment()
{
	indri::utility::IndriTimer timer;
	timer.start();
  *msgstream << "writing inverted linkDocument list to disk\t";
  std::stringstream name;
  lemur::file::File outFile;

  // name of the file is <indexName>.ivl<segmentNumber>
  name << linknames[INVLINK_INDEX] << _linksegments.size();//����*.invlinkN

  // open an output stream, and wrap it in a file buffer that
  // gives us a megabyte of output cache -- seems to make writing
  // faster with these very small writes (system call overhead?)
  outFile.open( name.str().c_str(), 
                std::ofstream::out | std::ofstream::binary );//��*.invlink�ļ�
  lemur::file::WriteBuffer out(outFile, KEYFILE_WRITEBUFFER_SIZE);

  int writes = 0;
  int totalSegments = _linksegments.size();

  for( int i = 0; i < linkcounts[UNIQUE_LINKS]; i++ ) //������ǰ��link�б�
  {
	  InvLinkDocList* list = this->invlinklists[i];//��ȡlink��Ӧ�ķ�ת�ĵ��б�
  
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
		  invlinklookup.get( list->linkID(), &linkData, actual, sizeof LinkData);
	  }


      linkData.documentCount += list->docFreq();
	  linkData.totallinkCount += list->linkCF();
	  for(int k=0; k<winSize.size(); k++){
		  linkData.totallinkCountX[k] += list->linkCF(winSize[k]);
	  }

	  lemur::api::COUNT_T vecLen;
	  lemur::api::LOC_T *byteVec = list->compInvLinkDocList(vecLen) ;//ѹ�����ݣ�ʹ�ú�����ͷ��ڴ�

      // we need to add the location of the new inverted list
      int missingSegments = _MINIM<unsigned>( (sizeof(LinkData) - actual) / sizeof(SegmentOffset), 
                                              LINK_KEYFILE_MAX_SEGMENTS);
      int termSegments = LINK_KEYFILE_MAX_SEGMENTS - missingSegments;

      linkData.segments[ termSegments ].segment = totalSegments;
      linkData.segments[ termSegments ].offset = out.tellp();
      linkData.segments[ termSegments ].length = vecLen;
      invlinklookup.put( list->linkID(), &linkData, 
                     sizeof(LinkData) - sizeof(SegmentOffset)*(missingSegments-1) );//д��*.invlinklookup

      out.write( (char*) byteVec, vecLen );//�Ȱ�ѹ������д��д����writebuffer�У��������1M��flush�������ݣ���������������д��*.invlink�ļ��С�
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

void link::api::KeyfileLinkIndex::mergeLinkCacheSegments()
{
	indri::utility::IndriTimer mergeTime;
	mergeTime.start();
  *msgstream << "merging [link] segments";
  unsigned int firstMergeSegment = 0;

  if( firstMergeSegment == 0 ) 
  {
    invlinklookup.close();
    // g++ doesn't like this
    // need to move this ifdef out of here
#ifdef WIN32
    ::unlink( linknames[INVLINK_LOOKUP].c_str() );//ɾ��*.invlinklookup�ļ�
#else
    std::remove( linknames[INVLINK_LOOKUP].c_str() );
#endif
    invlinklookup.create( linknames[INVLINK_LOOKUP].c_str() );//����*.invlinklookup�ļ�
  }

  /** �����ȶ����У����ȼ��ߵ�Ԫ���ȳ����С���׼��Ĭ��ʹ��Ԫ�����͵�<��������ȷ������֮������ȼ���ϵ,��С����ʽtop */
  std::priority_queue<link::file::LinkDocListSegmentReader*,
						std::vector<link::file::LinkDocListSegmentReader*>,reader_less> readers;
  lemur::file::File outFile;
  std::vector<std::string> listNames;

  std::stringstream oldName;
  oldName << linknames[INVLINK_INDEX]<< _linksegments.size();
  outFile.open( oldName.str(), std::ofstream::out | std::ofstream::binary );//����*.invlinkN�ļ�
  lemur::file::WriteBuffer out(outFile, KEYFILE_WRITEBUFFER_SIZE);  //Ϊ�ļ�����������������СΪ1M

  string nameToOpen(linknames[INVLINK_INDEX]);//*.invlink
  lemur::file::File* fileToOpen;
  // open segments
  for( unsigned int i = firstMergeSegment; i < _linksegments.size(); i++ ) //����ȫ��segment���ֱ���ж�ȡ��һ����¼
  {
    //    g++ doesn't like this.
    fileToOpen =  _linksegments[i];//*.invlink(0...N-1)
    link::file::LinkDocListSegmentReader* reader =  
      new link::file::LinkDocListSegmentReader(i, nameToOpen, fileToOpen, 
											KEYFILE_DOCLISTREADER_SIZE, this->winSize );//��̬�����ڴ棬����segment ��Ӧ�� Reader ����
    reader->pop(); // ���ļ��ж�ȡ��һ����¼���ڴ���top
    // if it is empty, don't push it.
    if( reader->top() ) 
	{
      readers.push(reader);//���浽����
    } 
	else //�ͷ��ڴ�
	{
		reader->getFile()->unlink();//ɾ��segment�ļ�
      delete reader->getFile();//�ͷ�segmentռ�õ��ڴ�
      delete reader;
    }
  }//for

  while( readers.size() ) //���������е�ÿ��Ԫ��
  {
    // find the minimum link id
    link::file::LinkDocListSegmentReader* reader = readers.top();//��ȡ������linkid��С��һ��Ԫ��
	// reindex the reader
    readers.pop();//�Ӷ�����ɾ��termid��С��Ԫ��
	link::api::InvLinkDocList* list = reader->top();//��ȡ��ǰdoclist��¼
	link::api::LINKID_T linkID = list->linkID();//��ȡlinkid

    reader->pop();//���ļ��ж�ȡ��һ����¼���ڴ���
    
    if( reader->top() ) 
	{
      readers.push(reader);//���浽����
    } 
	else 
	{
		reader->getFile()->unlink();
      delete reader->getFile();
      delete reader;
    }

    // now, find all other readers with similar link lists
    while( readers.size() && readers.top()->top()->linkID() == linkID ) //��ʣ��Ԫ����ȡ��linkid��С���뵱ǰlinkid�Ƚ�
	{
      reader = readers.top();//��ȡ������linkid��С��һ��Ԫ��

      // fetch the list, append it to the current list
      list->append( reader->top() );
	  // re-index the reader
      readers.pop();//����ӵ��б��У��Ӷ�����ɾ����Ԫ��

      delete reader->top();

      reader->pop();//���ļ��еĶ�ȡ��һ����¼���ڴ���
      if( reader->top() ) 
	  {//���SegmentReader�л���DocinfoList,��SegmentReader�ٴμ�����ɾ���˸�SegmentReader�������С�
        readers.push(reader);
      } 
	  else 
	  {//���û�У���ɾ����segment��invlink�ļ���
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
	lemur::api::LOC_T* listBuffer = list->compInvLinkDocList( listLength );//��̬�����ڴ棬���ͷ�
    lemur::file::File::offset_type offset = out.tellp();
    out.write( (char*) listBuffer, listLength );//д��*.invlinkN�ļ�
    delete[](listBuffer);//�ͷ��ڴ�
    delete list;//�ͷ��ڴ�

    // fetch the offset data from the lookup structure
    int actual = 0;

    if( firstMergeSegment > 0 ) 
	{
      invlinklookup.get( linkID, &linkData, actual, sizeof(LinkData) );
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

    invlinklookup.put( linkID, &linkData, sizeof(LinkData) - sizeof(SegmentOffset)*(LINK_KEYFILE_MAX_SEGMENTS-(segIndex+1)) );//д��*.invlinklookup�ļ�
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

void link::api::KeyfileLinkIndex::writeDocMgrIDs() {
	std::string dmname = name + DOCMGRMAP;
	std::ofstream dmID;

	dmID.open( dmname.c_str(), std::ofstream::out );
	for ( unsigned int i=0;i<docmgrs.size();i++ ) {
		dmID << i << " "
			<< docmgrs[i].length() << " "
			<< docmgrs[i] << std::endl;
	}
	dmID.close();
}

/**
* ���Ͻ����󽫻����е�����д���ļ������ҽ��кϲ��������Լ������ĵ������ļ���Ŀ¼�ļ�.link_key
*/
void link::api::KeyfileLinkIndex::endCollection(const lemur::parse::CollectionProps *cp)
{
	// flush everything in the cache
	if (_readOnly) return;
	
	this->lastWriteLinkCache();
	// write list of document managers
	writeDocMgrIDs();
	//write out the main toc file
	this->writeLinkTOC();
}

void link::api::KeyfileLinkIndex::lastWriteLinkCache()
{
	this->link_listlengths = -1;
	this->writeLinkCache(true);
}

void link::api::KeyfileLinkIndex::writeLinkTOC()
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

/** ����link<term-term>���ĵ���Ŀ����df(link) */
lemur::api::COUNT_T link::api::KeyfileLinkIndex::docCount(link::api::LINKID_T linkID) const
{
	if((linkID <= 0) || (linkID > this->linkcounts[UNIQUE_LINKS]))
	{
		return 0;
	}

	LinkData linkData;
	int actual;
	bool success = this->invlinklookup.get(linkID, (char*) &linkData, actual, sizeof(linkData));
	if(success)
	{
		return linkData.documentCount;
	}
	else
	{
		return 0;
	}

}

link::api::InvLinkDocList* link::api::KeyfileLinkIndex::internalLinkDocInfoList(link::api::LINKID_T linkID) const
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

	bool success = invlinklookup.get( linkID, (char**) &linkData, actual, 
		sizeof linkData );//����linkID��*.invlinklookup�ļ��ж�ȡ��Ӧ��linkdata
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
			segment->seekg( linkData.segments[i].offset, std::ifstream::beg );//�ض�λ��������λ��
			segment->read( buffer, linkData.segments[i].length );//��*.invlink�ļ���ȡ����RVL�㷨ѹ�������ݵ���������

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

lemur::api::DocInfoList* link::api::KeyfileLinkIndex::InvlinkDocList(link::api::LINKID_T linkID) const
{
	if((linkID <= 0) || (linkID > this->linkcounts[UNIQUE_LINKS]))
	{
		return NULL;
	}

	lemur::api::DocInfoList* result = internalLinkDocInfoList(linkID);

	if( !result ) 
	{
		throw lemur::api::Exception( "link::api::KeyfileLinkIndex::InvlinkDocList", 
			"Failed to retrieve docInfoList" );
	}

	return result;

}

link::api::LINKID_T link::api::KeyfileLinkIndex::linkTerm(const link::api::LINK_STR &pairWords) const
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

link::api::LINK_STR link::api::KeyfileLinkIndex::linkTerm(const link::api::LINKID_T linkID) const
{
	if ((linkID < 0) || (linkID > linkcounts[UNIQUE_LINKS]))
		return "";
	int actual = 0;
	lSTRs.get( linkID, this->linkKey, actual, sizeof this->linkKey );
	linkKey[actual] = 0;
	return this->linkKey;
}

link::api::InvLinkInfoList* link::api::KeyfileLinkIndex::linkInfoList(lemur::api::DOCID_T docID) const
{
	if ((docID <= 0) ) 
	{
		return NULL;
	}
	//we have to count the terms to make it bag of words from sequence
	InvLinkInfoList* tlist = (InvLinkInfoList *)linkInfoListSeq(docID);
	//tlist->countTerms();
	return tlist;
}

link::api::InvLinkInfoList* link::api::KeyfileLinkIndex::linkInfoListSeq(lemur::api::DOCID_T docID) const
{
	if ((docID <= 0) )
	{
		return NULL;
	}
	linkrecord r = fetchDocumentRecord( docID );
	// shouldn't really be re-using this varname like this
	writelinklist.seekg( r.offset, std::fstream::beg );//*.dl
	InvLinkInfoList* tlist = new InvLinkInfoList();//��̬�����ڴ棬�ɵ������ͷ�
	if (!tlist->binReadC(writelinklist)) 
	{
		return NULL;
	} 
	return tlist;
}

link::api::KeyfileLinkIndex::linkrecord link::api::KeyfileLinkIndex::fetchDocumentRecord( lemur::api::DOCID_T key ) const
{
	if (key == 0)
		return linkrecord();

	linkrecord rec;

	dllookupReadBuffer->seekg( (key-1)*sizeof(rec), std::fstream::beg );
	dllookupReadBuffer->read( (char*) &rec, sizeof rec );

	return rec;
}

link::api::LINKCOUNT_T link::api::KeyfileLinkIndex::linkCount(link::api::LINKID_T linkID) const
{
	if((linkID <= 0) || (linkID > this->linkcounts[UNIQUE_LINKS]))
	{
		return 0;
	}

	LinkData linkData;
	int actual;
	bool success = this->invlinklookup.get(linkID, (char*) &linkData, actual, sizeof(linkData));
	if(success)
	{
		return linkData.totallinkCount;
	}
	else
	{
		return 0;
	}

}

link::api::LINKCOUNT_T* link::api::KeyfileLinkIndex::linkCountX(link::api::LINKID_T linkID) const
{
	if((linkID <= 0) || (linkID > this->linkcounts[UNIQUE_LINKS]))
	{
		return 0;
	}

	LinkData linkData;
	int actual;
	bool success = this->invlinklookup.get(linkID, (char*) &linkData, actual, sizeof(linkData));
	if(success)
	{
		link::api::LINKCOUNT_T* linkcountX= new link::api::LINKCOUNT_T[winCount()];
		for (int i=0; i<winCount();i++)
		{
			linkcountX[i] = linkData.totallinkCountX[i];
		}
		return linkcountX;
	}
	else
	{
		return NULL;
	}


}
/*link���ĵ�docID��,����X�³��ֵĴ��������link���ĵ�docID�У��򷵻�һЩ������link���ĵ�docID�еĴ��������򷵻�NULL.*/
lemur::api::LOC_T* link::api::KeyfileLinkIndex::linkCountX(link::api::LINKID_T linkID, lemur::api::DOCID_T docID) const
{
	if((linkID <= 0) || (linkID > this->linkcounts[UNIQUE_LINKS]))
	{
		return NULL;
	}

	lemur::api::DocInfoList* docList = InvlinkDocList(linkID);

	if( !docList ) 
	{
		throw lemur::api::Exception( "link::api::KeyfileLinkIndex::linkCountX", 
			"Failed to retrieve docInfoList" );
	}
	docList->startIteration();
	while (docList->hasMore())
	{/*һ�α�������invLinkDoclist,�Ժ�����skiplist(����һЩ����λ��ָ��)*/

		link::api::InvLinkDocInfo* linkDocInfo =dynamic_cast<link::api::InvLinkDocInfo*> (docList->nextEntry());

		if (linkDocInfo->docID()==docID)
		{
			
			lemur::api::LOC_T* linkcount = new lemur::api::LOC_T[winCount()+1];
			lemur::api::LOC_T* temp = const_cast<lemur::api::LOC_T*> (linkDocInfo->linkFreq());
			for (int i=0; i<=winCount();i++)
			{
				linkcount[i] = temp[i];
			}
			
			
			delete docList;
			docList = NULL;
			return linkcount;
		}
	}

	delete docList;
	docList = NULL;
	return NULL;
}
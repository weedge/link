
/**======================================
/*
 * NAME DATE - COMMENTS
 * wy 03/2011 - created
 *========================================================================*/

#include "LinkDocListSegmentReader.hpp"
#include "IndexTypes.hpp"
#include "LinkTypes.hpp"
#include <sstream>

link::file::LinkDocListSegmentReader::LinkDocListSegmentReader(int segment, std::string &baseName, 
														lemur::file::File* stream, int readBufferSize, const std::vector<int> &winsizes)
{

	assert(winsizes.size()>0);
	assert(winsizes.front()>0);
	if(winsizes.size()>MAXNUM_WIN)
	{
		std::cerr<<"the num of the winsizes is more than MAXNUM_WIN."<<std::endl;
		system("pause");
		exit(-1);
	}
	this->_winSizes.resize(winsizes.size());
	this->_winSizes = winsizes;

	this->_segment = segment;
	this->_stream = stream;
	
	std::stringstream strName;
	strName<<baseName<<_segment;
	this->_name = strName.str();

	this->_top = NULL;
	this->_stream->seekg(0,std::fstream::beg);
	this->_file = new lemur::file::ReadBuffer(*_stream,readBufferSize);

	
}

link::file::LinkDocListSegmentReader::~LinkDocListSegmentReader()
{
	delete _top;
	delete _file;
}

/** 
* 从*.pl文件的segment中读取一条记录，包括linkid, df, length, doclist... 
*/
link::api::InvLinkDocList* link::file::LinkDocListSegmentReader::next()
{
	if( (_file->rdstate() & std::fstream::eofbit) ) //文件末尾
	{
		// file is at end, no more streams left
		return 0;
	} 
	else 
	{
		// read enough to find out how long the thing is
		// alignment issues on solaris?
		//    int* header = (int*) _file->peek( sizeof(int) * 4 );
		const char* head = _file->peek( sizeof(int) * 4 );//读取前16Byte
		int header[4];
    
		//    if( !header ) {
		if( !head ) 
		{
			return 0;
		}

		memcpy(&header, head, sizeof(int) * 4 );

		int dataLength = header[3];//压缩数据长度
		// alignment issue on solaris.
		//    const char* listData = _file->read( (sizeof(int)*4) + dataLength );
		int *listData = new int[4 + (dataLength/sizeof(int)) + 1];//动态分配内存，应释放
		_file->read((char *)listData, (sizeof(int)*4) + dataLength );//从*.pl中读取一条记录到内存
		// make a list from the data we read
		// dmf FIXME
		link::api::InvLinkDocList* list = new link::api::InvLinkDocList( const_cast<lemur::api::LOC_T*>( (const lemur::api::LOC_T*) listData ), _winSizes );//动态分配内存
		//clean up alignment data.
		delete[](listData);//释放内存
		return list;
	}
}

/*
* 定义比较操作符，优先队列中用到。优先linkID，如果linkID相等，则考虑segment编号。
*/
bool link::file::LinkDocListSegmentReader::operator <(const link::file::LinkDocListSegmentReader &other) const
{
	const link::api::InvLinkDocList* otherTop = other.top();
	const link::api::InvLinkDocList* thisTop = top();

	// if neither object has data, the smaller segment
	// number is 'better'
	if( !thisTop && !otherTop )
		return ( getSegment() < other.getSegment() );

	// if we have data but the other object doesn't,
	// we are bigger (we should go first)
	if( !otherTop )
		return true;

	// if we don't have data but the other object does,
	// they are bigger (they should go first)
	if( !thisTop )
		return false;

	link::api::LINKID_T linkID = const_cast<link::api::InvLinkDocList*>(thisTop)->linkID();
	link::api::LINKID_T otherLinkID = const_cast<link::api::InvLinkDocList*>(otherTop)->linkID();

	// if our term ids are the same, the smaller segment is 'better'
	if( linkID == otherLinkID ) 
	{
		return ( getSegment() < other.getSegment() );
	}

	// if our term ids aren't the same, smaller termID is 'better'
	//  return termID > otherTermID;
	return linkID < otherLinkID;
}
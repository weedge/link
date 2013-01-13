/**======================================
/*
 * NAME DATE - COMMENTS
 * wy 03/2011 - created
 *========================================================================*/

#include "InvLinkInfoList.hpp"
#include "RVLCompress.hpp"
#include <assert.h>
#include <iostream>

/**
 *读取时，初始化。
*/
link::api::InvLinkInfoList::InvLinkInfoList()
{
	linkcount = 0;
	index = 0;
	linklistlen = 0;
	uid = -1;
	linklist = NULL;
	counts = NULL;
}


link::api::InvLinkInfoList::InvLinkInfoList(lemur::api::DOCID_T docid, int linkCount, std::vector<LinkedTerm> &lts)
{
	this->index = 0;
	this->linkcount = linkCount;
	this->uid = docid;
	this->counts = NULL;

	this->linklistlen = lts.size();
	this->linklist = new LinkedTerm[this->linklistlen];

	for(int i=0; i<linklistlen; i++)
		linklist[i] = lts[i];
}

link::api::InvLinkInfoList::~InvLinkInfoList()
{
	if (linklist != NULL)
		delete[](linklist);

	if (counts != NULL) 
	{
		delete[](counts);
		delete[](linklistcounted);
	}
}
/**
 * 向*.dl文件中写入经过压缩的数据，格式为：
 * |documentID|linkCount|compressed linklist length| compressed linklist|...
 *		|			|					|
 *	  4Byte		  4Byte				  4Byte
 */
void link::api::InvLinkInfoList::binWriteC(lemur::file::File &outfile)
{
	outfile.write((const char*) &uid, sizeof(lemur::api::DOCID_T));//写入文档ID，4字节
	outfile.write((const char*) &linkcount, sizeof(link::api::LINKCOUNT_T));//写入文档中link的数目，4字节
 
	if (linklistlen == 0) 
	{
		lemur::api::COUNT_T zero = 0;
		outfile.write((const char*) &zero, sizeof(lemur::api::COUNT_T));
		return;
	}
	
	deltaEncode();//将linklist中的linkid字段进行差值编码
	// compress it
	// it's ok to make comp the same size.  the compressed will be smaller
	unsigned char* comp = new unsigned char[linklistlen * sizeof(LinkedTerm)];//动态分配内存

	lemur::api::COUNT_T compbyte = lemur::utility::RVLCompress::compress_ints((int *)linklist, comp, 
																			linklistlen * 2);//将缓冲区中的数据压缩，注意该函数的用法
	outfile.write((const char*) &compbyte, sizeof(lemur::api::COUNT_T));//写入压缩后的linklist长度，4字节
	// write out the compressed bits
	outfile.write((const char*) comp, compbyte);//写入经过RVL算法压缩后的linklist

	delete[](comp);//释放内存

}

void link::api::InvLinkInfoList::deltaEncode()
{
	// we will encode in place
	// go backwards starting at the last linkid
	// we're counting on two always being bigger than one
	link::api::LINKID_T* two = (link::api::LINKID_T *)(linklist + linklistlen - 1);//结构体中字节对齐了。
	link::api::LINKID_T* one = two-2;

	//int i = 0;
	while (two != (link::api::LINKID_T *)linklist) 
	{
		*two = *two-*one;
		//if((*two)<0)std::cerr<<"有负数"<<i++<<std::endl;
		two = one;
		one -= 2;
	}
}

void link::api::InvLinkInfoList::deltaDecode()
{
	// we will decode in place
	// start at the begining
	link::api::LINKID_T* one = (link::api::LINKID_T *)linklist;
	link::api::LINKID_T* two = one+2;

	while (one != (link::api::LINKID_T*)(linklist + linklistlen - 1)) 
	{
		*two = *two + *one;
		one = two;
		two += 2;
	}//while
}

bool link::api::InvLinkInfoList::binReadC(lemur::file::File& infile)
{
	lemur::api::COUNT_T size = 0;

	infile.read((char*) &uid, sizeof(lemur::api::DOCID_T));//读取文档ID，4字节
	infile.read((char*) &linkcount, sizeof(link::api::LINKCOUNT_T));//读取文档总长度，4字节
	infile.read((char*) &size, sizeof(lemur::api::COUNT_T));//读取压缩后的termlist长度，4字节

	if (size == 0) 
	{
		linklist = new LinkedTerm[0];
		linkcount = 0;
		return true;
	}

	unsigned char *buffer = new unsigned char[size];//动态分配内存
	infile.read((char *)buffer, size);//读取经过RVL算法压缩后的termlist

	if (!(linklist == NULL))
		delete[](linklist);

	linklist = new LinkedTerm[size * 4];//动态分配内存
	// decompress it
	linklistlen = lemur::utility::RVLCompress::decompress_ints(buffer, (int *)linklist, size)/2;
	deltaDecode();//解密

	delete[](buffer);//释放内存
	return true;
}
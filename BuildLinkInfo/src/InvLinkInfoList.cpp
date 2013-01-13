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
 *��ȡʱ����ʼ����
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
 * ��*.dl�ļ���д�뾭��ѹ�������ݣ���ʽΪ��
 * |documentID|linkCount|compressed linklist length| compressed linklist|...
 *		|			|					|
 *	  4Byte		  4Byte				  4Byte
 */
void link::api::InvLinkInfoList::binWriteC(lemur::file::File &outfile)
{
	outfile.write((const char*) &uid, sizeof(lemur::api::DOCID_T));//д���ĵ�ID��4�ֽ�
	outfile.write((const char*) &linkcount, sizeof(link::api::LINKCOUNT_T));//д���ĵ���link����Ŀ��4�ֽ�
 
	if (linklistlen == 0) 
	{
		lemur::api::COUNT_T zero = 0;
		outfile.write((const char*) &zero, sizeof(lemur::api::COUNT_T));
		return;
	}
	
	deltaEncode();//��linklist�е�linkid�ֶν��в�ֵ����
	// compress it
	// it's ok to make comp the same size.  the compressed will be smaller
	unsigned char* comp = new unsigned char[linklistlen * sizeof(LinkedTerm)];//��̬�����ڴ�

	lemur::api::COUNT_T compbyte = lemur::utility::RVLCompress::compress_ints((int *)linklist, comp, 
																			linklistlen * 2);//���������е�����ѹ����ע��ú������÷�
	outfile.write((const char*) &compbyte, sizeof(lemur::api::COUNT_T));//д��ѹ�����linklist���ȣ�4�ֽ�
	// write out the compressed bits
	outfile.write((const char*) comp, compbyte);//д�뾭��RVL�㷨ѹ�����linklist

	delete[](comp);//�ͷ��ڴ�

}

void link::api::InvLinkInfoList::deltaEncode()
{
	// we will encode in place
	// go backwards starting at the last linkid
	// we're counting on two always being bigger than one
	link::api::LINKID_T* two = (link::api::LINKID_T *)(linklist + linklistlen - 1);//�ṹ�����ֽڶ����ˡ�
	link::api::LINKID_T* one = two-2;

	//int i = 0;
	while (two != (link::api::LINKID_T *)linklist) 
	{
		*two = *two-*one;
		//if((*two)<0)std::cerr<<"�и���"<<i++<<std::endl;
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

	infile.read((char*) &uid, sizeof(lemur::api::DOCID_T));//��ȡ�ĵ�ID��4�ֽ�
	infile.read((char*) &linkcount, sizeof(link::api::LINKCOUNT_T));//��ȡ�ĵ��ܳ��ȣ�4�ֽ�
	infile.read((char*) &size, sizeof(lemur::api::COUNT_T));//��ȡѹ�����termlist���ȣ�4�ֽ�

	if (size == 0) 
	{
		linklist = new LinkedTerm[0];
		linkcount = 0;
		return true;
	}

	unsigned char *buffer = new unsigned char[size];//��̬�����ڴ�
	infile.read((char *)buffer, size);//��ȡ����RVL�㷨ѹ�����termlist

	if (!(linklist == NULL))
		delete[](linklist);

	linklist = new LinkedTerm[size * 4];//��̬�����ڴ�
	// decompress it
	linklistlen = lemur::utility::RVLCompress::decompress_ints(buffer, (int *)linklist, size)/2;
	deltaDecode();//����

	delete[](buffer);//�ͷ��ڴ�
	return true;
}
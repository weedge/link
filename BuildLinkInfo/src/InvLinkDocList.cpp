/**======================================
/*
 * NAME DATE - COMMENTS
 * wy 03/2011 - created
 *========================================================================*/

#include "InvLinkDocList.hpp"
#include "LinkTypes.hpp"
#include "RVLCompress.hpp"
#include <assert.h>
#include <string>
#include <iostream>

/**
* ����invLinkDocListʱ����ʼinvLinkDocList.
*/
link::api::InvLinkDocList::InvLinkDocList(const link::api::LINKID_T &lid, 
										  const std::vector<int> &winsizes):lemur::index::InvDocList(0,0)

{
	this->winCount = winsizes.size();
	
	this->winSizes = new int[winCount+1];
	memset(winSizes,0,winCount+1);

	winSizes[0] = 0;
	for(int i=1;i<=winCount;i++) winSizes[i] = winsizes[i-1];

	ulid = lid;	

	//this->list_size = (int) pow(2.0,INITSIZE);//2��9�η���512
	//_begin = new lemur::api::LOC_T[list_size/sizeof(lemur::api::LOC_T)];
	//_lastid = _begin;
	//*_lastid = -1;
	//_end = _begin;
	//_freq = _begin;
	//df = 0;
	//LOC_Tsize = sizeof(lemur::api::LOC_T);
}

/**
* ��invLinkDocListʱ(���贰����Ϣʱ)�������ѹ��
*/
link::api::InvLinkDocList::InvLinkDocList(lemur::api::LOC_T *vec)
{
	READ_ONLY = false;
	hascache = false;
	LOC_Tsize = sizeof(lemur::api::LOC_T);
	uid = vec[0];
	df = vec[1];
	lemur::api::COUNT_T diff = vec[2];
	lemur::api::COUNT_T vecLength = vec[3];
	size = vecLength * 4;
	unsigned char* buffer = (unsigned char *) (vec + 4);
	// this should be big enough
	//  begin = (LOC_T*) malloc(s);
	// use new/delete[] so an exception will be thrown if out of memory.
	begin = new lemur::api::LOC_T[size/sizeof(lemur::api::LOC_T)];

	// decompress it
	int len = lemur::utility::RVLCompress::decompress_ints(buffer, (int *)begin, vecLength);

	lastid = begin + diff;
	end = begin + len;
	freq = lastid+1;

	deltaDecode();//��ѹ��
}
/**
* ��invLinkDocListʱ(��������)�������ѹ��
*/
link::api::InvLinkDocList::InvLinkDocList(lemur::api::LOC_T *vec,const std::vector<int> &winsizes)
{

	this->winCount = winsizes.size();
	
	this->winSizes = new int[winCount+1];
	memset(winSizes,0,winCount+1);

	winSizes[0] = 0;
	for(int i=1;i<=winCount;i++) winSizes[i] = winsizes[i-1];


	READ_ONLY = false;
	hascache = false;
	LOC_Tsize = sizeof(lemur::api::LOC_T);
	ulid = vec[0];
	df = vec[1];
	lemur::api::COUNT_T diff = vec[2];
	lemur::api::COUNT_T vecLength = vec[3];
	size = vecLength * 4;
	unsigned char* buffer = (unsigned char *) (vec + 4);
	// this should be big enough
	//  begin = (LOC_T*) malloc(s);
	// use new/delete[] so an exception will be thrown if out of memory.
	begin = new lemur::api::LOC_T[size/sizeof(lemur::api::LOC_T)];

	// decompress it
	int len = lemur::utility::RVLCompress::decompress_ints(buffer, (int *)begin, vecLength);

	lastid = begin + diff;
	end = begin + len;
	freq = lastid+1;

	deltaDecode();//��ѹ��

}

link::api::InvLinkDocList::~InvLinkDocList()
{
	
	if(winSizes !=NULL){
		delete[](winSizes);
		winSizes = NULL;
	}
	
}

/** 
 * ���link�ķ�ת�ĵ��б���ʽΪ��
 * |docid|lf-win0|lf-win1|,,,|linkFreq|...|docid|lf-win0|lf-win1|,,,|linkFreq|...|		|
 *	  |										|		 |							  |
 *	begin								  lastid	freq						 end
 */
bool link::api::InvLinkDocList::addFrequence(lemur::api::DOCID_T docid, link::api::DIST_T dist)
{ 
	if (READ_ONLY)
		return false;
  // check that we can add at all
	if (this->size == 0&&this->winCount>MAXNUM_WIN)
		return false;

  // check to see if it's a new document
	if (docid == *lastid) //�ĵ��Ѿ����ڣ�˵����link���ĵ��г��ֲ�ֻһ��
	{

//=====��Ҫ�öδ����ع���ά����̫��޸�MAXNUM_WIN����Ҫ�޸ģ���=================
		//if(dist<=winSizes[0]){//  |lf-win0|,,, 
		//	for(int i=0;i<winCount;i++){
		//		(*(freq+i))++;
		//	}
		//}
		//else if(dist<=winSizes[1]){//   |lf-win1|,,,
		//	for(int i=1;i<winCount;i++){
		//		(*(freq+i))++;
		//	}

		//}
		//else if(dist<=winSizes[2]){//   |lf-win2|,,, 
		//	for(int i=2;i<winCount;i++){
		//		(*(freq+i))++;
		//	}		
		//}
		//else if(dist<=winSizes[3]){//  |lf-win3|,,,
		//	for(int i=3;i<winCount;i++){
		//		(*(freq+i))++;
		//	}
		//}
		//else if(dist<=winSizes[4]){//   |lf-win4|, 
		//	for(int i=4;i<winCount;i++){
		//		(*(freq+i))++;
		//	}
		//}
//===========================================================================
		for(int i=0; i<winCount; i++){
			if(dist>winSizes[i] && dist<=winSizes[i+1]){//  |lf-win i|,,, 
				for(int j=i;j<winCount;j++){
					(*(freq+j))++;
				}
				break;
			}
		}

		(*(freq+winCount))++;//|linkFreq|
	} 
	else//�ĵ�δ���б���
	{
		//get more mem if needed
		if ((end-begin+winCount+2)*LOC_Tsize > this->size) 
		{
			if (!getMoreMem())
				return false;
		}

		lastid = end;
		*lastid = docid;
		freq = lastid+1;

		for(int i=0;i<winCount;i++){
			*(freq+i) = 0;
		}
		*(freq+winCount) = 1;//|linkFreq|

//=====��Ҫ�öδ����ع���ά����̫��޸�MAXNUM_WIN����Ҫ�޸ģ���=======================
		//if(dist<=winSizes[0]){//  |lf-win0|,,,
		//	for(int i=0;i<winCount;i++){
		//		*(freq+i) = 1;
		//	}
		//}
		//else if(dist<=winSizes[1]){//   |lf-win1|,,,
		//	for(int i=1;i<winCount;i++){
		//		*(freq+i) = 1;
		//	}
		//}
		//else if(dist<=winSizes[2]){//   |lf-win2|,,,
		//	for(int i=2;i<winCount;i++){
		//		*(freq+i) = 1;
		//	}		
		//}
		//else if(dist<=winSizes[3]){//  |lf-win3|,,, 
		//	for(int i=3;i<winCount;i++){
		//		*(freq+i) = 1;
		//	}
		//}
		//else if(dist<=winSizes[4]){//   |lf-win4|,  
		//	for(int i=4;i<winCount;i++){
		//		*(freq+i) = 1;
		//	}
		//}
//=========================================================================
		for(int i=0; i<winCount; i++){
			if(dist>winSizes[i] && dist<=winSizes[i+1]){//  |lf-win i|,,, 
				for(int j=i;j<winCount;j++){
					(*(freq+j))++;
				}
				break;
			}
		}

		end = freq+winCount+1;
		df++;
	}

	return true;
}

/**
* ����ԭ����С��2���ڴ�ռ䡣
*/
bool link::api::InvLinkDocList::getMoreMemory()
{
	int enddiff = end - begin;
	int ldiff = lastid - begin;
	int newsize = size*2;

	lemur::api::LOC_T* pervious = begin;
    //begin = (int*) malloc(newsize);
    // use new/delete[] so an exception will be thrown if out of memory
    begin = new lemur::api::LOC_T[newsize/sizeof(lemur::api::LOC_T)];
    memcpy(begin, pervious, size);
    //free(pervious);
    delete[](pervious);
	pervious = NULL;

	
	lastid = begin + ldiff;
	freq = lastid + 1;
	end = begin +enddiff;
	size = newsize;

	return true;
}


/** ʹ��RVL�㷨��inverted linkdocument list ����ѹ�� */
lemur::api::LOC_T* link::api::InvLinkDocList::compInvLinkDocList(lemur::api::COUNT_T &vecLength)
{
	int len = end - begin;
	int diff = lastid - begin;
	deltaEncode();

	unsigned char* data = new unsigned char[(len+4)*sizeof(lemur::api::LOC_T)];

	vecLength = lemur::utility::RVLCompress::compress_ints((int *)begin, data+4*LOC_Tsize, len);

	lemur::api::LOC_T* temp = (lemur::api::LOC_T*)data;
	temp[0] = ulid;//__64int to  int  possible loss of data. how to solve this bug???
	temp[1] = df;
	temp[2] = diff;
	temp[3] = vecLength;
	vecLength = vecLength + 4*LOC_Tsize;

	return temp;
}

/**
* дinvlinkDoclistʱ��������в�ֵ���롣
*/
void link::api::InvLinkDocList::deltaEncode()
{
	lemur::api::LOC_T* doctwo = lastid;
	lemur::api::LOC_T* frebeg = freq;
	lemur::api::LOC_T* fretwo = end-1;
	lemur::api::LOC_T* freone = end-2;

	std::vector<lemur::api::LOC_T*> trackdocone;
	
	lemur::api::LOC_T* k = begin;
	while(k != lastid){
		lemur::api::LOC_T* j = k;
		trackdocone.push_back(j);
		k = k + (winCount+1) +1;
	}
	lemur::api::LOC_T* docone;
	while(doctwo != begin){
		docone = trackdocone.back();
		trackdocone.pop_back();

		while(fretwo != frebeg){
			*fretwo = *fretwo - *freone;
			fretwo = freone;
			freone--;
		}

		fretwo = doctwo-1;
		frebeg = docone+1;
		freone = doctwo-2;

		*doctwo = *doctwo - *docone;
		doctwo = docone;

	}

	/*�����һ���ĵ��е�freq*/
	while(fretwo != frebeg){
		*fretwo = *fretwo - *freone;
		fretwo = freone;
		freone--;
	}
	
}
/**
* ��invlinkDoclistʱ�������ֵ���롣
*/
void link::api::InvLinkDocList::deltaDecode()
{
	lemur::api::LOC_T* docone = begin;
	lemur::api::LOC_T* doctwo = begin + (winCount+1) + 1;
	
	lemur::api::LOC_T* freone = begin+1;
	lemur::api::LOC_T* fretwo = begin+2;

	while(docone != lastid)
	{
		while(fretwo != doctwo){
			*fretwo = *fretwo + *freone;
			freone = fretwo;
			fretwo++;
		}

		*doctwo = *doctwo + *docone;
		docone = doctwo;
		doctwo = docone + (winCount+1) +1;

		freone = docone+1;
		fretwo = docone+2;
	}

	/*do freqs for the last one */
	while(fretwo != doctwo){
		*fretwo = *fretwo + *freone;
		freone = fretwo;
		fretwo++;
	}
}

/**
* link�ڼ����г��ֵĴ���,����������ˣ���Ҫ����֮����ܵõ�����
*/
link::api::LINKCOUNT_T link::api::InvLinkDocList::linkCF() const
{
	link::api::LINKCOUNT_T linkcf = 0;
	lemur::api::LOC_T* ptr = begin;

	while(ptr !=lastid){
		linkcf += *(ptr+winCount+1);
		ptr = ptr + winCount + 2;
	}

	//linkcf += *(ptr+winCount+1);
	linkcf += *(freq+winCount);

	return linkcf;
}

/**
* link�ڴ���winSize�£������г��ֵĴ���,����������ˣ���Ҫ����֮����ܵõ�������winSizeû���ڸ������ڴ�С������£���������0.
*/
link::api::LINKCOUNT_T link::api::InvLinkDocList::linkCF(int winSize) const
{
	bool find = false;
	int i = 0;
	while(i<winCount){
		if(winSize == winSizes[i+1]){
			find = true;
			break;
		}
		i++;
	}

	link::api::LINKCOUNT_T linkcfX = 0;
	lemur::api::LOC_T* ptr = begin;

	if(find){
		while(ptr != lastid){
			linkcfX += *(ptr+i+1);
			ptr = ptr+winCount+2;
		}
		
		//linkcfX += *(ptr+i+1);
		linkcfX += *(freq+i);
		return linkcfX;
	}else{
		std::cerr<<"the winSize is not found in the winSizes. "<<std::endl;
		return 0;
	}

}

/**
* ��ԭ�е�doclistβ����µ�doclist.
*/
bool link::api::InvLinkDocList::append(lemur::index::InvDocList *part_tail)
{
	if(this->READ_ONLY)
		return false;

	// through subclass object access the protected member of the parent-class.
	link::api::InvLinkDocList* tail = (link::api::InvLinkDocList*) part_tail; 

	// we only want to append the actual content
	lemur::api::LOC_T* ptr = tail->begin;
	lemur::api::COUNT_T len = tail->length();

	// check for memory
	while ((end-begin+len)*LOC_Tsize > size) 
	{
		if (!getMoreMem())
			return false;
	}//while

	// update doc frequency
	df += tail->docFreq();
  
	// check for overlap (by 1 docid)
	// this method will mainly be used for merging lists from indexing
	// in that case, overlap of docids would only occur by 1
	if (*ptr == *lastid) 
	{
		// add linkfreqs together
		for(int i=0; i<=winCount; i++){
			*(freq+i) += *(ptr+1+i);
		}

		// doc frequency is actually one less
		df--;

		// advance pointer to next doc
		ptr =ptr + (winCount+2);
		len =len - (winCount+2);
	}

	// copy list over
	if (len > 0) 
	{
		memcpy(end, ptr, len*LOC_Tsize);

		end += len;
		lastid = end-(winCount+2);
		freq = end-(winCount+1);
	}

  	return true;
}

lemur::api::DocInfo* link::api::InvLinkDocList::nextEntry() const
{
	// info is stored in LOC_T as docid and LINKCOUNT_T* as lfwin0,,,lfwinX,linkFreq.
	entry.docID(*iter);
	iter++;
	entry.linkFreq(iter);
	iter += this->winCount+1 ;
	return &entry;
}
#include "LinkInfo.hpp"
#include "Index.hpp"
#include "Param.hpp"
#include "InvFPTermList.hpp"
#include <algorithm>

using namespace lemur::api;
using namespace std;
/**
 * ���캯��������link���ĵ��еĳ��ֵĴ������������Ϣ
 */
LinkInfo::LinkInfo(const int &left, const int &right, const int &dID, const Index &dbIndex) : leftID(left), rightID(right), docID(dID), ind(dbIndex)
{	
	lemur::index::InvFPTermList *tl = (lemur::index::InvFPTermList *)ind.termInfoList(docID);//��ȡ�ĵ��е�term�б�ʹ�ú��ͷ��ڴ�

	while(tl->hasMore())//�����ĵ��е�term�б�term�Ƿ�����ظ����������ظ���Ҳ���迼��
	{
		lemur::index::InvFPTerm *t = (lemur::index::InvFPTerm *)tl->nextEntry();
		
		const vector<lemur::api::LOC_T> *positions = t->getPositions();//��ȡterm���ĵ��е�λ���б��Ƿ�Ӧ��ɾ��ָ�룿����Ҫ��ֻҪ�ͷ�InvFPTermList����
		
		termPositions[t->termID()] = positions;
	}//while
	
	for(vector<lemur::api::LOC_T>::const_iterator leftIt = termPositions[leftID]->begin(); leftIt != termPositions[leftID]->end(); ++leftIt)
	{
		for(vector<lemur::api::LOC_T>::const_iterator rightIt = termPositions[rightID]->begin(); rightIt != termPositions[rightID]->end(); ++rightIt)
		{
			int d = abs(*leftIt - *rightIt);
			distance.push_back(d);
		}//for
	}//for
	
	count = distance.size();
	sort(distance.begin(), distance.end());//����

	if(lemur::api::ParamGetInt("debug", 0) == 2)
	{
		std::cout << "============================================" << endl;
		std::cout << "leftID : " << leftID << "<" << ind.term(leftID) << ">" << endl;
		std::cout << "rightID : " << rightID << "<" << ind.term(leftID) << ">" << endl;
		std::cout << "docID : " << docID << "<" << ind.document(docID)<< ">" << endl;
		std::cout << "total count in document : " << count << endl;
		std::cout << "distance :" ;
		for(vector<int>::iterator it = distance.begin(); it != distance.end(); ++it)
		{
			std::cout << " " << *it ;
		}//for
		std::cout << endl << "============================================" << endl;
	}//if

	delete tl;
	tl = NULL;
}//end LinkInfo

/**
 * �����������ͷ��ڴ�
 */
LinkInfo::~LinkInfo()
{
	//for(map<int, const vector<lemur::api::LOC_T> *>::iterator it = termPositions.begin(); it != termPositions.end(); ++it)
	//{
	//	free(it->second);
	//}//for
	//termPositions.clear();
}//end ~LinkInfo

/**
 * ��ȡlink��ָ�����ڴ�С�³��ֵĴ���
 */
int LinkInfo::getCount(const int &windowSize) const
{
	if(distance.back() <= windowSize)//link�ľ��붼�ڴ�����
	{
		return distance.size();
	}
	else
	{
		vector<int>::const_iterator upper = upper_bound(distance.begin(), distance.end(), windowSize);//��ȡ������ڴ���ֵ�ĵ�һ��link��λ��
		int num = 0;
		for(vector<int>::const_iterator it = distance.begin(); it != upper; ++it)
		{
			++num;
		}

		return num;
	}
}//end getCount

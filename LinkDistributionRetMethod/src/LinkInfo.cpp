#include "LinkInfo.hpp"
#include "Index.hpp"
#include "Param.hpp"
#include "InvFPTermList.hpp"
#include <algorithm>

using namespace lemur::api;
using namespace std;
/**
 * 构造函数，计算link在文档中的出现的次数，距离等信息
 */
LinkInfo::LinkInfo(const int &left, const int &right, const int &dID, const Index &dbIndex) : leftID(left), rightID(right), docID(dID), ind(dbIndex)
{	
	lemur::index::InvFPTermList *tl = (lemur::index::InvFPTermList *)ind.termInfoList(docID);//获取文档中的term列表，使用后释放内存

	while(tl->hasMore())//遍历文档中的term列表，term是否存在重复？若存在重复，也无需考虑
	{
		lemur::index::InvFPTerm *t = (lemur::index::InvFPTerm *)tl->nextEntry();
		
		const vector<lemur::api::LOC_T> *positions = t->getPositions();//获取term在文档中的位置列表，是否应该删除指针？不需要，只要释放InvFPTermList就行
		
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
	sort(distance.begin(), distance.end());//排序

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
 * 析构函数，释放内存
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
 * 获取link在指定窗口大小下出现的次数
 */
int LinkInfo::getCount(const int &windowSize) const
{
	if(distance.back() <= windowSize)//link的距离都在窗口内
	{
		return distance.size();
	}
	else
	{
		vector<int>::const_iterator upper = upper_bound(distance.begin(), distance.end(), windowSize);//获取距离大于窗口值的第一个link的位置
		int num = 0;
		for(vector<int>::const_iterator it = distance.begin(); it != upper; ++it)
		{
			++num;
		}

		return num;
	}
}//end getCount

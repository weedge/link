#include "LinkQueryRep.hpp"
#include "Link.hpp"
#include "LinkInfo.hpp"
#include "Param.hpp"
#include "InvFPTermList.hpp"

using namespace lemur::api;

LinkQueryRep::LinkQueryRep(const LinkQuery &qry, const Index &dbIndex) : ind(dbIndex), iter(0)
{	
	qry.startIteration();
	while(qry.hasMore())
	{
		Link *link = qry.nextLink();
		
		LinkInfoList *liList = initLinkInfo(link->getLeftID(), link->getRightID());//无需释放内存
		if(liList != NULL)
		{
			linkInfoListVec.push_back(liList);
		}
	}//while
	
}//end LinkQueryRep

/**
 * 析构函数，释放内存
 */
LinkQueryRep::~LinkQueryRep()
{
	if(lemur::api::ParamGetInt("debug", 0))
	{
		std::cout << "~LinkQueryRep 析构函数被调用，释放 LinkInfoList 对象所占内存" << endl;
	}

	while(!linkInfoListVec.empty())
	{
		delete linkInfoListVec.back();
		linkInfoListVec.back() = 0;
		linkInfoListVec.pop_back();
	}//while

}//end ~LinkQueryRep


/**
 * 设置link的详细信息
 */
LinkInfoList *LinkQueryRep::initLinkInfo(const int &leftID, const int &rightID)
{
	DocInfoList *leftDocList = ind.docInfoList(leftID);//使用后需要删除指针
	DocInfoList *rightDocList = ind.docInfoList(rightID);

	leftDocList->startIteration();
	rightDocList->startIteration();

	DocInfo *leftDocInfo;//不需要删除的指针
	DocInfo *rightDocInfo;

	if(leftDocList->hasMore() && rightDocList->hasMore())
	{
		leftDocInfo = leftDocList->nextEntry();
		rightDocInfo = rightDocList->nextEntry();
	}
	else
	{
		return NULL;
	}

	bool end = false;
	vector<int> docIDVec;

	/** 获取包含link的文档列表 */
	while(!end)
	{
		if(leftDocInfo->docID() == rightDocInfo->docID())
		{
			docIDVec.push_back(leftDocInfo->docID());

			if(leftDocList->hasMore() && rightDocList->hasMore())
			{
				leftDocInfo = leftDocList->nextEntry();
				rightDocInfo = rightDocList->nextEntry();
			}
			else
			{
				end = true;
			}
		}
		else if(leftDocInfo->docID() > rightDocInfo->docID())
		{
			if(rightDocList->hasMore())
			{
				rightDocInfo = rightDocList->nextEntry();
			}
			else
			{
				end = true;
			}
		}
		else
		{
			if(leftDocList->hasMore())
			{
				leftDocInfo = leftDocList->nextEntry();
			}
			else
			{
				end = true;
			}
		}
	}//while

	delete leftDocList;
	leftDocList = NULL;

	delete rightDocList;
	rightDocList = NULL;


	if(docIDVec.empty())//link没有在任何文档中出现，无效link
	{
		return NULL;
	}

	LinkInfoList *liList = new LinkInfoList();
	
	if(lemur::api::ParamGetInt("debug", 0) == 2)
	{
		std::cout << "link < " << leftID << "," << rightID << " > appears in documents : " << endl;
		for(vector<int>::iterator it = docIDVec.begin(); it != docIDVec.end(); ++it)
		{
			std::cout << *it << "<" << ind.document(*it) <<  "> ";
		}//for
		std::cout << endl << "Total documents : " << docIDVec.size() << endl;
	}//if

	/** 遍历包含link的文档列表 */
	for(vector<int>::iterator it = docIDVec.begin(); it != docIDVec.end(); ++it)
	{	
		LinkInfo *linkInfo = new LinkInfo(leftID, rightID, *it, ind);//生成link在文档中出现的次数，距离等信息，内存由LinkInfoList释放
		liList->addLinkInfo(linkInfo);
	}//for

	return liList;

}//end initLinkInfo

/**
 * 获取指向LinkInfoList对象的指针
 */
LinkInfoList *LinkQueryRep::nextEntry() const
{
	return linkInfoListVec[iter++];
}//end nextEntry
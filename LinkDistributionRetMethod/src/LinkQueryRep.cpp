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
		
		LinkInfoList *liList = initLinkInfo(link->getLeftID(), link->getRightID());//�����ͷ��ڴ�
		if(liList != NULL)
		{
			linkInfoListVec.push_back(liList);
		}
	}//while
	
}//end LinkQueryRep

/**
 * �����������ͷ��ڴ�
 */
LinkQueryRep::~LinkQueryRep()
{
	if(lemur::api::ParamGetInt("debug", 0))
	{
		std::cout << "~LinkQueryRep �������������ã��ͷ� LinkInfoList ������ռ�ڴ�" << endl;
	}

	while(!linkInfoListVec.empty())
	{
		delete linkInfoListVec.back();
		linkInfoListVec.back() = 0;
		linkInfoListVec.pop_back();
	}//while

}//end ~LinkQueryRep


/**
 * ����link����ϸ��Ϣ
 */
LinkInfoList *LinkQueryRep::initLinkInfo(const int &leftID, const int &rightID)
{
	DocInfoList *leftDocList = ind.docInfoList(leftID);//ʹ�ú���Ҫɾ��ָ��
	DocInfoList *rightDocList = ind.docInfoList(rightID);

	leftDocList->startIteration();
	rightDocList->startIteration();

	DocInfo *leftDocInfo;//����Ҫɾ����ָ��
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

	/** ��ȡ����link���ĵ��б� */
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


	if(docIDVec.empty())//linkû�����κ��ĵ��г��֣���Чlink
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

	/** ��������link���ĵ��б� */
	for(vector<int>::iterator it = docIDVec.begin(); it != docIDVec.end(); ++it)
	{	
		LinkInfo *linkInfo = new LinkInfo(leftID, rightID, *it, ind);//����link���ĵ��г��ֵĴ������������Ϣ���ڴ���LinkInfoList�ͷ�
		liList->addLinkInfo(linkInfo);
	}//for

	return liList;

}//end initLinkInfo

/**
 * ��ȡָ��LinkInfoList�����ָ��
 */
LinkInfoList *LinkQueryRep::nextEntry() const
{
	return linkInfoListVec[iter++];
}//end nextEntry
#ifndef _LINK_INFO_HPP
#define _LINK_INFO_HPP
#include <vector>
#include <map>

#include "Index.hpp"

/**
 * ����link���ĵ��г��ֵĴ������������Ϣ
 */
class LinkInfo
{
public:
	LinkInfo(const int &left, const int &right, const int &dID, const lemur::api::Index &dbIndex);
	virtual ~LinkInfo();

	virtual int getDocID() const {return docID;}
	virtual int getLeftID() const {return leftID;}
	virtual int geRrightID() const {return rightID;}
	/** ��ȡlink�ڴ���ֵΪ�����ʱ���ֵĴ��� */
	virtual int getCount() const{return count;}

	/** ��ȡlink��ָ�����ڴ�С�³��ֵĴ��� */
	virtual int getCount(const int &windowSize) const;
protected:
	int docID;//�ĵ�ID
	int count;//link���ĵ��г��ֵĴ����������Ǵ��ڴ�С��������Ϊ�����
	int leftID;
	int rightID;
	std::vector<int> distance;//link��term֮��ľ���
	const lemur::api::Index &ind;
	std::map<int, const std::vector<lemur::api::LOC_T> *> termPositions;//�ĵ���ÿ��term���ֵ�λ���б�
};

#endif

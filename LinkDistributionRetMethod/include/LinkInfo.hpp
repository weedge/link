#ifndef _LINK_INFO_HPP
#define _LINK_INFO_HPP
#include <vector>
#include <map>

#include "Index.hpp"

/**
 * 描述link在文档中出现的次数，距离等信息
 */
class LinkInfo
{
public:
	LinkInfo(const int &left, const int &right, const int &dID, const lemur::api::Index &dbIndex);
	virtual ~LinkInfo();

	virtual int getDocID() const {return docID;}
	virtual int getLeftID() const {return leftID;}
	virtual int geRrightID() const {return rightID;}
	/** 获取link在窗口值为无穷大时出现的次数 */
	virtual int getCount() const{return count;}

	/** 获取link在指定窗口大小下出现的次数 */
	virtual int getCount(const int &windowSize) const;
protected:
	int docID;//文档ID
	int count;//link在文档中出现的次数，不考虑窗口大小，即窗口为无穷大
	int leftID;
	int rightID;
	std::vector<int> distance;//link中term之间的距离
	const lemur::api::Index &ind;
	std::map<int, const std::vector<lemur::api::LOC_T> *> termPositions;//文档中每个term出现的位置列表
};

#endif

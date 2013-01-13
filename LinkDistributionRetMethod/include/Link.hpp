#ifndef _LINK_HPP
#define _LINK_HPP

class Link
{
public:
	Link(){}
	Link(const int &left, const int &right) : leftID(left), rightID(right) {}
	const int getLeftID() const {return leftID;}
	const int getRightID() const {return rightID;}
	void setLeftID(const int &left) {leftID = left;}
	void setRightID(const int &right) {rightID = right;}
	virtual ~Link(){}

protected:
	int leftID;
	int rightID;
};

#endif

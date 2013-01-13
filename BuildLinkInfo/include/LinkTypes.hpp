
/*===========================================
 NAME	DATE			CONMENTS
 wy		02/2011			created

 =================================================
 *
 */
#ifndef LINK_TYPES_HPP
#define LINK_TYPES_HPP

#include <string>
#include <vector>

namespace link{
	namespace api{

#define MAXNUM_WIN 10 //最多开10个窗口

		typedef std::string LINK_STR;

		typedef	__int64 LINKID_T;
		typedef unsigned int LINKCOUNT_T;
		typedef unsigned int DIST_T;


		struct LinkedTerm { // linkedterm ID and its distance
			link::api::LINKID_T linkID;
			link::api::DIST_T dist;
		};

		struct LDTerm { // linkedterm ID and the list of its distance
			link::api::LINKID_T linkID;
			std::vector<link::api::DIST_T> dists;
		};



	}
}

#endif //LINK_TYPE_HPP
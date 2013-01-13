/**======================================
/*
 * NAME DATE - COMMENTS
 * wy 02/2011 - created 
 *========================================================================*/

#include "LinkCache.hpp"
#include <iostream>


link::utility::LinkCache::LinkCache()
{
	clear();
}

bool link::utility::LinkCache::add(const char *linkName, link::api::LINKID_T LinkID)
{
	  // don't cache links longer than LINKCACHE_MAX_LINK_LENGTH
	if (linkName && (strlen(linkName) > LINKCACHE_MAX_LINK_LENGTH)) {
		return false;
	}
	else
	{
		int bucket = _hashFunction( linkName );

		if( bucket >= 0 ) 
		{
			if( _linkCache[bucket].linkID == -1 ) 
			{
				strncpy( _linkCache[bucket].link_str, linkName, sizeof _linkCache[bucket].link_str );
				_linkCache[bucket].linkID = LinkID;
				return true;
			}else{
				return false;
			}
		}
	}
}

void link::utility::LinkCache::clear()
{
	memset( this->_linkCache, 0xff, sizeof(this->_linkCache) );
}

/**
* 哈希函数，优化，减少冲突。
*/
link::api::LINKID_T link::utility::LinkCache::_hashFunction(const char *linkName) const
{
	int hash = 0;
	int index;

	for( index = 0; linkName[index]; index++ )
		hash = 7*hash + linkName[index];

	if( index <= sizeof( _linkCache[0].link_str ) ){
		return hash % ( LINKCACHE_SIZE / sizeof( struct cache_entry ) );
	}
	return -1;
}

link::api::LINKID_T link::utility::LinkCache::find(const char *linkName) const
{
	int bucket = _hashFunction( linkName );

	if( _linkCache[bucket].linkID != -1 ) 
	{
		if( !strncmp( linkName, _linkCache[bucket].link_str, sizeof _linkCache[bucket].link_str ) ) 
		{
			return _linkCache[bucket].linkID;
		}
	}

	return -1;
}


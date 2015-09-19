/*************************************************************************
    @ File Name: types_def.h
    @ Method: various types used in the whole projects
    @ Author: Jerry Shi
    @ Mail: jerryshi0110@gmail.com 
    @ Created Time: 2015年09月19日 星期六 15时00分25秒
 ************************************************************************/
#ifndef TYPES_DEF_H
#define TYPES_DEF_H

#include "common.h"

typedef pair<std::string,float> PAIR;

/**struct**/
struct DATA_TYPE
{
	float score;
	std::string txt;
};

struct QueryData
{
	std::string text; // query txt
	std::size_t hits; // search times
	std::size_t counts; // results number

	vector<std::size_t> tid; // terms hash value vector
};

struct biggerthan
{
	bool operator()(const PAIR& lhs,const PAIR& rhs)
	{
		return lhs.second > rhs.second;
	}
};

struct cmpByValue
{
	bool operator()(const DATA_TYPE& lhs,DATA_TYPE& rhs)
	{
		return lhs.score > rhs.score;
	}
};

/**types define**/
typedef boost::unordered_map<std::string,std::size_t> StrToIntMap;
typedef boost::unordered_map<std::string,std::size_t>::iterator StrToIntMapIter;

/**id to query id list **/
typedef boost::unordered_map<std::size_t,vector<std::size_t> > IdToQList;
typedef boost::unordered_map<std::size_t,vector<std::size_t> >::iterator IdToQListIter;

/**id to query content**/
typedef boost::unordered_map<std::size_t,QueryData> IntToStruct;
typedef boost::unordered_map<std::size_t,QueryData>::IntToStructIter;

/****/
typedef boost::unordered_map<std::size_t,vector<std::string> > IntToStrList;
typedef boost::unordered_map<std::size_t,vector<std::string> >::IntToStrListIter;





#endif //types_def.h

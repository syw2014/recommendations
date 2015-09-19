/*************************************************************************
    @ File Name: common.h
    @ Method: public headers and functions
    @ Author: Jerry Shi
    @ Mail: jerryshi0110@gmail.com 
    @ Created Time: 2015年09月19日 星期六 15时05分04秒
 ************************************************************************/
#ifndef COMMON_H
#define COMMON_H

/**Standard C++ headers**/
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <list>
#include <set>

/**Boost headers**/
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/unordered_map.hpp>
#include <boost/functional/hash.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


/**Standard C headers**/
#include <ctype.h>
#include <math.h>
#include <time.h>

/**Other headers**/
#include <util/hashFunction.h>
#include <glog/logging.h>

#include "knlp/horse_tokenize.h"
#include "normalize.h"
#include "json/json.h"


/*************************************************************************
    @ File Name: recommendEngine.h
    @ Method:
    @ Author: Jerry Shi
    @ Mail: jerryshi0110@gmail.com 
    @ Created Time: 2015年07月17日 星期五 15时52分32秒
 ************************************************************************/
#ifndef RECOMMENDENGINE_H
#define RECOMMENDENGINE_H

#include <iostream>
#include <map>

#include "indexEngine.h"
#include "normalize.h"
#include "time.h"
#include "math.h"

class recommendEngine
{
	public:
		recommendEngine(const std::string& token_dir,const std::string& dict_pth);
		~recommendEngine();

	public:
			void getCandicate(const std::string& userQuery
					,queryProperty& qProperty
					,StrToIntMap& termsIdMap); //get candicate

			void recommendNoResults(std::string inputQuery
					,queryProperty& qProperty
					,Json::Value& jsonResult
					,StrToIntMap& termsIdMap); //

			void recommendCorrection();

			void recommend(std::string inputQuery
					,queryProperty& qProperty
					,Json::Value& jsonResult
					,StrToIntMap& termsIdMap
					,const std::size_t TopK = 9);

			void InsertRsKeywords(std::string& KeyAndRskey);
			bool isNeedBuild();
			bool isNeedAdd();
			void jsonResults(const std::string& userQuery,std::string& res);
			void buildEngine();

	private:
			std::string token_dir_;
			std::string workdir_;
			std::size_t timestamp_;

			indexEngine* indexer_;

			bool isNeedIndex;

			void clear();
};



#endif //recommendEngine.h

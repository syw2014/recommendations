/*************************************************************************
    @ File Name: recommendEngine.cpp
    @ Method: recommend engine,query recommendation,correction,etc.
    @ Author: Jerry Shi
    @ Mail: jerryshi0110@gmail.com 
    @ Created Time: 2015年07月17日 星期五 15时52分57秒
 ************************************************************************/
#include "recommendEngine.h"

bool Is_subset(vector<std::size_t> v1,vector<std::size_t> v2);
void caculateNM(vector<std::size_t> v1,vector<std::size_t> v2,std::size_t& cnt);
void sortByScore(vector<DATA_TYPE>& queryScoreMap,DATA_TYPE& dt,const std::size_t& TopK=9);

static std::string timestamp = ".RecommendEngineTimestamp";

//construct,and initialize
recommendEngine::recommendEngine(const std::string& token_dir,const std::string& workdir)
	:token_dir_(token_dir)
	,workdir_(workdir)
	,timestamp_(0)
	,indexer_(NULL)
	,isNeedIndex(false)
{
	//initial dictionary and resoure directory
	std::string dict = workdir_ + "dict";
	if(!boost::filesystem::exists(dict + "/dict/"))
	{
		boost::filesystem::create_directory(dict + "/dict/");
	}

	indexer_ = new indexEngine(token_dir_,dict + "/dict/"); //set token path and dictionary path
	isNeedIndex = indexer_->open();//load dictionary to memory
	//get timestamp
	std::string resource = workdir_;
    resource += "/";
	resource += "resource";
	if(!boost::filesystem::exists(resource))
	{
		LOG(ERROR) << "No original query sources directory!";
	}
	else
	{
		resource += timestamp;
		std::ifstream in;
		in.open(resource.c_str(),std::ifstream::in);
		if(in)
			in >> timestamp_;
		in.close();
	}
}

recommendEngine::~recommendEngine()
{
	if(NULL != indexer_)
	{
		delete indexer_;
		indexer_ = NULL;
	}
}

void recommendEngine::getCandicate(const std::string& userQuery
					,Terms2QidMap& terms2qIDs
					,QueryIdataMap& queryIdata
					,QueryCateMap& query2Cate
					,QueryCateMap& rsKeywords
					,String2IntMap& termsIdMap)
{
	//buildEngine();
//	if(0 == userQuery.length())
//		return;
   termsIdMap = indexer_->search(userQuery,terms2qIDs
		   ,queryIdata,query2Cate,rsKeywords);
}

//no results recommendation
void recommendEngine::recommendNoResults(Terms2QidMap& terms2qIDs,
		QueryIdataMap& queryIdata
        ,QueryCateMap& query2Cate
		,QueryCateMap& rsKeywords
		,Json::Value& jsonResult
        ,String2IntMap& termsIdMap
		,std::string inputQuery)
{
	//no candicate or no terms
//	if(0 == terms2qIDs.size() || 0 == queryIdata.size() || 0 == termsIdMap.size())
//		return;
	vector<std::size_t> qTermsID;
	vector<std::size_t> termsID;
	String2IntMapIter termsIter;
	vector<DATA_TYPE> queryScoreVector;
    vector<DATA_TYPE> vec;

    vec.clear();
	qTermsID.clear();
	termsID.clear();
	queryScoreVector.clear();

	//find biggest score
	Terms2QidMapIter termsIdIter;

	bool subset = false;
	float bigScore1 = 0.0;
	float bigScore2 = 0.0;

    std::size_t cnt = 0;

	std::string res1 = "";
	std::string res2 = "";
	std::string big_term = "";
    std::string queryText = "";
    DATA_TYPE dt;
    dt.score = 0.0;
    dt.txt = "";

	for(termsIter = termsIdMap.begin(); termsIter != termsIdMap.end(); ++termsIter)
	{
		if(big_term.length() < termsIter->first.length())
			big_term = termsIter->first;
		termsID.push_back(termsIter->second);
	}

	for(termsIdIter = terms2qIDs.begin(); termsIdIter != terms2qIDs.end(); ++termsIdIter)
	{
		for(std::size_t i = 0; i < termsIdIter->second.size(); ++i)
		{
            dt.txt = queryIdata[termsIdIter->second[i]].text;
            if(dt.txt.size() > 30)
                continue;
            //LOG(INFO) << "candicate query length:" << queryText.size();
			qTermsID = queryIdata[termsIdIter->second[i]].tid;
			//caculateNM(termsID,qTermsID,cnt);
			float weight = (float) queryIdata[termsIdIter->second[i]].counts / (
			queryIdata[termsIdIter->second[i]].hits + (float)0.3*queryIdata[termsIdIter->second[i]].counts);
			if(Is_subset(termsID,qTermsID))
			{
				subset = true;

				float score = (float) weight * qTermsID.size();
				DATA_TYPE sq = {score,dt.txt};
				if(bigScore1 < score)
				{
					bigScore1 = score;  //score
					res1 = dt.txt;
					vec.push_back(sq);
                    
				}
			}
			else
			{
				float tscore = (float) weight * qTermsID.size();
				if(bigScore2 < tscore)
				{
					bigScore2 = tscore;
					res2 = queryIdata[termsIdIter->second[i]].text; //get query 
				}
			}
			if(rsKeywords.size() == 0)
			{
			//find related
			if(inputQuery.size() < 31 )
            {
			caculateNM(termsID,qTermsID,cnt);
            float simScore = (float) cnt / (qTermsID.size() + 0.1);
            float wScore = (float)log(queryIdata[termsIdIter->second[i]].hits 
					+ 2.0) / (qTermsID.size() + 0.1);
            dt.score = (float) wScore * simScore;
            sortByScore(queryScoreVector,dt);
            }
			}
		}
	}
	
	//get the most similarity query
	std::string ss = "";
	float b_score = 0.0;
	if(subset)
	{
		ss = res1;
		b_score = bigScore1;
	}
	else
	{
		ss = res2;
		b_score = bigScore2;
	}

	//check the suggestion
	if(ss.length() <= 3 && b_score !=0)
		ss = big_term;
	if(inputQuery == ss || ss == "")
		ss = "null";
    
	jsonResult["NoResult_Recommend"] = ss;

	//get TopK
	std::size_t Topk = 9;
	std::size_t upperbound;
	Json::Value recommend;
	if(rsKeywords.size() != 0)
	{
		QueryCateMapIter rsIter;
		queryScoreVector.clear();
		for(rsIter = rsKeywords.begin(); rsIter != rsKeywords.end(); ++rsIter)
			for(int i = 0; i < rsIter->second.size();++i)
			recommend.append(rsIter->second[i]);
	}
	else
	if(inputQuery.size() > 31)
		queryScoreVector = vec;

	if(queryScoreVector.size() < Topk)
		upperbound = Topk;
	else
		upperbound = queryScoreVector.size();

    for(std::size_t i = 0; i < queryScoreVector.size(); ++i)
    {
        if(3 >= queryScoreVector[i].txt.length() || inputQuery == queryScoreVector[i].txt)
            continue;
        recommend.append(queryScoreVector[i].txt);
    }
    jsonResult["recommendation"] = recommend;

	LOG(INFO) << "The most similar keyword:" << ss 
		<< "\t biggest score:" << b_score;
}

//insert taobao rskeywords
//{"query":"","rskeywords":["",""]}
void recommendEngine::InsertRsKeywords(std::string& KeyAndRskey)
{
	if(KeyAndRskey.size() == 0)
		return;

	Json::Reader reader;
	Json::Value  rskey;
	vector<std::string> rskeywords;
	std::cout << KeyAndRskey << std::endl;
	bool flag = reader.parse(KeyAndRskey,rskey,false);
	if(flag)
	{
		for(std::size_t i = 0; i < rskey["rskeywords"].size();++i)
		{
			std::cout << rskey["rskeywords"][i] << ",";
			rskeywords.push_back(rskey["rskeywords"][i].asString());
		}
		std::cout << rskey["query"] << endl;
		std::string query = rskey["query"].asString();
		//indexer_->InsertRsKeywords(query,rskeywords);
	}

}

//query correction
void recommendEngine::recommendCorrection()
{
}

//related query recommendation
void recommendEngine::recommend(Terms2QidMap& terms2qIDs,QueryIdataMap& queryIdata
            ,QueryCateMap& query2Cate
			,Json::Value& jsonResult
			,String2IntMap& termsIdMap
            ,std::string inputQuery,const std::size_t TopK)
{
	//check
	//if(0 == terms2qIDs.size() || 0 == queryIdata.size() || 0 == termsIdMap.size())
	//	return;

	vector<std::size_t> qTermsID;
	vector<std::size_t> termsID;
	vector<DATA_TYPE> queryScoreMap;

    
	qTermsID.clear();
	termsID.clear();
	queryScoreMap.clear();

	Terms2QidMapIter termsIdIter;
	String2IntMapIter termsIter;
	float queryScore = 0.0;
	std::size_t cnt = 0;
	std::string queryText = "";
    DATA_TYPE dt;

	for(termsIter = termsIdMap.begin(); termsIter != termsIdMap.end(); ++termsIter)
	{
		termsID.push_back(termsIter->second);
	}
	float weight = 0.0;
	float similar = 0.0;
	//caculate score
	for(termsIdIter = terms2qIDs.begin(); termsIdIter != terms2qIDs.end(); ++termsIdIter)
	{
		for(std::size_t j = 0; j < termsIdIter->second.size(); ++j)
		{
			dt.txt = queryIdata[termsIdIter->second[j]].text;
			if(dt.txt.size() > 45)
			    continue;
			qTermsID = queryIdata[termsIdIter->second[j]].tid;
			caculateNM(termsID,qTermsID,cnt);

			similar = (float) cnt / (qTermsID.size() + 0.1) ;
			weight = (float)log(queryIdata[termsIdIter->second[j]].hits + 2.0)/(qTermsID.size() +0.1);
			dt.score = (float) weight * similar;
            sortByScore(queryScoreMap,dt);
		}
	}

	//get TopK 
	Json::Value recommend;
	for(std::size_t i = 0; i < queryScoreMap.size(); ++i)
	{
		if(3 >= queryScoreMap[i].txt.length() || inputQuery == queryScoreMap[i].txt
				|| jsonResult["NoResult_Recommend"] == queryScoreMap[i].txt)
			continue;
		recommend.append(queryScoreMap[i].txt);
	}
	jsonResult["recommedndation"] = recommend;
}

//need add new datas
bool recommendEngine::isNeedAdd()
{
	std::string path = workdir_ + "resource/";
	path += "newdata.txt";
	std::size_t mt = boost::filesystem::last_write_time(path);
	if(mt > timestamp_)
		return true;
	else
		return false;	
}

//build index engine
bool recommendEngine::isNeedBuild()
{
	//check time
	std::time_t mt;
	if(0 != timestamp_)
	{
		mt = time(NULL) - timestamp_;
//		std::cout << "times invertal:" << mt << std::endl;
	if(isNeedIndex || mt > 100)
		return true;
	else
	{
//		std::cout << "do not need indexing!\n";
		return false;
	}
	}
	else
		return true;
}

//final recommendation results
void recommendEngine::jsonResults(const std::string& userQuery,std::string& res)
{
    Json::Value jsonResult;
	Json::Value catJson;
	Json::Value category;

    Terms2QidMap terms2qIDs;
    QueryIdataMap queryIdata;
    QueryCateMap query2Cate;
	QueryCateMap rsKeywords;
    String2IntMap termsIdMap;

    if(indexer_->isForbidden(userQuery))
	{
		jsonResult["Forbidden"] = "true";
		jsonResult["NoResult_Recommend"] = "null";
		jsonResult["category"] = category;
		jsonResult["recommendation"] = catJson;
	}
	else
	{
		jsonResult["Forbidden"] = "false";
    //caculate cost time
    boost::posix_time::ptime time_start,time_end;
    boost::posix_time::millisec_posix_time_system_config::time_duration_type time_elapse;

    time_start = boost::posix_time::microsec_clock::universal_time();
	getCandicate(userQuery,terms2qIDs,queryIdata,query2Cate,rsKeywords,termsIdMap);
    time_end = boost::posix_time::microsec_clock::universal_time();
    time_elapse = time_end - time_start;
    int cost_time1 = time_elapse.ticks();

    time_start = boost::posix_time::microsec_clock::universal_time();
	recommendNoResults(terms2qIDs,queryIdata,query2Cate,rsKeywords
			,jsonResult,termsIdMap,userQuery);
    time_end = boost::posix_time::microsec_clock::universal_time();
    time_elapse = time_end - time_start;
    int cost_time2 = time_elapse.ticks();
    
    //time_start = boost::posix_time::microsec_clock::universal_time();
	//recommend(terms2qIDs,queryIdata,query2Cate,jsonResult,termsIdMap,userQuery);
    //time_end = boost::posix_time::microsec_clock::universal_time();
    //time_elapse = time_end - time_start;
    //int cost_time3 = time_elapse.ticks();
    
    LOG(INFO) << "GetCandicate time: " << cost_time1 << "us\tFind NoResult time:" 
              << cost_time2 <<  "us";

    Json::Value catJson;
    Json::Value category;
    vector<string> cate;

	QueryCateMapIter cateIter;
	for(cateIter = query2Cate.begin();cateIter != query2Cate.end(); ++cateIter)
    {
        category.clear();
        for(std::size_t i = 0; i < cateIter->second.size();++i)
        {
            cate.clear();
            catJson.clear();
            boost::split(cate,cateIter->second[i],boost::is_any_of(">"));
            for(std::size_t j = 0; j < cate.size()-1; ++j)
                catJson.append(cate[j]);
            category.append(catJson);
        }
    }
	
    jsonResult["category"] = category;
	}
	res = jsonResult.toStyledString();
}

void recommendEngine::clear()
{
	//clear dictornary in directory
	std::string dict_dir = workdir_ + "dict/dict/";
	if(boost::filesystem::exists(dict_dir))
	{
		boost::filesystem::remove_all(dict_dir);
	}
	boost::filesystem::create_directory(dict_dir);

}

void recommendEngine::buildEngine()
{
	std::string Tpth = workdir_ + "resource/";
	Tpth += timestamp;
	ofstream out;

	if(isNeedBuild())
		
	{
		std::cout << "Build the whole dictonary!\n";
		std::string pth = workdir_ + "resource/query.txt";
		indexer_->clear();
		indexer_->indexing(pth);
		clear();
		indexer_->flush();
		isNeedIndex = false;
		
		//update timestamp
		timestamp_ = time(NULL);
		out.open(Tpth.c_str(),std::ofstream::out | std::ofstream::trunc);
		out << timestamp_;
		out.close();
	}
	 if(isNeedAdd())
	{
		std::cout << "Add new datas into dictionary!\n";
		std::string path = workdir_ + "resource/newdata.txt";
		//indexer_->indexing(path);
		//indexer_->flush();
		isNeedIndex = false;

		//update timestamp
		timestamp_ = time(NULL);
		out.open(Tpth.c_str(),std::ofstream::out | std::ofstream::trunc);
		out << timestamp_;
		out.close();
	}
}


bool Is_subset(vector<std::size_t> v1,vector<std::size_t> v2)
{
	if(0 == v1.size() || 0 == v2.size())
		return false;
	boost::unordered_map<std::size_t,std::size_t> sets;
	sets.clear();
	
	for(std::size_t i = 0; i < v1.size(); ++i)
	{
		sets.insert(make_pair(v1[i],1));
	}
	
	for(std::size_t j = 0; j < v2.size(); ++j)
	{
		if(sets.end() == sets.find(v2[j]))
			return false;
	}
	return true;
}

void caculateNM(vector<std::size_t> v1,vector<std::size_t> v2,std::size_t& cnt)
{
	if(0 == v1.size() || 0 == v2.size())
	{
		cnt = 0;
		return;
	}
	boost::unordered_map<std::size_t,std::size_t> sets;
	sets.clear();
	cnt = 0;
	for(std::size_t i = 0; i < v1.size(); ++i)
		sets.insert(make_pair(v1[i],1));
	for(std::size_t j = 0; j < v2.size(); ++j)
	{
		if(sets.end() != sets.find(v2[j]))
			cnt += 1;
	}
}

void sortByScore(vector<DATA_TYPE>& queryScoreMap,DATA_TYPE& dt,const std::size_t& TopK)
{
    if(TopK == 0 || dt.score == 0)
        return;
    
    if(queryScoreMap.size() <= TopK)
    	{
    		queryScoreMap.push_back(dt);
        }
    else 
    {
        sort(queryScoreMap.begin(),queryScoreMap.end(),cmpByValue());
        if(dt.score > queryScoreMap.back().score)
        {
        	//std::cout << "test1---->" << dt.txt <<"\tscore:" << dt.score ;
        	for(std::size_t i = 0; i < queryScoreMap.size(); ++i)
				if(dt.txt == queryScoreMap[i].txt)
					return;
			queryScoreMap.pop_back();
        	queryScoreMap.push_back(dt);

        }
        dt.txt = "";
        dt.score = 0.0;
    }
}

/*
void sortByScore(map<std::string,float>& queryScoreMap,DATA_TYPE& dt,vector<PAIR>& vec,std::size_t TopK)
{
    if(TopK ==0 || dt.score == 0)
    	return;
    map<std::string,float>::iterator it;
    if(queryScoreMap.size() < TopK)
    {
    	if(queryScoreMap.end() ==  queryScoreMap.find(dt.txt))
    		queryScoreMap.insert(make_pair(dt.txt,dt.score));
    }
    else
    {
    	vec(queryScoreMap.begin(),queryScoreMap.end());
    	sort(vec.begin(),vec.end(),biggerthan());
    	if(dt.score > vec.back().score)
        {
        	it = queryScoreMap.find(vec.back().txt)

        	if(queryScoreMap.end() == queryScoreMap.find(dt.txt))
             queryScoreMap.insert(make_pair(dt.txt,dt.score));
            
        }
    }
}*/

/*************************************************************************
    @ File Name: indexEngine.cpp
    @ Method:
    @ Author: Jerry Shi
    @ Mail: jerryshi0110@gmail.com 
    @ Created Time: 2015年07月13日 星期一 13时51分14秒
 ************************************************************************/
#include "indexEngine.h"

//glog initialize

void initGlog(const char* cProgram, const char* logDir)
{
	if(!boost::filesystem::exists(logDir))
	{
		boost::filesystem::create_directory(logDir);
	}

	char cInfoPath[100];
	char cErrPath[100];
	char cWarnPath[100];
	char cFatalPath[100];

	//set log path
	snprintf(cInfoPath,sizeof(cInfoPath),"%s%s",logDir,"/INFO_");
	snprintf(cErrPath,sizeof(cErrPath),"%s%s",logDir,"/ERROR_");
	snprintf(cWarnPath,sizeof(cWarnPath),"%s%s",logDir,"/IWARNING_");
	snprintf(cFatalPath,sizeof(cFatalPath),"%s%s",logDir,"/FATAL_");

	google::InitGoogleLogging(cProgram);

	FLAGS_logbufsecs = 0; //no cache
	FLAGS_stop_logging_if_full_disk = true; // no write when disk is full
	FLAGS_alsologtostderr = false; // close to stderr

	google::SetLogDestination(google::GLOG_INFO,cInfoPath);
	google::SetLogDestination(google::GLOG_ERROR,cErrPath);
	google::SetLogDestination(google::GLOG_WARNING,cWarnPath);
	google::SetLogDestination(google::GLOG_FATAL,cFatalPath);
}

 boost::hash<std::string> hash_query;
//assistant function
void splitByPattern(const std::string& str,char splitPattern,vector<std::string>& res)
{
	if(0 == str.length())
		return;
	res.clear();
	std::size_t nPattern = std::count(str.begin(),str.end(),splitPattern);
	std::string s = str;
	while(nPattern--)
	{
		std::size_t pos = s.find(nPattern);
		if(std::string::npos != pos)
		{
			std::string ss = s.substr(0,pos);
			res.push_back(ss);
			s.assign(s,pos+1,s.length()-1);
		}
	}
	res.push_back(s);
}

//construct
indexEngine::indexEngine(const std::string& dir,const std::string& dict_pth)
	:dir_(dir),dict_pth_(dict_pth),isNeedflush(false)
{
	std::string cProgram = "RECOMMENDATION";
	std::string logDir = "../log";
	initGlog(cProgram.c_str(),logDir.c_str());

	if(!boost::filesystem::exists(dir_))
	{
		LOG(FATAL) << "Tokenize dictionary path not exits!";
		//std::cerr << "Tokenize dictionary path not exits!" << std::endl;
	}

	//tokenizer
	tok_ = new ilplib::knlp::HorseTokenize(dir_);
}

indexEngine::~indexEngine()
{
	if(tok_)
		delete tok_;
	tok_ = NULL;
}

bool indexEngine::open()
{
	if(0 == dict_pth_.length())
		return false;
	std::string termId_pth =  dict_pth_ + "/termId.v";
	std::string queryDat_pth = dict_pth_ + "/queryDat.v";
    
	ifstream finTermsId;
	ifstream finQueryDat;

	std::string sLine = "";
	std::string s = "";

	//open term id dictionary if exist , load data
	finTermsId.open(termId_pth.c_str());
	if(finTermsId.is_open())
	{
		vector<std::size_t> queryIdVector;
		vector<std::string> context;
		std::size_t termID;
		//std::size_t queryID;
		while(getline(finTermsId,sLine))
		{
			queryIdVector.clear();
			context.clear();
			if(0 >= sLine.length())
			    continue;
            boost::split(context,sLine,boost::is_any_of("\t"));
            if(2 > context.size())
                continue;
            termID = boost::lexical_cast<std::size_t>(context[0]);
            for(std::size_t it = 1;it < context.size()-1;++it)
            {
                std::size_t id = boost::lexical_cast<std::size_t>(context[it]);
                queryIdVector.push_back(id);
            }
			terms2qIDs_.insert(make_pair(termID,queryIdVector));
		}
	}
//	std::cout << "读入terms的大小：" << terms2qIDs_.size() << std::endl;
	finTermsId.close();
	//open query data exist if exist , load data
	finQueryDat.open(queryDat_pth.c_str());
	if(finQueryDat.is_open())
	{
		QueryData qDat;
		std::size_t queryID;
        std::size_t tID;
		sLine = "";
		s = "";
		vector<std::size_t> termsIdVector;
		vector<std::string> context;
		while(getline(finQueryDat,sLine))
		{
			termsIdVector.clear();
			qDat.tid.clear();
			context.clear();
			if(0>= sLine.length())
			    continue;
            boost::split(context,sLine,boost::is_any_of("\t"));
            if( 5 > context.size())
                continue;
            queryID = boost::lexical_cast<std::size_t>(context[0]);
            qDat.text = context[1];
            qDat.hits = boost::lexical_cast<std::size_t>(context[2]);
            qDat.counts = boost::lexical_cast<std::size_t>(context[3]);
            for(vector<std::string>::iterator it = context.begin()+4;it != context.end();++it)
            {
                tID = boost::lexical_cast<std::size_t>(*it);
                termsIdVector.push_back(tID);
            }
			qDat.tid = termsIdVector;
			queryIdata_.insert(make_pair(queryID,qDat));
		}
		
	}
//	std::cout << "读入query id 的大小:" << queryIdata_.size() << std::endl;
	finQueryDat.close();
	
	//load forbid keywords list
	ifstream finForbid;
	std::string forbid_pth = dict_pth_ + "/forbidden.v";
	finForbid.open(forbid_pth.c_str());
	if(finForbid.is_open())
	{
		sLine = "";
		forbidList_.clear();
		while(getline(finForbid,sLine))
		{
			if(0 == sLine.size())
				continue;
			forbidList_.insert(sLine);
		}
	}
	finForbid.close();

	//load category dictonary
	ifstream finQuery2Cate;
	string cate_pth = dict_pth_ + "/query2Cate.v";
    finQuery2Cate.open(cate_pth.c_str());
    if(finQuery2Cate.is_open())
    {
        vector<std::string> context;
        vector<std::string> cate;
        sLine = "";
        std::size_t qID;
        while(getline(finQuery2Cate,sLine))
        {
            cate.clear();
            context.clear();
            if(0 >= sLine.length())
                continue;
            std::size_t pos = sLine.find('\t');
            if(std::string::npos == pos)
            {
                continue;
            }
            else
            {
                string ss = sLine.substr(0,pos);
                sLine.assign(sLine,pos+1,sLine.length()-1);
                qID = boost::lexical_cast<std::size_t>(ss);
                if( std::string::npos != sLine.find(","))
                {
                boost::split(context,sLine,boost::is_any_of(","));
               // if(1 > context.size())
                //    continue;
                for(std::size_t id = 0; id < context.size();++id)
                    cate.push_back(context[id]);
                }
                else
                    cate.push_back(sLine);
            }
            query2Cate_.insert(make_pair(qID,cate));
        }

    }
    finQuery2Cate.close();
    std::cout << "读入的category size：" << query2Cate_.size() <<  std::endl;

	LOG(INFO) << "Load terms dictionary size:" << terms2qIDs_.size()
		<< "\tLoad keywords size:" << queryIdata_.size()
		<< "\tLoad category size:" << query2Cate_.size()
		<< "\tLoad forbidden keywords size:" << forbidList_.size();

	if(0 == terms2qIDs_.size() || 0 == queryIdata_.size())
	{
		isNeedflush = true;
		return true;
	}
	else 
		return false; // load dictionary successfully!
}

void indexEngine::insert(QueryData& userQuery)
{
	if(0 == userQuery.text.length())
		return;
	
	std::size_t queryID = izenelib::util::izene_hashing(userQuery.text);
	boost::unordered_map<std::size_t,QueryData>::iterator queryIter;
	queryIter = queryIdata_.find(queryID);
	if(queryIdata_.end() != queryIter && queryIter->second.text == userQuery.text)
	{
		queryIter->second.hits = userQuery.hits;
		return;
	}
	else
	{
	//generated terms id
	if(1 > userQuery.tid.size())
	{
		String2IntMap termsMap;
		String2IntMapIter termsMapIter;
		vector<std::size_t> termsIdVector;
		vector<std::size_t> queryIdVector;

		Terms2QidMapIter termsIdIter;

		tokenTerms(userQuery.text,termsMap);
		for(termsMapIter = termsMap.begin(); termsMapIter != termsMap.end(); ++termsMapIter)
		{
			termsIdVector.push_back(termsMapIter->second);
			termsIdIter = terms2qIDs_.find(termsMapIter->second);
			//inset into terms dictionary
			if(terms2qIDs_.end() != termsIdIter)
				termsIdIter->second.push_back(queryID);
			else
			{
				queryIdVector.push_back(queryID);
				terms2qIDs_.insert(make_pair(termsMapIter->second,queryIdVector));
			}
		}
		userQuery.tid = termsIdVector;
	}
	//insert in query data
	queryIdata_.insert(make_pair(queryID,userQuery));
	}
}

//get candicate query id,category
String2IntMap indexEngine::search(const std::string& userQuery,Terms2QidMap& candicateQid
		,QueryIdataMap& candicateQuery,QueryCateMap& candicateCate)
{
	std::string nstr = userQuery;
	if(0 != userQuery.length())
	{
		Normalize::normalize(nstr);
		candicateQid.clear();
		candicateQuery.clear();
        candicateCate.clear();

		String2IntMap termsMap;
		String2IntMapIter termsMapIter;

		tokenTerms(nstr,termsMap);
		Terms2QidMapIter termsQidIter;

	//get candicate query id
	for(termsMapIter = termsMap.begin(); termsMapIter != termsMap.end(); ++termsMapIter)
	{
		termsQidIter = terms2qIDs_.find(termsMapIter->second);
		if(terms2qIDs_.end() != termsQidIter)
		{
			candicateQid.insert(make_pair(termsMapIter->second,termsQidIter->second));
		}
	}

	QueryIdataIter queryIter; 
	//get candicate query data
	for(termsQidIter = candicateQid.begin(); termsQidIter != candicateQid.end(); ++termsQidIter)
	{
		for(std::size_t i = 0; i < termsQidIter->second.size(); ++i)
		{
			queryIter = queryIdata_.find(termsQidIter->second[i]);
			if(queryIdata_.end() != queryIter)
            {
                if(queryIter->second.text.size() > 30)
                    continue;
				candicateQuery.insert(make_pair(termsQidIter->second[i],queryIter->second));
            }
		}
	}

//	std::cout << "termsmap.size" << termsMap.size() << "\tcandicateqid.size()" 
//		<< candicateQid.size() <<"\tcandicate query.size()" 
//		<< candicateQuery.size()<< std::endl;
    //get candicate category
	std::string str = userQuery;
	Normalize::normalize(str);
    std::size_t qID = hash_query(str);
    QueryCateMapIter cateIter;
    cateIter = query2Cate_.find(qID);
    if(query2Cate_.end() != cateIter)
    {
        candicateCate.insert(make_pair(qID,cateIter->second));
    }
	return termsMap;
	}
	LOG(INFO) << "User query terms num:" << candicateQid.size() 
		<< "\tCandicate query num:" << candicateQuery.size() 
		<< "\tCandicate category num:" << candicateCate.size();
}

void indexEngine::indexing(const std::string& corpus_pth)
{
	ifstream ifOrigin_data; //file stream to load data from corpus
	ifOrigin_data.open(corpus_pth.c_str());
	if(!ifOrigin_data.is_open())
		//std::cerr << "Open corpus files failed!" << std::endl;
		LOG(ERROR) << "Open indexing corpus files error!";

	std::string sLine = "";
	std::string sData = "";
	vector<string> splitDat;
	
	while(getline(ifOrigin_data,sLine))
	{
		splitDat.clear();
		
		//normalize
		Normalize::normalize(sLine);

		//get detail data from original corpus
		for(unsigned int i = 0; i < 3; ++i)
		{
			std::size_t pos = sLine.find('\t');
			if(std::string::npos != pos)
			{
				sData = sLine.substr(0,pos);
				sLine.assign(sLine,pos+1,sLine.length()-1);
				splitDat.push_back(sData);
			}
		}
		splitDat.push_back(sLine);

		//check data
		if(4 != splitDat.size())
			continue;
		QueryData qDat;
		qDat.text = splitDat[0]; //query text
		qDat.hits = atoi(splitDat[2].c_str()); //query times
		qDat.counts = atoi(splitDat[3].c_str());//results numbers

		//get term id
		String2IntMap termsVector;
		tokenTerms(qDat.text,termsVector);
		
		std::size_t queryID = izenelib::util::izene_hashing(qDat.text);
		vector<std::size_t> queryIdVector;
		vector<std::size_t> termsIdVector;
		
		Terms2QidMapIter termsQueryIter;
		queryIdVector.push_back(queryID);
		String2IntMapIter termsIter;
		//assign hash id for every terms
		for(termsIter = termsVector.begin(); termsIter != termsVector.end(); ++termsIter)
		{
			termsIdVector.push_back(termsIter->second);
			//find terms in dictornary,termsID.v
			termsQueryIter = terms2qIDs_.find(termsIter->second);
			if(termsQueryIter != terms2qIDs_.end())
			{
				termsQueryIter->second.push_back(queryID); 
			}
			else
			{
				terms2qIDs_.insert(make_pair(termsIter->second,queryIdVector));
			}
		}

		//complete data for one query
		qDat.tid = termsIdVector;
		boost::unordered_map<std::size_t,QueryData>::iterator queryIter;
		queryIdata_.insert(make_pair(queryID,qDat));
		
		 //flush to disk file
		/* ofQueryDat << queryID << "\t" <<qDat.text << "\t" << qDat.hits 
		   << "\t" << qDat.counts;
		 for(unsigned int i = 0; i < qDat.tid.size(); ++i)
		 {
			ofQueryDat << "\t" << qDat.tid[i];
			//cout << "terms id :" << qDat.tid[i] << ",";
		 }
		 ofQueryDat << std::endl;
		 
		
		//merge the searching times
		boost::unordered_map<std::size_t,QueryData>::iterator queryIter;
		queryIter = queryIdata_.find(queryID);
		if(queryIdata_.end() != queryIter && queryIter->second.text == qDat.text)
		{
			std::cout << "queryIter->seoncd.text:" << queryIter->second.text << 
				"\t qDat.text:" << qDat.text << std::endl;
			std::cout << "queryID:" << queryID << "\tqueryIter->first:" << queryIter->first << std::endl; 
			std::cout << "\t text:" << qDat.text << std::endl;	
			queryIter->second.hits += qDat.hits;
			std::cout  << "test--hits:" << queryIter->second.hits << "\t qdat.hits:" << qDat.hits << std::endl; 	
		}
		else
			queryIdata_.insert(make_pair(queryID,qDat));
		queryIdata_.insert(make_pair(queryID,qDat));*/
	}
	ifOrigin_data.close();//file stream close
	//ofQueryDat.close();
}

bool indexEngine::isForbidden(const std::string& userQuery)
{
	if(userQuery.size() == 0)
		return false;
	std::set<std::string>::iterator it;
	for(it = forbidList_.begin(); it != forbidList_.end(); ++it)
	{
		if(std::string::npos != userQuery.find(*it))
			return true;
	}

	return false;
}

void indexEngine::flush()
{
//	if(!isNeedflush)
//		return;
//	std::cout << "flush to disk\n";
	ofstream ofTermsId;
	ofstream ofQueryDat;

	std::string termId_pth =  dict_pth_ + "/termId.v";
	std::string queryDat_pth = dict_pth_ + "/queryDat.v";

	ofTermsId.open(termId_pth.c_str(),ios::app);
	if(!ofTermsId.is_open())
		LOG(ERROR) << "Open terms dictionary error!";

	ofQueryDat.open(queryDat_pth.c_str(),ios::app);
	if(!ofQueryDat.is_open())
		LOG(ERROR) << "Open queryDat dictionary error!";

	//flush query data to disk,queryDat.v
	boost::unordered_map<std::size_t,QueryData>::iterator queryIter;
	for(queryIter = queryIdata_.begin(); queryIter != queryIdata_.end(); ++queryIter)
	{
		ofQueryDat << queryIter->first << "\t" << queryIter->second.text << "\t"
			        << queryIter->second.hits << "\t" << queryIter->second.counts;
		for(unsigned int i = 0; i < queryIter->second.tid.size(); ++i)
		{
			ofQueryDat << "\t" << queryIter->second.tid[i];
		}
		ofQueryDat << std::endl;
	}

	//flush terms id,candicate query id to disk,termsId.v
	boost::unordered_map<std::size_t,vector<std::size_t> >::iterator termsQueryIter;
	for(termsQueryIter = terms2qIDs_.begin(); termsQueryIter != terms2qIDs_.end(); ++termsQueryIter)
	{
		//std::cout << "Terms:" << termsQueryIter->first << std::endl;
		//std::cout << "\tQuery id:" << std::endl;
		ofTermsId << termsQueryIter->first << "\t";
		for(unsigned int i = 0; i < termsQueryIter->second.size(); ++i)
		{
			//std::cout << termsQueryIter->second[i] << ",";
			ofTermsId << termsQueryIter->second[i] << "\t";
		}
		std::cout << std::endl;
		ofTermsId << std::endl;
	}

	ofTermsId.close();
	ofQueryDat.close();
	//std::cout << "test size of quey data:" << queryIdata_.size() << std::endl;
}

void indexEngine::clear()
{
	terms2qIDs_.clear();
	queryIdata_.clear();
}

bool indexEngine::isUpdate()
{
}

//return terms and it's hash value
void indexEngine::tokenTerms(const std::string& userQuery,String2IntMap& termsMap)
{
	if(0 == userQuery.length())
		return;
	termsMap.clear();
	vector<pair<std::string,float> > segTerms;
	try
	{
		tok_->tokenize(userQuery,segTerms);
	}
	catch(...)
	{
		segTerms.clear();
	}

	//dedup
	String2IntMapIter termsMapIter;
	for(std::size_t i = 0; i < segTerms.size(); ++i)
	{
		termsMapIter = termsMap.find(segTerms[i].first);
		if(termsMap.end() != termsMapIter && termsMapIter->first == segTerms[i].first)
			continue;
		else
		{
			std::size_t termsID = izenelib::util::izene_hashing(segTerms[i].first);
			termsMap.insert(make_pair(segTerms[i].first,termsID));
		}
	}
}

#include "MyTool.h"

FuncLogInfo myFuncLogInfo[MAX_LOGGED_FUNC];
map<string, int> myFuncLogInfoMap;
int myFuncLogInfoCnt;

FuncCondInfo myFuncCondInfo[MAX_LOGGED_FUNC];
map<string, int> myFuncCondInfoMap;
int myFuncCondInfoCnt;

map<string, int> myFileMap;


FILE* in;
FILE* out;

string lastName;
int fileNum;

int cntfunc;
int cntlog;
int cntulog;

int totalLogNum;

int returnCodeNum;
int totalReturnNum;



static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp(
	"\tFor example, to run clang-mytool on all files in a subtree of the\n"
	"\tsource tree, use:\n"
	"\n"
	"\t  find path/in/subtree -name '*.cpp'|xargs clang-mytool\n"
	"\n"
	"\tor using a specific build path:\n"
	"\n"
	"\t  find path/in/subtree -name '*.cpp'|xargs clang-mytool -p build/path\n"
	"\n"
	"\tNote, that path/in/subtree and current directory should follow the\n"
	"\trules described above.\n"
	"\n"
);

static cl::OptionCategory ClangMytoolCategory("clang-mytool options");
static std::unique_ptr<opt::OptTable> Options(createDriverOptTable());

static cl::opt<bool> RecordCondition(
	"find-log-condition",
    cl::desc("Print all the log condition."),
    cl::cat(ClangMytoolCategory));

static cl::opt<bool> InsertLog(
	"insert-log-statement",
                               cl::desc("Insert log statement."),
                               cl::cat(ClangMytoolCategory));

static cl::opt<bool> FindLogFunction(
	"find-logging-function",
                                     cl::desc("Find logging function."),
                                     cl::cat(ClangMytoolCategory));

static cl::opt<bool> CountReturnCode(
	"count-return-code",cl::desc("Count the number of return error code and total return statement."),
                                     cl::cat(ClangMytoolCategory));

static cl::opt<bool> FindLoggedSnippet(
                                     "find-logged-snippet",cl::desc("Find the logged snippets."),
                                     cl::cat(ClangMytoolCategory));

void readInfluencingCond(){
	
	if((in = fopen("./influencing_condition.out","r")) == NULL) {
		errs()<<"Miss the need log function file \"influencing_condition.out\".\n";
	}
	else {
		while(!feof(in)) {
			char buf[300];
			fgets(buf, 300, in);
			if(strlen(buf) <= 1) break;
			buf[strlen(buf)-1] = '\0';
			
			string readline = buf;
			
			if(readline[0] == '2') continue;
			
			string funcName;
			string funcCond;
			int funcCondTime;
			string time;
			
			time = readline.substr(readline.find_last_of(',') + 1, string::npos);
			readline = readline.substr(0, readline.find_last_of(','));
			funcCondTime = atoi(time.c_str());
			
			funcCond = readline.substr(readline.find_last_of(',') + 1, string::npos);
			readline = readline.substr(0, readline.find_last_of(','));
			
			funcName = readline.substr(readline.find_last_of(',') + 1, string::npos);
			readline = readline.substr(0, readline.find_last_of(','));
			
			if(funcCondTime == 0)break;
			//llvm::errs()<<funcName<<" "<<funcCond<<" "<<funcCondTime<<"\n";
			
			if(myFuncCondInfoMap[funcName] == 0){
				myFuncCondInfoMap[funcName] = ++myFuncCondInfoCnt;
			}
			int index = myFuncCondInfoMap[funcName];
			myFuncCondInfo[index].setFuncName(funcName);
			myFuncCondInfo[index].addCond(funcCond, funcCondTime);
		}
		fclose(in);
	}
}

void readLogFunction(){
    
    if((in = fopen("./logging_function.out","r")) == NULL) {
        errs()<<"Miss the need log function file \"logging_function.out\".\n";
    }
    else {
        while(!feof(in)) {
            char buf[300];
            fgets(buf, 300, in);
            if(strlen(buf) <= 1) continue;
            buf[strlen(buf)-1] = '\0';
            string name = buf;
            llvm::outs()<<name<<"\n";
        }
    }
    
}

void readLoggedFunction(){
	
	if((in = fopen("./logged_snippet.out","r")) == NULL) {
		errs()<<"Miss the need log function file \"logged_snippet.out\".\n";
	}
	else {
		while(!feof(in)) {
			char buf[300];
			fgets(buf, 300, in);
			if(strlen(buf) <= 1) break;
			buf[strlen(buf)-1] = '\0';
			
			string name = buf;
			string funcLocFile;
			string funcLocLine;
			string logName;
			string logLocFile;
			string logLocLine;
			
			logLocLine = name.substr(name.find_last_of(':') + 1, string::npos);
			name = name.substr(0, name.find_last_of(':'));
			
			logLocFile = name.substr(name.find_last_of('@') + 1, string::npos);
			name = name.substr(0, name.find_last_of('@'));
			
			logName = name.substr(name.find_last_of('#') + 1, string::npos);
			name = name.substr(0, name.find_last_of('#'));
			
			funcLocLine = name.substr(name.find_last_of(':') + 1, string::npos);
			name = name.substr(0, name.find_last_of(':'));
			
			funcLocFile = name.substr(name.find_last_of('@') + 1, string::npos);
			name = name.substr(0, name.find_last_of('@'));
			
			int funcLine = atoi(funcLocLine.c_str());
			int logLine = atoi(logLocLine.c_str());
			
			if(myFuncLogInfoMap[name] == 0){
				myFuncLogInfoMap[name] = ++myFuncLogInfoCnt;
			}
			int index = myFuncLogInfoMap[name];
			//llvm::outs()<<"AAA"<<index<<" "<<name<<" "<<funcLocFile<<" "<<funcLine<<" "<< 
			//logName<<" "<<logLocFile<<" "<<logLine<<"\n";
			myFuncLogInfo[index].setFuncName(name);
			LogInfo l(funcLocFile, funcLine, logName, logLocFile, logLine);
			myFuncLogInfo[index].addLog(l);
		}
		fclose(in);
	}
}

void printResult(){
	out = fopen("./sl_runtime_info","w");
	//freopen("/home/jiazhouyang/mysql-5.6.17/liblog.h","w",stdout);
	
	fprintf(out, "------------------Result------------------\n");
	
}

int main(int argc, const char **argv) {
	//llvm::sys::PrintStackTraceOnErrorSignal();
	
	//vector<string> Sources;
	//Sources.push_back("/home/jiazhouyang/httpd-2.4.10/server/mpm/event/event.c");
	//Sources.push_back("/home/jiazhouyang/httpd-2.4.10/os/unix/unixd.c");
	
	
	CommonOptionsParser OptionsParser(argc, argv, ClangMytoolCategory);
	vector<string> source = OptionsParser.getSourcePathList();
	
	bool diag = true;
	
	fileNum = 0;
	lastName = "";
		
	std::unique_ptr<FrontendActionFactory> FrontendFactory;
    
    if(FindLoggedSnippet){
        
        readLogFunction();
        FrontendFactory = newFrontendActionFactory<RecordCondAction>();
        
        for(unsigned i = 0; i < source.size(); i++){
            vector<string> mysource;
            mysource.push_back(source[i]);
            ClangTool Tool(OptionsParser.getCompilations(), mysource);
            Tool.clearArgumentsAdjusters();
            if(diag){IgnoringDiagConsumer* idc = new IgnoringDiagConsumer();
                Tool.setDiagnosticConsumer(idc);}
            //Tool.run(FrontendFactory.get());
        }
    }
    
	if(RecordCondition){
		
		myFuncLogInfoMap.clear();
		myFuncLogInfoCnt = 0;
		
		readLoggedFunction();
		
		cntfunc = 0;
		cntlog = 0;
		cntulog = 0;
        
		FrontendFactory = newFrontendActionFactory<RecordCondAction>();	
			
		for(unsigned i = 0; i < source.size(); i++){
			vector<string> mysource;
			mysource.push_back(source[i]);
			ClangTool Tool(OptionsParser.getCompilations(), mysource);
			Tool.clearArgumentsAdjusters();
			if(diag){IgnoringDiagConsumer* idc = new IgnoringDiagConsumer();	
			Tool.setDiagnosticConsumer(idc);}
			Tool.run(FrontendFactory.get());
		}
	
		if(!DEBUG){
			for(int i = 1; i < myFuncLogInfoCnt; i++){
				myFuncLogInfo[i].simplify();
				//myFuncLogInfo[i].print();
				myFuncLogInfo[i].countCond(2);
			}
			llvm::errs()<<"total logged func: "<<myFuncLogInfoCnt<<"\n";
			llvm::errs()<<"total checking cond: "<<cntlog<<"\n";
			llvm::errs()<<"total unknown cond: "<<cntulog<<"\n";

			for(int i = 1; i < myFuncLogInfoCnt; i++){
				myFuncLogInfo[i].countArgu();
			}
		}
	}
	
	if(InsertLog){
		myFuncCondInfoMap.clear();
		myFuncCondInfoCnt = 0;
		
		readInfluencingCond();
		
		totalLogNum = 0;
		
		FrontendFactory = newFrontendActionFactory<InsertLogAction>();
		
		for(unsigned i = 0; i < source.size(); i++){
			vector<string> mysource;
			mysource.push_back(source[i]);
			ClangTool Tool(OptionsParser.getCompilations(), mysource);
			Tool.clearArgumentsAdjusters();
			if(diag){IgnoringDiagConsumer* idc = new IgnoringDiagConsumer();	
			Tool.setDiagnosticConsumer(idc);}
			Tool.run(FrontendFactory.get());
		}
		
// 		for(int i = 1; i < myFuncCondInfoCnt; i++){
// 			myFuncCondInfo[i].print();
// 			llvm::errs()<<"\n";
// 		}
		
		llvm::errs()<<"Total "<<totalLogNum<<" functions are logged!\n";
		
		//printResult();
	}

	if(FindLogFunction){
		FrontendFactory = newFrontendActionFactory<FindLogAction>();
		for(unsigned i = 0; i < source.size(); i++){
			vector<string> mysource;
			mysource.push_back(source[i]);
			ClangTool Tool(OptionsParser.getCompilations(), mysource);
			Tool.clearArgumentsAdjusters();
			if(diag){IgnoringDiagConsumer* idc = new IgnoringDiagConsumer();	
			Tool.setDiagnosticConsumer(idc);}
			Tool.run(FrontendFactory.get());
		}
		
	}
	
	if(CountReturnCode){
		
		returnCodeNum = 0;
		totalReturnNum = 0;
		
		FrontendFactory = newFrontendActionFactory<CountRetAction>();
		for(unsigned i = 0; i < source.size(); i++){
			vector<string> mysource;
			mysource.push_back(source[i]);
			ClangTool Tool(OptionsParser.getCompilations(), mysource);
			Tool.clearArgumentsAdjusters();
			if(diag){IgnoringDiagConsumer* idc = new IgnoringDiagConsumer();	
			Tool.setDiagnosticConsumer(idc);}
			Tool.run(FrontendFactory.get());
		}
		
		llvm::errs()<<"Total "<<returnCodeNum<<" error return code!\n";
		llvm::errs()<<"Total "<<totalReturnNum<<" return statement!\n";
	}
	
	return 0;
}

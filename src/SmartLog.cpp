//
//  SmartLog.cpp
//  LLVM
//
//  Created by ZhouyangJia on 16/4/13.
//
//

#include "SmartLog.h"
#include "FindKeywordFunction.h"
#include "FindLoggingFunction.h"
#include "FindLoggedSnippet.h"
#include "FindOutputSnippet.h"
#include "FindPatternSnippet.h"
#include "LogTimes.h"


///shared variables
//count files
string lastName;
int fileNum;
int fileTotalNum;

//input and output files
FILE* in;
FILE* out;
FILE* out2;


///for find-logging-function
//record the features of each function
FunctionFeat myCalledFeat[MAX_FUNC_NUM];
map<string, int> myCalledFeatMap;
int myCalledFeatCnt;

//count results
int totalLoggingFunction;


///for find-keyword-function
//record visited functions
map<string, int> myKeywordFunc;

//count results
int totalKeywordFunction;
int totalKeywordCallsite;


///for find-*-snippet
//choose the type of logging functions
enum logging_function{
    LOGGING_FUNCTION,
    KEYWORD_FUNCTION
};

//record logging functions
string logNames[MAX_LOG_FUNC];
int logNamesCnt;

//count results
int totalLoggedSnippet;
int totalOutputSnippet;
int totalPatternSnippet;


///for log-times
//record function info
LogTime myLogTime[MAX_FUNC_NUM];
map<string, int> myLogTimeMap;
int myLogTimeCnt;

//record logged snippet
string myFuncNames[MAX_FUNC_NUM];
string myLogNames[MAX_FUNC_NUM];
//string myLocNames[MAX_FUNC_NUM];
int myNamesCnt;


static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp(
	"\tFor example, to run clang-smartlog on all files in a subtree of the\n"
	"\tsource tree, use:\n"
	"\n"
	"\t  find path/in/subtree -name '*.cpp'|xargs clang-smartlog\n"
	"\n"
	"\tor using a specific build path:\n"
	"\n"
	"\t  find path/in/subtree -name '*.cpp'|xargs clang-smartlog -p build/path\n"
	"\n"
	"\tNote, that path/in/subtree and current directory should follow the\n"
	"\trules described above.\n"
	"\n"
);

static cl::OptionCategory ClangMytoolCategory("clang-smartlog options");
static std::unique_ptr<opt::OptTable> Options(createDriverOptTable());

static cl::opt<bool> FindKeywordFunction("find-keyword-function",
                                         cl::desc("Find keyword functions."),
                                         cl::cat(ClangMytoolCategory));

static cl::opt<bool> FindLoggingFunction("find-logging-function",
                                     cl::desc("Find logging functions."),
                                     cl::cat(ClangMytoolCategory));

static cl::opt<bool> FindOutputSnippet("find-output-snippet",
                                       cl::desc("Find output snippets."),
                                       cl::cat(ClangMytoolCategory));

static cl::opt<bool> FindPatternSnippet("find-pattern-snippet",
                                       cl::desc("Find pattern snippets."),
                                       cl::cat(ClangMytoolCategory));

static cl::opt<bool> FindLoggedSnippet("find-logged-snippet",
                                       cl::desc("Find logged snippets."),
                                       cl::cat(ClangMytoolCategory));

static cl::opt<bool> LogTimes("log-times",
                              cl::desc("Calculate log times of each function."),
                              cl::cat(ClangMytoolCategory));

static cl::opt<bool> LogScore("log-score",
                             cl::desc("Calculate log score."),
                             cl::cat(ClangMytoolCategory));


///read logged snippet
void readLoggedFunction(){
    
    myNamesCnt = 0;
    
    in = fopen("./logged_snippet.out", "r");
    
    if(in == NULL) {
        errs()<<"Miss file \"logged_snippet_key.out\".\n";
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
            
            //int funcLine = atoi(funcLocLine.c_str());
            //int logLine = atoi(logLocLine.c_str());
            myFuncNames[myNamesCnt] = name;
            myLogNames[myNamesCnt] = logName;
            //myLocNames[myNamesCnt] = logLocFile.append(logLocLine);
            myNamesCnt++;
        }
        fclose(in);
    }
}

int getLevel(string name){
    
    if(name.find("crit") != string::npos || name.find("emerg") != string::npos){
        return 5;
    }
    
    if(name.find("alert") != string::npos || name.find("err") != string::npos || name.find("die") != string::npos
       || name.find("fail") != string::npos || name.find("crit") != string::npos){
        return 4;
    }
    
    if(name.find("log") != string::npos || name.find("assert") != string::npos || name.find("warn") != string::npos
       || name.find("print") != string::npos){
        return 3;
    }
    
    if(name.find("out") != string::npos || name.find("out") != string::npos || name.find("write") != string::npos
       || name.find("dump") != string::npos || name.find("msg") != string::npos || name.find("message") != string::npos
       || name.find("hint") != string::npos || name.find("trace") != string::npos || name.find("report") != string::npos
       || name.find("record")){
        return 2;
    }

    if(name.find("debug") != string::npos){
        return 1;
    }

    return 0;
}


///read logging function
void readLoggingFunction(enum logging_function logging_method){
    
    logNamesCnt = 0;
    
    if(logging_method == LOGGING_FUNCTION)
        in = fopen("./logging_function.out", "r");
    else if(logging_method == KEYWORD_FUNCTION)
        in = fopen("./keyword_function.out","r");
    else{
        errs()<<"Error logging method!\n";
        return;
    }
    
    if(in == NULL) {
        errs()<<"Miss file \"logging_function.out\".\n";
    }
    else {
        while(!feof(in)) {
            char buf[300];
            fgets(buf, 300, in);
            if(strlen(buf) <= 1) break;
            buf[strlen(buf)-1] = '\0';
            string name = buf;
            logNames[logNamesCnt++] = name;
            
            if(logNamesCnt == MAX_LOG_FUNC){
                errs()<<"Too many logging functions!\n";
                exit(1);
            }
        }
        fclose(in);
    }
}


///read data flow feature, got from IR, used for choose logging funtions
void readFunctionFlow(){
    
    if((in = fopen("./function_flow.out","r")) == NULL) {
        errs()<<"Miss file \"function_flow.out\".\n";
    }
    else {
        while(!feof(in)) {
            char buf[300];
            fgets(buf, 300, in);
            if(strlen(buf) <= 1) break;
            buf[strlen(buf)-1] = '\0';
            string name = buf;
            
            //outs()<<name<<"\n";
            
            int index = myCalledFeatMap[name];
            myCalledFeat[index].flow = 1;

        }
        fclose(in);
    }
}


int main(int argc, const char **argv) {
	
	CommonOptionsParser OptionsParser(argc, argv, ClangMytoolCategory);
	vector<string> source = OptionsParser.getSourcePathList();
    fileTotalNum = source.size();
    
    std::unique_ptr<FrontendActionFactory> FrontendFactory;
    
    if(FindLoggingFunction){
        
        llvm::errs()<<"Find logging functions:\n";
        
        bool diag = true;
        fileNum = 0;
        lastName = "";
        totalLoggingFunction = 0;
        
        myCalledFeatCnt = 0;
        myCalledFeatMap.clear();
        
        FrontendFactory = newFrontendActionFactory<FindLoggingAction>();
        for(unsigned i = 0; i < source.size(); i++){
            vector<string> mysource;
            mysource.push_back(source[i]);
            ClangTool Tool(OptionsParser.getCompilations(), mysource);
            Tool.clearArgumentsAdjusters();
            if(diag){IgnoringDiagConsumer* idc = new IgnoringDiagConsumer();
                Tool.setDiagnosticConsumer(idc);}
            Tool.run(FrontendFactory.get());
        }
        
        
        out = fopen("logging_function.out","w");
        
        //outs()<<"funcName\tfuncKeyWord\tfileName\tfileKeyWord\tlenth\tflow\tcalledNumber\tfileNumber\thaschar\tdecl\tlabel\n";
        for(int i = 1; i < myCalledFeatCnt; i++){
            //myCalledFeat[i].print();
            
            if(myCalledFeat[i].calledNumber<3 || myCalledFeat[i].fileNumber<2)
                myCalledFeat[i].label = -1;
            
            if(myCalledFeat[i].funcKeyWord &&
               myCalledFeat[i].lenth < 100 &&
               myCalledFeat[i].calledNumber >= 10 &&
               (myCalledFeat[i].haschar || !myCalledFeat[i].decl) &&
               myCalledFeat[i].fileNumber >= 2){
                
                myCalledFeat[i].label = 1;
                fputs(myCalledFeat[i].funcName.c_str(), out);
                fputs("\n", out);
                //outs()<<myCalledFeat[i].funcName<<"\n";
                totalLoggingFunction++;
                
                logNames[logNamesCnt++] = myCalledFeat[i].funcName;
                if(logNamesCnt == MAX_LOG_FUNC){
                    errs()<<"Too many logging functions!\n";
                    exit(1);
                }
            }
        }
        
        for(int i = 1; i < myCalledFeatCnt; i++){
            if(myCalledFeat[i].label != 1){
                if(myCalledFeat[i].funcKeyWord && (myCalledFeat[i].calledNumber >= 100 || myCalledFeat[i].fileNumber >= 10)){
                    
                    myCalledFeat[i].label = 1;
                    fputs(myCalledFeat[i].funcName.c_str(), out);
                    fputs("\n", out);
                    //outs()<<myCalledFeat[i].funcName<<"\n";
                    totalLoggingFunction++;
                    
                    logNames[logNamesCnt++] = myCalledFeat[i].funcName;
                    if(logNamesCnt == MAX_LOG_FUNC){
                        errs()<<"Too many logging functions!\n";
                        exit(1);
                    }
                }
            }
        }
        llvm::errs()<<"Total logging functions: "<<totalLoggingFunction<<"\n";
        
        
        readFunctionFlow();
        
        outs()<<"funcName,funcKeyWord,fileName,fileKeyWord,lenth,flow,calledNumber,fileNumber,haschar,decl,label\n";
        for(int i = 1; i < myCalledFeatCnt; i++){
            myCalledFeat[i].print();
        }
        
        fclose(out);
    }
    
    if(FindKeywordFunction){
        
        llvm::errs()<<"Find keyword functions:\n";
        
        bool diag = true;
        fileNum = 0;
        lastName = "";
        totalKeywordFunction = 0;
        totalKeywordCallsite = 0;
        
        
        myKeywordFunc.clear();
        
        out = fopen("keyword_function.out","w");
        out2 = fopen("keyword_callsite.out","w");
        
        FrontendFactory = newFrontendActionFactory<FindKeywordAction>();
        for(unsigned i = 0; i < source.size(); i++){
            vector<string> mysource;
            mysource.push_back(source[i]);
            ClangTool Tool(OptionsParser.getCompilations(), mysource);
            Tool.clearArgumentsAdjusters();
            if(diag){IgnoringDiagConsumer* idc = new IgnoringDiagConsumer();
                Tool.setDiagnosticConsumer(idc);}
            Tool.run(FrontendFactory.get());
        }
        
        
        /*FrontendFactory = newFrontendActionFactory<FindKeywordAction>();
        ClangTool Tool(OptionsParser.getCompilations(), source);
        Tool.run(FrontendFactory.get());*/
        
        
        llvm::errs()<<"Total keyword functions: "<<totalKeywordFunction<<"\n";
        llvm::errs()<<"Total keyword callsites: "<<totalKeywordCallsite<<"\n";
        
        fclose(out);
        fclose(out2);
    }
    
    if(FindOutputSnippet){
        
        llvm::errs()<<"Find output snippets:\n";
        
        bool diag = true;
        fileNum = 0;
        lastName = "";
        totalOutputSnippet = 0;
        
        enum logging_function loggingMethod;
        loggingMethod = KEYWORD_FUNCTION;
        readLoggingFunction(loggingMethod);
        
        FrontendFactory = newFrontendActionFactory<FindOutputAction>();
        
        out = fopen("output_snippet.out","w");
        for(unsigned i = 0; i < source.size(); i++){
            vector<string> mysource;
            mysource.push_back(source[i]);
            ClangTool Tool(OptionsParser.getCompilations(), mysource);
            Tool.clearArgumentsAdjusters();
            if(diag){IgnoringDiagConsumer* idc = new IgnoringDiagConsumer();
                Tool.setDiagnosticConsumer(idc);}
            Tool.run(FrontendFactory.get());
        }
        
        llvm::errs()<<"Total output snippets: "<<totalOutputSnippet<<"\n";
        
        fclose(out);
    }
    
    if(FindPatternSnippet){
        
        llvm::errs()<<"Find pattern snippets:\n";
        
        bool diag = true;
        fileNum = 0;
        lastName = "";
        totalPatternSnippet = 0;
        
        enum logging_function loggingMethod;
        loggingMethod = KEYWORD_FUNCTION;
        readLoggingFunction(loggingMethod);
        
        FrontendFactory = newFrontendActionFactory<FindPatternAction>();
        
        out = fopen("pattern_snippet.out","w");
        for(unsigned i = 0; i < source.size(); i++){
            vector<string> mysource;
            mysource.push_back(source[i]);
            ClangTool Tool(OptionsParser.getCompilations(), mysource);
            Tool.clearArgumentsAdjusters();
            if(diag){IgnoringDiagConsumer* idc = new IgnoringDiagConsumer();
                Tool.setDiagnosticConsumer(idc);}
            Tool.run(FrontendFactory.get());
        }
        
        llvm::errs()<<"Total pattern snippets: "<<totalPatternSnippet<<"\n";
        
        fclose(out);
    }
    
    
    if(FindLoggedSnippet){
        
        llvm::errs()<<"Find logged snippets:\n";
        
        bool diag = true;
        fileNum = 0;
        lastName = "";
        totalLoggedSnippet = 0;
        
        enum logging_function loggingMethod;
        loggingMethod = LOGGING_FUNCTION;
        readLoggingFunction(loggingMethod);
        
        FrontendFactory = newFrontendActionFactory<FindLoggedAction>();
        
        out = fopen("logged_snippet.out","w");
        for(unsigned i = 0; i < source.size(); i++){
            vector<string> mysource;
            mysource.push_back(source[i]);
            ClangTool Tool(OptionsParser.getCompilations(), mysource);
            Tool.clearArgumentsAdjusters();
            if(diag){IgnoringDiagConsumer* idc = new IgnoringDiagConsumer();
                Tool.setDiagnosticConsumer(idc);}
            Tool.run(FrontendFactory.get());
        }
        
        llvm::errs()<<"Total logged snippets: "<<totalLoggedSnippet<<"\n";
        
        fclose(out);
    }
    
    if(LogTimes){
        
        llvm::errs()<<"Calculate log times:\n";
        
        bool diag = true;
        fileNum = 0;
        lastName = "";
        
        myLogTimeMap.clear();
        myLogTimeCnt = 0;
        
        out = fopen("log_times","w");
        
        FrontendFactory = newFrontendActionFactory<LogTimesAction>();
        for(unsigned i = 0; i < source.size(); i++){
            vector<string> mysource;
            mysource.push_back(source[i]);
            ClangTool Tool(OptionsParser.getCompilations(), mysource);
            Tool.clearArgumentsAdjusters();
            if(diag){IgnoringDiagConsumer* idc = new IgnoringDiagConsumer();
                Tool.setDiagnosticConsumer(idc);}
            Tool.run(FrontendFactory.get());
        }
        
        readLoggedFunction();
        
        for(int i = 0; i < myNamesCnt; i++){
            //outs()<<myFuncNames[i]<<"#";
            //outs()<<myLogNames[i]<<"\n";
            
            int index = myLogTimeMap[myFuncNames[i]];
            if(index != 0){
                myLogTime[index].log_time++;
                myLogTime[index].log_level += getLevel(myLogNames[i]);
            }
            //myLogTimeMap[myFuncNames[i]] = 0;
        }
        
        for(int i = 1; i < myLogTimeCnt; i++){
            myLogTime[i].print();
        }
        //outs()<<"Total function: "<<myLogTimeCnt<<"\n";

        fclose(out);
    }
    
    if(LogScore){

    }

	
	return 0;
}

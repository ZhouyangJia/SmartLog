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
#include "InsertLog.h"

#include "Python.h"


// Shared variables
// count files
string lastName;
int fileNum;
int fileTotalNum;

// Input and output files
// Input
FILE* fCallDep;
// Output
FILE* fCallStmt;
FILE* fLogStmt;
FILE* fLogBehavior;
FILE* fNorBehavior;
FILE* fUnloggedFunc;
FILE* fFuncRuleModel;
// Other
FILE* in;
FILE* out;
FILE* out2;

// python objects
PyObject *pName,*pModule,*pDict,*pFunc,*pResult,*pTmp;


// Record the features of each function
FunctionFeat myFunctionFeat[MAX_FUNC_NUM];
map<string, int> myFunctionFeatMap;
int myFunctionFeatCnt;

// Record the call dependence info
map<string, int>myCallDepMap;

// Record log behavior
vector<LogBehavior> myLogBehavior;

// Record bo which can switch operands
map<BinaryOperator::Opcode, bool> BO_com_map;
// Record bo which can not switch operands
map<BinaryOperator::Opcode, bool> BO_seq_map;
// Record bo which can switch, but semantics changes when switching
map<BinaryOperator::Opcode, bool> BO_swi_map;

// Frequent item mining
map<string, int>funcMap;
map<string, int>funcNameMap;
map<string, int>funcCheckMap;
map<string, int>funcLogMap;
map<string, int>funcApprMap;

// Function rule mining
map<string, int> myLoggedTime;
map<string, int> myCalledTime;


//count results
//int totalLoggingFunction;


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

///Insert logging statement
FuncCondInfo myFuncCondInfo[MAX_FUNC_NUM];
map<string, int> myFuncCondInfoMap;
int myFuncCondInfoCnt;
int totalLogNum;


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

static cl::opt<bool> FindLoggingBehavior("find-logging-behavior",
                                         cl::desc("Find logging behavior."),
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


static cl::opt<bool> InsertLog("insert-log-statement",
                               cl::desc("Insert log statement."),
                               cl::cat(ClangMytoolCategory));

static cl::opt<bool> LogTimes("log-times",
                              cl::desc("Calculate log times of each function."),
                              cl::cat(ClangMytoolCategory));

static cl::opt<bool> LogScore("log-score",
                              cl::desc("Calculate log score."),
                              cl::cat(ClangMytoolCategory));

/// Get the importance level of logging function
/// Input: function name
/// Output: importance level
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

/// Read logged snippet
void readLoggedFunction(){
    
    myNamesCnt = 0;
    
    in = fopen("./logged_snippet.out", "r");
    
    if(in == NULL) {
        errs()<<"Miss file \"logged_snippet.out\".\n";
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

/// Read logging function
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

/// Read data flow feature got from IR, used for choose logging funtions
void readFunctionFlow(){
    
    if((in = fopen("./dep_on_para.csv","r")) == NULL) {
        errs()<<"Miss file \"dep_on_para.csv\".\n";
    }
    else {
        char buf[300];
        while(!feof(in)) {
            buf[0] = '\0';
            fgets(buf, 300, in);
            if(strlen(buf) <= 1)
                break;
            
            string line = buf;
            StringRef file;
            file = line.substr(0, line.find_first_of(','));
            line = line.substr(line.find_first_of(',') + 1);
            
            StringRef caller;
            caller = line.substr(0, line.find_first_of(','));
            line = line.substr(line.find_first_of(',') + 1);
            
            StringRef callee;
            callee = line.substr(0, line.find_first_of(','));
            
            //errs()<<file<<" "<<caller<<" "<<callee<<"\n";
            
            int callerIndex = myFunctionFeatMap[caller];
            int calleeIndex = myFunctionFeatMap[callee];
            if(myFunctionFeat[callerIndex].fileName.compare(file)){
                continue;
            }
            myFunctionFeat[calleeIndex].flowID[callerIndex] = 1;
            
        }
        fclose(in);
    }
}

/// Read call dependence got from IR
void readCallDep(){
    
    if((fCallDep = fopen("./call_dependence.csv","r")) == NULL) {
        errs()<<"Miss file \"call_dependence.csv\".\n";
    }
    else {
        char buf[300];
        while(!feof(fCallDep)) {
            buf[0] = '\0';
            fgets(buf, 300, fCallDep);
            if(strlen(buf) <= 1)
                break;
            
            string line = buf;
            line = line.substr(0, line.find_last_of(','));
            
            string args = line.substr(line.find_last_of(',') + 1);
            string key = line.substr(0, line.find_last_of(','));
            
            int arg = atoi(args.c_str());
            myCallDepMap[key] = arg + 1;
            //errs()<<key<<" "<<arg<<" "<<myCallDepMap[key]<<"\n";
            
        }
        fclose(fCallDep);
    }
}


void readUnloggedFunc(){
    
    if((fUnloggedFunc = fopen("./unlogged_func.out","r")) == NULL) {
        errs()<<"Miss file \"unlogged_func.out\".\n";
    }
    else {
        char buf[300];
        while(!feof(fUnloggedFunc)) {
            buf[0] = '\0';
            fgets(buf, 300, fUnloggedFunc);
            if(strlen(buf) <= 1)
                break;
            
            string funcName = buf;
            funcName = funcName.substr(0, funcName.find_last_of('\n'));
            
            if(myFuncCondInfoMap[funcName] == 0){
                myFuncCondInfoMap[funcName] = ++myFuncCondInfoCnt;
            }
            int index = myFuncCondInfoMap[funcName];
            if(index == MAX_FUNC_NUM){
                errs()<<"Too many conditions!\n";
                exit(1);
            }
            
            myFuncCondInfo[index].setFuncName(funcName);
        }
        fclose(fUnloggedFunc);
    }
}

/// Program entrance
int main(int argc, const char **argv) {
    
    CommonOptionsParser OptionsParser(argc, argv, ClangMytoolCategory);
    vector<string> source = OptionsParser.getSourcePathList();
    fileTotalNum = source.size();
    
    std::unique_ptr<FrontendActionFactory> FrontendFactory;
    
    if(FindLoggingBehavior){
        
        // Initialize python module
        Py_Initialize();
        if(!Py_IsInitialized()){
            return 0;
        }
        PyRun_SimpleString("import sys");
        PyRun_SimpleString("sys.path.append('../../script/')");
        pName = PyString_FromString("segment");
        pModule = PyImport_Import(pName);
        if(!pModule){
            printf("can't find segment.py");
            return 0;
        }
        pDict = PyModule_GetDict(pModule);
        if(!pDict){
            printf("can't find dict");
            return 0;
        }
        pFunc = PyDict_GetItemString(pDict, "segment");
        if(!pFunc || !PyCallable_Check(pFunc)){
            printf("can't find function [segment]");
            return 0;
        }
        
        
        // Init shared variables
        bool diag = true;
        fileNum = 0;
        lastName = "";
        
        myFunctionFeatCnt = 0;
        myFunctionFeatMap.clear();
        myCallDepMap.clear();
        funcMap.clear();
        
        BO_com_map.clear();
        BO_swi_map.clear();
        BO_seq_map.clear();
        BO_com_map[BO_Mul] = true; //*
        BO_com_map[BO_Add] = true; //+
        BO_com_map[BO_And] = true; //&
        BO_com_map[BO_Xor] = true; //^
        BO_com_map[BO_Or] = true; //|
        BO_com_map[BO_Comma] = true; //,
        BO_com_map[BO_EQ] = true; //==
        BO_com_map[BO_NE] = true; //!=
        BO_seq_map[BO_Div] = true; ///
        BO_seq_map[BO_Rem] = true; //%
        BO_seq_map[BO_Sub] = true; //-
        BO_seq_map[BO_Shl] = true; //<<
        BO_seq_map[BO_Shr] = true; //>>
        BO_seq_map[BO_MulAssign] = true; //*=
        BO_seq_map[BO_DivAssign] = true; //\=
        BO_seq_map[BO_RemAssign] = true; //%=
        BO_seq_map[BO_AddAssign] = true; //+=
        BO_seq_map[BO_SubAssign] = true; //-=
        BO_seq_map[BO_ShlAssign] = true; //<<=
        BO_seq_map[BO_ShrAssign] = true; //>>=
        BO_seq_map[BO_AndAssign] = true; //&=
        BO_seq_map[BO_XorAssign] = true; //^=
        BO_seq_map[BO_OrAssign] = true; //|=
        BO_swi_map[BO_LT] = true; //<
        BO_swi_map[BO_GT] = true; //>
        BO_swi_map[BO_LE] = true; //<=
        BO_swi_map[BO_GE] = true; //>=
        
        // Read call dependence info
        readCallDep();
        
        fCallStmt = fopen("call_statement.out", "w");
        fLogStmt = fopen("logging_statement.out","w");
        fLogBehavior = fopen("logging_behavior.out","w");
        fNorBehavior = fopen("normalized_behavior.out","w");
        
        // Run find logging behaivor action
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
        
        // Run insert log action
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
        
        // Frequent item mining
        funcNameMap.clear();
        funcCheckMap.clear();
        funcLogMap.clear();
        funcApprMap.clear();
        unsigned num = myLogBehavior.size();
        for(unsigned i = 0; i < num; i++){
            char arg[10];
            sprintf(arg, "%d", myLogBehavior[i].argIndex);
            
            string funcNameStr = "";
            string funcCheckStr = "";
            string funcLogStr = "";
            string funcApprStr = "";
            
            funcNameStr = funcNameStr.append(myLogBehavior[i].callName);
            funcCheckStr = funcCheckStr.append(myLogBehavior[i].callName).append("#").append(arg).append("#");
            funcCheckStr = funcCheckStr.append(myLogBehavior[i].argType).append("#").append(myLogBehavior[i].regularText);
            funcLogStr = funcLogStr.append(myLogBehavior[i].callName).append("#").append(arg).append("#");
            funcLogStr = funcLogStr.append(myLogBehavior[i].argType).append("#").append(myLogBehavior[i].regularText);
            funcLogStr = funcLogStr.append("#").append(myLogBehavior[i].logType);
            funcApprStr = funcNameStr;
            funcApprStr = funcApprStr.append("#").append(myLogBehavior[i].logType);
            
            funcNameMap[funcNameStr]++;
            funcCheckMap[funcCheckStr]++;
            funcLogMap[funcLogStr]++;
            funcApprMap[funcApprStr]++;
            
            fputs(funcLogStr.c_str(), fNorBehavior);
            fputs("\n", fNorBehavior);
        }
        map<string,int>::iterator it;
        for(it = funcCheckMap.begin(); it != funcCheckMap.end(); ++it){
            
            string check = it->first;
            string name = check.substr(0, check.find_first_of('#'));
            
            string checkLL = check, checkRL = check, checkNL = check;
            string nameLL = name, nameRL = name, nameNL = name;
            checkLL = checkLL.append("#LL");
            checkRL = checkRL.append("#RL");
            checkNL = checkNL.append("#NL");
            nameLL = nameLL.append("#LL");
            nameRL = nameRL.append("#RL");
            nameNL = nameNL.append("#NL");
            
            int nameCnt = funcNameMap[name];
            
            //int cntMax = funcMap[name]>nameCnt?funcMap[name]:nameCnt;
            int cntMax = nameCnt;
            
            int checkCntLL = funcLogMap[checkLL];
            int nameCntLL = funcApprMap[nameLL];
            float suppLL = (float)checkCntLL/(float)cntMax;
            float confLL = (float)checkCntLL/(float)(it->second);
            float meanLL = 2/(1/suppLL + 1/confLL);
            float corrLL = 0;
            if(nameCntLL != 0)
                corrLL = (float)(checkCntLL*nameCnt)/(float)((it->second)*nameCntLL); // (ab/n)/(a/n*b/n) = (ab*n)/(a*b)
            
            int checkCntRL = funcLogMap[checkRL];
            int nameCntRL = funcApprMap[nameRL];
            float suppRL = (float)checkCntRL/(float)cntMax;
            float confRL = (float)checkCntRL/(float)(it->second);
            float meanRL = 2/(1/suppRL + 1/confRL);
            float corrRL = 0;
            if(nameCntRL != 0)
                corrRL = (float)(checkCntRL*nameCnt)/(float)((it->second)*nameCntRL); // (ab/n)/(a/n*b/n) = (ab*n)/(a*b)
            
            int checkCntNL = funcLogMap[checkNL];
            int nameCntNL = funcApprMap[nameNL];
            float suppNL = (float)checkCntNL/(float)cntMax;
            float confNL = (float)checkCntNL/(float)(it->second);
            float meanNL = 2/(1/suppNL + 1/confNL);
            float corrNL = 0;
            if(nameCntNL != 0)
                corrNL = (float)(checkCntNL*nameCnt)/(float)((it->second)*nameCntNL); // (ab/n)/(a/n*b/n) = (ab*n)/(a*b)
            
            outs()<<name<<";"<<funcMap[name]<<";"<<nameCnt<<";";
            outs()<<it->second<<";";
            outs()<<checkCntLL<<";"<<suppLL<<";"<<confLL<<";"<<meanLL<<";"<<corrLL<<";";
            outs()<<checkCntRL<<";"<<suppRL<<";"<<confRL<<";"<<meanRL<<";"<<corrRL<<";";
            outs()<<checkCntNL<<";"<<suppNL<<";"<<confNL<<";"<<meanNL<<";"<<corrNL<<";";
            
            // Ignore "\n"
            string::size_type pos = 0;
            while ( (pos = check.find('\n', pos)) != string::npos ) {
                check.replace( pos, 1, " ");
                pos++;
            }
            outs()<<check<<"\n";
        }
        
        // Clear
        fclose(fCallStmt);
        fclose(fLogStmt);
        fclose(fLogBehavior);
        fclose(fNorBehavior);
        
        // Finalize python module
        Py_DECREF(pName);
        Py_DECREF(pModule);
        Py_DECREF(pDict);
        Py_DECREF(pFunc);
        if(pResult){
            Py_DECREF(pResult);
            Py_DECREF(pTmp);
        }
        Py_Finalize();
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
        
        //llvm::errs()<<"Find function rule model instances:\n";
        
        bool diag = true;
        fileNum = 0;
        lastName = "";
        totalLoggedSnippet = 0;
        myLoggedTime.clear();
        myCalledTime.clear();
        
        FrontendFactory = newFrontendActionFactory<FindLoggedAction>();
        
        fFuncRuleModel = fopen("function_rule_model.out","w");
        for(unsigned i = 0; i < source.size(); i++){
            vector<string> mysource;
            mysource.push_back(source[i]);
            ClangTool Tool(OptionsParser.getCompilations(), mysource);
            Tool.clearArgumentsAdjusters();
            if(diag){IgnoringDiagConsumer* idc = new IgnoringDiagConsumer();
                Tool.setDiagnosticConsumer(idc);}
            Tool.run(FrontendFactory.get());
        }
        
        map<string,int>::iterator it;
        for(it = myCalledTime.begin(); it != myCalledTime.end(); ++it){
            
            string name = it->first;
            if(name == "")
                continue;
            int call = it->second;
            int log = myLoggedTime[name];
            char arg[100];
            sprintf(arg, "%s,%d,%d\n", name.c_str(), call, log);
            fputs(arg,fFuncRuleModel);
            
        }
        
        //llvm::errs()<<"Total function rule model instances: "<<totalLoggedSnippet<<"\n";
        
        fclose(fFuncRuleModel);
    }
    
    if(InsertLog){
        bool diag = true;
        
        myFuncCondInfoMap.clear();
        myFuncCondInfoCnt = 0;
        totalLogNum = 0;
        readUnloggedFunc();
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
        for(int i = 1; i < myFuncCondInfoCnt; i++){
            //myFuncCondInfo[i].print();
            errs()<<myFuncCondInfo[i].funcName<<"\n";
        }
        llvm::errs()<<"Total "<<totalLogNum<<" functions are logged!\n";
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

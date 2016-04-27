//
//  MyTool.cpp
//  LLVM
//
//  Created by ZhouyangJia on 16/4/13.
//
//

#include "MyTool.h"
#include "FindLoggingFunction.h"
#include "FindLoggedSnippet.h"
#include "FindOutputSnippet.h"
#include "FindPatternSnippet.h"

//record the called-time of each function
FunctionFeat myCalledFeat[MAX_FUNC_NUM];
map<string, int> myCalledFeatMap;
int myCalledFeatCnt;

string lastName;
int fileNum;
int fileTotalNum;

string logNames[MAX_LOG_FUNC];
int logNamesCnt;

FILE* in;
FILE* out;

int totalLoggingFunction;
int totalLoggedSnippet;
int totalOutputSnippet;
int totalPatternSnippet;


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

static cl::opt<bool> LogScore("log-score",
                             cl::desc("Calculate log score."),
                             cl::cat(ClangMytoolCategory));

void readLoggingFunction(){
    
    logNamesCnt = 0;
    
    if((in = fopen("./logging_function.out","r")) == NULL) {
        errs()<<"Miss the need log function file \"logging_function.out\".\n";
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


void readFunctionFlow(){
    
    if((in = fopen("./function_flow.out","r")) == NULL) {
        errs()<<"Miss the need log function file \"function_flow.out\".\n";
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
    
    if(FindOutputSnippet){
        
        llvm::errs()<<"Find output snippets:\n";
        
        bool diag = true;
        fileNum = 0;
        lastName = "";
        totalOutputSnippet = 0;
        
        readLoggingFunction();
        
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
        
        readLoggingFunction();
        
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
        
        readLoggingFunction();
        
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
    
    if(LogScore){

    }

	
	return 0;
}

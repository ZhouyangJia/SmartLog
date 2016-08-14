#ifndef INSERTLOG_H
#define INSERTLOG_H

#include <map>
#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/Option/OptTable.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Signals.h"
#include "clang/Analysis/CFG.h"
#include "clang/Analysis/CallGraph.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Driver/Options.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/FrontendActions.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "clang/StaticAnalyzer/Frontend/FrontendActions.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"

using namespace std;

using namespace clang::driver;
using namespace clang::tooling;
using namespace clang;
using namespace llvm;
using namespace llvm::opt;

class FuncCondInfo{
public:
    FuncCondInfo(){
        string funcName = "";
        condcnt = 0;
        condTime = 0;
        totalTime = 0;
    };
    
    string funcName;
    string funcCond[100];
    string errormsg[100];
    int condTime;
    int totalTime;
    int isChosen[100];
    int condcnt;
    
    
    
    void setFuncName(string);
    void addCond(string, int, int, string);
    
    void print();
};

class InsertLogVisitor : public RecursiveASTVisitor <InsertLogVisitor> {
public:
	explicit InsertLogVisitor(CompilerInstance* CI, StringRef InFile, Rewriter rewriter) : CI(CI), InFile(InFile), rewriter(rewriter){};
	
	bool VisitFunctionDecl (FunctionDecl*);
	
	void travelStmt(Stmt*, Stmt*);	
//private:
	CompilerInstance* CI;
	StringRef InFile;
	Rewriter rewriter;
	
};

class InsertLogConsumer : public ASTConsumer {
public:
	explicit InsertLogConsumer(CompilerInstance* CI, StringRef InFile, Rewriter rewriter) : Visitor(CI, InFile, rewriter){}
	virtual void HandleTranslationUnit (clang::ASTContext &Context);
private: 
	InsertLogVisitor Visitor;
	StringRef InFile;
};


class InsertLogAction : public ASTFrontendAction {
public:
	virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &Compiler, StringRef InFile);
private:
};


#endif

#ifndef RECORDCOND_H
#define RECORDCOND_H

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

#include "FuncLogInfo.h"

using namespace std;

using namespace clang::driver;
using namespace clang::tooling;
using namespace clang;
using namespace llvm;
using namespace llvm::opt;


#define GET_SPELLING_LOC 1
#define GET_EXPANSION_LOC 2


class RecordCondVisitor : public RecursiveASTVisitor <RecordCondVisitor> {
public:
	explicit RecordCondVisitor(CompilerInstance* CI, StringRef InFile) : CI(CI), InFile(InFile){
		retName = "";
		funcName = "";
		condcnt = 0;
	};
	bool VisitFunctionDecl (FunctionDecl*);
	
	/*utility methods*/
	StringRef expr2str(Expr*, int mode = GET_SPELLING_LOC);
	string getfile(string);
	//int inStmt(int, Stmt*);
	
	/*elements about logged function*/
	bool findLoggedFunction(Stmt* stmt);
	string retName;
	string funcName;
	int funcLocLine;
	QualType retType;
	
	/*elements about loggign condition*/
	void searchLoggingBlock(CFGBlock* block, int level);
	void analyzeCond(Expr* expr, Condition* cond);
	string getDeeperName(Stmt* stmt);
	void addCondition(int index, int i); //add condition for index(th) function, i(th) log 
	Condition* conditions[5];
	Expr* condexpr[5];
	bool iselse[5];
	int condcnt;
	
	
	/*elements about logged argument*/
	void addArgument(int index, int logID); //add argument for index(th) function, i(th) log 
	void analyzeArgu(Argument* a, Stmt* s, int i); //analyze i(th) argument s, and assign to a
	CallExpr* funcCall;
	
	/*elements about all callexpr argument*/ 
	void travelStmtForCall(Stmt* stmt);
	void addCallArg(int index, CallExpr* call); //add all call argument for index(th) function
	
	CompilerInstance* CI;
	StringRef InFile;
	
};

class RecordCondConsumer : public ASTConsumer {
public:
	explicit RecordCondConsumer(CompilerInstance* CI, StringRef InFile) : Visitor(CI, InFile){}
	virtual void HandleTranslationUnit (clang::ASTContext &Context);
private: 
	RecordCondVisitor Visitor;
	StringRef InFile;
};

class RecordCondAction : public ASTFrontendAction {
public:
	virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &Compiler, StringRef InFile);
private:
};


#endif

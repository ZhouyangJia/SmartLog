//
//  FindLoggingFunction.h
//  LLVM
//
//  Created by ZhouyangJia on 16/4/13.
//
//

#ifndef FindLoggingFunction_h
#define FindLoggingFunction_h

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
#include "clang/AST/ParentMap.h"
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


struct FunctionFeat{
    
    string funcName;
    string fileName;
    bool funcKeyWord;
    
	int calledNumber;
    int normalNumber;
    int errorNumber;
	
	int lenth;
    set<int> callerID;
    map<int, bool> flowID;
    
	void print();
};

struct Node{
    struct Node* parent;
    struct Node* childl;
    struct Node* childr;
    
    BinaryOperator* BO;
    
    string textl;
    string textr;
    
    bool LNot;
    bool changeOp;
    
};

struct LogBehavior{
    
    string callName;
    string callText;
    string callLoc;
    CallExpr* callExpr;
    string retName;
    
    int argIndex;
    string argType;
    
    string ifText;
    string regularText;
    string ifLoc;
    int ifBranch;
    Stmt* stmt;
    
    string logText;
    string logType;
    string logLoc;
    
    void print();
    
    struct Node* root;
    vector<BinaryOperator*>BOList;
    
    CompilerInstance* CI;
    
    void regular(CompilerInstance*);
    void nodeCollect(Stmt*);
    void nodeRegular(Node*);
    string getNodeText(Node*);
    void nodeFree(Node*);
    
    bool getAtMost2BO(Stmt*);
    Expr* expr;
    BinaryOperator* bo1;
    BinaryOperator* bo2;
    string getExprText(bool);
    string getLeafText(Expr*);
    string getExprRegular(string,string,BinaryOperator::Opcode);
    
    string getText(Stmt*);
};

class FindLoggingVisitor : public RecursiveASTVisitor <FindLoggingVisitor> {
    
public:
	explicit FindLoggingVisitor(CompilerInstance* CI, StringRef InFile) : CI(CI), InFile(InFile){};
    bool VisitFunctionDecl (FunctionDecl*);
	void travelStmt(Stmt*,Stmt*);
    
    
    bool hasKeyword(string);
    bool spiltWord(string);
    bool equalKeyword(string);
    
    bool searchKeyword(Stmt*);
    bool hasErrorKeyword(string);
    bool hasExitKeyword(string);
    bool hasLimitationKeyword(string);
    bool hasPrintKeyword(string);
    bool hasNonCorrectKeyword(string);
    

    bool hasChar(QualType);
    string expr2str(Stmt*);
    StringRef getStemName(StringRef);
    
    bool isLoggingStatement(CallExpr*, Stmt*);
    
    void extractLoggingSnippet(FunctionDecl*);
    bool hasReturnStmt(Stmt*);
    bool hasLogStmt(Stmt*);
    bool hasCallExpr(Stmt*);
    Stmt* getNextNode(Stmt*, Stmt*);
	
private:
	CompilerInstance* CI;
    StringRef InFile;
    int callerID;
    
    // isInIfCond
    bool isInIfCond;
    
    // Call info and branch info
    vector<CallExpr*> callList;
    map<CallExpr*, bool> logListMap;
    vector<IfStmt*> ifList;
    vector<SwitchStmt*> switchList;
    
    // Remote log and Local log
    ReturnStmt* myReturnStmt;
    CallExpr* myLogStmt;
    
};

class FindLoggingConsumer : public ASTConsumer {
public:
	explicit FindLoggingConsumer(CompilerInstance* CI, StringRef InFile) : Visitor(CI, InFile){}
	virtual void HandleTranslationUnit (clang::ASTContext &Context);
private:
	FindLoggingVisitor Visitor;
	StringRef InFile;
};

class FindLoggingAction : public ASTFrontendAction {
public:
	virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &Compiler, StringRef InFile);
private:
};

#endif /* FindLoggedSnippet_h */

//
//  FindLoggingFunction.hpp
//  LLVM
//
//  Created by ZhouyangJia on 16/4/13.
//
//

#ifndef FindLoggingFunction_hpp
#define FindLoggingFunction_hpp

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


struct FunctionFeat{
	int calledNumber;
	int fileNumber;
	
	string funcName;
	bool funcKeyWord;
	
	string fileName;
	bool fileKeyWord;
	
	int lenth;
	bool flow;
    
    bool decl;
    bool haschar;
    
    int logNumber;
    
	int label;
    
    string source;
    
	void print();
};


class FindLoggingVisitor : public RecursiveASTVisitor <FindLoggingVisitor> {
public:
	explicit FindLoggingVisitor(CompilerInstance* CI, StringRef InFile) : CI(CI), InFile(InFile){};
	
	bool VisitFunctionDecl (FunctionDecl*);
	
	void travelStmt(Stmt*);	
	
    string expr2str(Stmt*);
	string getfile(string);
	bool hasKeyWord(string);
    bool hasChar(QualType);
	
private:
	CompilerInstance* CI;
	StringRef InFile;
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

#endif /* FindLoggedSnippet_hpp */

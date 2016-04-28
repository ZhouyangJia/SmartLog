//
//  FindOutputSnippet.h
//  LLVM
//
//  Created by ZhouyangJia on 16/4/26.
//
//

#ifndef FindOutputSnippet_h
#define FindOutputSnippet_h

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


class FindOutputVisitor : public RecursiveASTVisitor <FindOutputVisitor> {
public:
    explicit FindOutputVisitor(CompilerInstance* CI, StringRef InFile) : CI(CI), InFile(InFile){};
    
    bool VisitFunctionDecl (FunctionDecl*);
    
    void travelStmt(Stmt*, Stmt*);
    
    void searchLog(CallExpr*, Stmt*);
    CallExpr* searchCall(Stmt*);
    
    void recordCallLog(CallExpr*, CallExpr*);
    
    StringRef expr2str(Stmt*);
    
    void readLogFunction();
    
    map<CallExpr*, int> hasRecorded;
    
    CompilerInstance* CI;
    StringRef InFile;
    
};

class FindOutputConsumer : public ASTConsumer {
public:
    explicit FindOutputConsumer(CompilerInstance* CI, StringRef InFile) : Visitor(CI, InFile){}
    virtual void HandleTranslationUnit (clang::ASTContext &Context);
private:
    FindOutputVisitor Visitor;
    StringRef InFile;
};

class FindOutputAction : public ASTFrontendAction {
public:
    virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &Compiler, StringRef InFile);
private:
};

#endif /* FindOutputSnippet_h */




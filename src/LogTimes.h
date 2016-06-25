//
//  LogTimes.h
//  LLVM
//
//  Created by ZhouyangJia on 16/5/11.
//
//

#ifndef LogTimes_h
#define LogTimes_h

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

struct LogTime{
    
    LogTime(){
        name = "";
        call_time = 0;
        log_time = 0;
        log_level = 0;
    }
    
    string name;
    int call_time;
    int log_time;
    int log_level;
    
    void print();
};

class LogTimesVisitor : public RecursiveASTVisitor <LogTimesVisitor> {
public:
    explicit LogTimesVisitor(CompilerInstance* CI, StringRef InFile) : CI(CI), InFile(InFile){};
    
    bool VisitFunctionDecl (FunctionDecl*);
    
    void travelStmt(Stmt*);
    
    string expr2str(Stmt*);
    
private:
    CompilerInstance* CI;
    StringRef InFile;
};

class LogTimesConsumer : public ASTConsumer {
public:
    explicit LogTimesConsumer(CompilerInstance* CI, StringRef InFile) : Visitor(CI, InFile){}
    virtual void HandleTranslationUnit (clang::ASTContext &Context);
private:
    LogTimesVisitor Visitor;
    StringRef InFile;
};

class LogTimesAction : public ASTFrontendAction {
public:
    virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &Compiler, StringRef InFile);
private:
};


#endif /* LogTimes_h */

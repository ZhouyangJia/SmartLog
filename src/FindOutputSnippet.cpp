//
//  FindOutputSnippet.cpp
//  LLVM
//
//  Created by ZhouyangJia on 16/4/26.
//
//

#include "SmartLog.h"
#include "FindOutputSnippet.h"
#include "FindLoggingFunction.h"

extern string lastName;
extern int fileTotalNum;
extern int fileNum;

extern string logNames[MAX_LOG_FUNC];
extern int logNamesCnt;

extern int totalOutputSnippet;

extern FILE* in;
extern FILE* out;


///record and output call-log pair
void FindOutputVisitor::recordCallLog(CallExpr *callExpr, CallExpr *logExpr){
    
    if(!callExpr || !logExpr)
        return;
    
    if(callExpr == logExpr)
        return;
    
    if(!callExpr->getDirectCallee() || !logExpr->getDirectCallee())
        return;
    
    if(hasRecorded[logExpr] == 0)
        hasRecorded[logExpr] = 1;
    else
        return;
    
    SourceLocation callLocation = callExpr->getLocStart();
    SourceLocation logLocation = logExpr->getLocStart();
    
    if(callLocation.isValid() && logLocation.isValid()){
        
//        llvm::outs()<<callExpr->getDirectCallee()->getQualifiedNameAsString()<<"@"<<callLocation.printToString(CI->getSourceManager());
//        llvm::outs()<<"#";
//        llvm::outs()<<logExpr->getDirectCallee()->getQualifiedNameAsString()<<"@"<<logLocation.printToString(CI->getSourceManager());
//        llvm::outs()<<"\n";
        
        fputs(callExpr->getDirectCallee()->getQualifiedNameAsString().c_str(), out);
        fputs("@", out);
        fputs(callLocation.printToString(CI->getSourceManager()).c_str(), out);
        fputs("#", out);
        fputs(logExpr->getDirectCallee()->getQualifiedNameAsString().c_str(), out);
        fputs("@", out);
        fputs(logLocation.printToString(CI->getSourceManager()).c_str(), out);
        fputs("\n", out);
        
        totalOutputSnippet++;
    }
}


///get the source code of stmt
StringRef FindOutputVisitor::expr2str(Stmt *s){
    
    if(!s)
        return "";
    
    FullSourceLoc begin = CI->getASTContext().getFullLoc(s->getLocStart());
    FullSourceLoc end = CI->getASTContext().getFullLoc(s->getLocEnd());
    
    if(begin.isInvalid() || end.isInvalid())
        return "";
    
    SourceRange sr(begin.getExpansionLoc(), end.getExpansionLoc());
    
    return Lexer::getSourceText(CharSourceRange::getTokenRange(sr), CI->getSourceManager(), LangOptions(), 0);
}


///search log site in the subtree of stmt
void FindOutputVisitor::searchLog(CallExpr * callExpr, Stmt *stmt){
    
    if(!stmt)
        return;
    
    if(CallExpr *logExpr = dyn_cast<CallExpr>(stmt)){
        if(FunctionDecl *functionDecl = logExpr->getDirectCallee()){
            StringRef logName = functionDecl->getQualifiedNameAsString();
            
            for(int i = 0; i < logNamesCnt-1; i++){
                if(logNames[i].find(logName) != string::npos){
                    recordCallLog(callExpr, logExpr);
                    break;
                    //return;
                }
            }
        }
    }
    
    for(Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
        if(Stmt *child = *it){
            searchLog(callExpr, child);
            //if(ret)
                //return ret;
        }
    }
    
    return;
}


///search call site in the subtree of stmt
CallExpr* FindOutputVisitor::searchCall(Stmt *stmt){
    
    if(!stmt)
        return 0;
    
    if(CallExpr *callExpr = dyn_cast<CallExpr>(stmt)){
        if(callExpr->getDirectCallee()){
            return callExpr;
        }
    }
    
    for(Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
        if(Stmt *child = *it){
            CallExpr *ret = searchCall(child);
            if(ret)
                return ret;
        }
    }
    
    return 0;
}


///search the function body for call-log pairs
void FindOutputVisitor::travelStmt(Stmt *stmt, Stmt *father){
    
    if(!stmt || !father)
        return;

    ///find if(foo()), stmt is located in the condexpr of ifstmt
    if(IfStmt *ifStmt = dyn_cast<IfStmt>(father)){
        if(ifStmt->getCond() == stmt){
            if(CallExpr *callExpr = searchCall(stmt)){
                searchLog(callExpr, father);
            }
        }
    }
    ///find switch(foo()), stmt is located in the condexpr of switchstmt
    if(SwitchStmt *switchStmt = dyn_cast<SwitchStmt>(father)){
        if(switchStmt->getCond() == stmt){
            if(CallExpr *callExpr = searchCall(stmt)){
                searchLog(callExpr, father);
            }
        }
    }
    ///find 'ret = foo()', when stmt = 'BinaryOperator', op == '=' and RHS == 'callexpr'
    if(BinaryOperator *binaryOperator = dyn_cast<BinaryOperator>(stmt)){
        if(binaryOperator->getOpcode() == BO_Assign){
            if(Expr *expr = binaryOperator->getRHS()){
                expr = expr->IgnoreImpCasts();
                expr = expr->IgnoreImplicit();
                expr = expr->IgnoreParens();
                if(CallExpr *callExpr = dyn_cast<CallExpr>(expr)){
                    for(Stmt::child_iterator bro = father->child_begin(); bro != father->child_end(); ++bro){
                        if(Stmt *brother = *bro){
                            searchLog(callExpr, brother);
                        }
                    }
                }
            }
        }
    }
    
    ///find foo(), search its brother node
    if(CallExpr *callExpr = dyn_cast<CallExpr>(stmt)){
        for(Stmt::child_iterator bro = father->child_begin(); bro != father->child_end(); ++bro){
            if(Stmt *brother = *bro){
                searchLog(callExpr, brother);
            }
        }
    }
    
    for(Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
        if(Stmt *child = *it)
            travelStmt(child, stmt);
    }
    
    return;
}


bool FindOutputVisitor::VisitFunctionDecl (FunctionDecl* Declaration){
    
    if(!Declaration)
        return true;
    
    if(!(Declaration->isThisDeclarationADefinition() && Declaration->hasBody()))
        return true;
    
    FullSourceLoc FullLocation = CI->getASTContext().getFullLoc(Declaration->getLocStart());
    
    if(!FullLocation.isValid())
        return true;
    
    ///the function is located in head file
    if(FullLocation.getExpansionLoc().getFileID() != CI->getSourceManager().getMainFileID())
        return true;
    
    //llvm::errs()<<"Found function "<<Declaration->getQualifiedNameAsString() ;
    //llvm::errs()<<" @ " << FullLocation.printToString(FullLocation.getManager()) <<"\n";
    
    hasRecorded.clear();
    
    if(Stmt* function = Declaration->getBody()){
        travelStmt(function, function);
    }
    
    return true;
}


void FindOutputConsumer::HandleTranslationUnit(ASTContext& Context) {
    
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    return;
}


std::unique_ptr<clang::ASTConsumer> FindOutputAction::CreateASTConsumer(CompilerInstance& Compiler, StringRef InFile){

    if(InFile == lastName)
        return 0;
    else{
        llvm::errs()<<"["<<++fileNum<<"/"<<fileTotalNum<<"]"<<" Now analyze the file: "<<InFile<<"\n";
        lastName = InFile;
        Rewriter rewriter;
        rewriter.setSourceMgr(Compiler.getSourceManager(), Compiler.getLangOpts());
        return std::unique_ptr<clang::ASTConsumer>(new FindOutputConsumer(&Compiler, InFile));
    }
}

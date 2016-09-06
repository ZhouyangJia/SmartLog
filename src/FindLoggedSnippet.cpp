//
//  FindLoggedSnippet.cpp
//  LLVM
//
//  Created by ZhouyangJia on 16/4/13.
//
//

#include "SmartLog.h"
#include "FindLoggedSnippet.h"
#include "FindLoggingFunction.h"

extern string lastName;
extern int fileTotalNum;
extern int fileNum;


extern int totalLoggedSnippet;


extern map<string, int> myLoggedTime;
extern map<string, int> myCalledTime;

extern FILE* fFuncRuleModel;


///record and output call-log pair
void FindLoggedVisitor::recordCallLog(CallExpr *callExpr, CallExpr *logExpr){
    
    return;
    
    if(!callExpr || !logExpr)
        return;
    
    if(!callExpr->getDirectCallee() || !logExpr->getDirectCallee())
        return;
    
    if(hasRecorded[callExpr] == 0)
        hasRecorded[callExpr] = 1;
    else
        return;
    
    SourceLocation callLocation = callExpr->getLocStart();
    SourceLocation logLocation = logExpr->getLocStart();
    
    if(callLocation.isValid() && logLocation.isValid()){
        
//        llvm::outs()<<callExpr->getDirectCallee()->getQualifiedNameAsString()<<"@"<<callLocation.printToString(CI->getSourceManager());
//        llvm::outs()<<"#";
//        llvm::outs()<<logExpr->getDirectCallee()->getQualifiedNameAsString()<<"@"<<logLocation.printToString(CI->getSourceManager());
//        llvm::outs()<<"\n";
        
        fputs(callExpr->getDirectCallee()->getQualifiedNameAsString().c_str(), fFuncRuleModel);
        fputs("@", fFuncRuleModel);
        fputs(callLocation.printToString(CI->getSourceManager()).c_str(), fFuncRuleModel);
        fputs("#", fFuncRuleModel);
        fputs(logExpr->getDirectCallee()->getQualifiedNameAsString().c_str(), fFuncRuleModel);
        fputs("@", fFuncRuleModel);
        fputs(logLocation.printToString(CI->getSourceManager()).c_str(), fFuncRuleModel);
        fputs("\n", fFuncRuleModel);
        
        totalLoggedSnippet++;
    }
}


///get the source code of stmt
StringRef FindLoggedVisitor::expr2str(Stmt *s){
    
    if(!s)
        return "";
    
    FullSourceLoc begin = CI->getASTContext().getFullLoc(s->getLocStart());
    FullSourceLoc end = CI->getASTContext().getFullLoc(s->getLocEnd());
    
    if(begin.isInvalid() || end.isInvalid())
        return "";
    
    SourceRange sr(begin.getExpansionLoc(), end.getExpansionLoc());
    
    return Lexer::getSourceText(CharSourceRange::getTokenRange(sr), CI->getSourceManager(), LangOptions(), 0);
}


bool FindLoggedVisitor::hasKeyWord(string name){
    
    transform(name.begin(), name.end(), name.begin(), ::tolower);  //::toupper
    
    //string* spilted;
    //spilted = spiltWord(name);
    
    string word[30] = {
        "log",
        "error",
        "err",
        "die",
        "fail",
        
        "hint",
        "put",
        "assert",
        "trace",
        "print",
        
        "write",
        "report",
        "record",
        "dump",
        "msg",
        
        "message",
        "out",
        "warn",
        "debug"
        "emerg"
        
        "alert"
        "crit"
    };
    
    /*string nword[30] = {
     "binlog",
     "logic",
     "parse",
     "input",
     "sprint",
     "snprint",
     "pullout",
     "compute",
     "timeout",
     "routine",
     "__errno_location",
     "wcserror",
     "strerror",
     "putc",
     "layout"
     };*/
    
    /*for(int i = 0; i < 30; i++){
     if(nword[i].length() == 0)
     break;
     if(name.find(nword[i]) < name.length() && (name.find_last_of(':') == string::npos || name.find(nword[i]) > name.find_last_of(':')))
     return false;
     }*/
    
    for(int i = 0; i < 30; i++){
        if(word[i].length() == 0)
            break;
        if(name.find(word[i]) < name.length() && (name.find_last_of(':') == string::npos || name.find(word[i]) > name.find_last_of(':')))
            return true;
    }
    
    return false;
}



///search log site in the subtree of stmt
CallExpr* FindLoggedVisitor::searchLog(Stmt *stmt){
    
    if(!stmt)
        return 0;
    
    if(CallExpr *callExpr = dyn_cast<CallExpr>(stmt)){
        if(FunctionDecl *functionDecl = callExpr->getDirectCallee()){
            StringRef callName = functionDecl->getQualifiedNameAsString();
            
            if(hasKeyWord(callName)){
                return callExpr;
            }
        }
    }
    
    for(Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
        if(Stmt *child = *it){
            CallExpr *ret = searchLog(child);
            if(ret)
                return ret;
        }
    }
    
    return 0;
}


///search call site in the subtree of stmt
CallExpr* FindLoggedVisitor::searchCall(Stmt *stmt){
    
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
void FindLoggedVisitor::travelStmt(Stmt *stmt, Stmt *father){
    
    if(!stmt || !father)
        return;
    
    if(CallExpr* callExpr = dyn_cast<CallExpr>(stmt)){
        StringRef callName = "";
        if(FunctionDecl *functionDecl = callExpr->getDirectCallee())
            callName = functionDecl->getQualifiedNameAsString();
        myCalledTime[callName]++;
    }

    ///deal with first log pattern
    //code
    //  if(foo())
    //      log();
    //\code
    
    ///find if(foo()), stmt is located in the condexpr of ifstmt
    if(IfStmt *ifStmt = dyn_cast<IfStmt>(father)){
        if(ifStmt->getCond() == stmt){
            
            ///search for call in condexpr
            if((myCallExpr = searchCall(stmt))){
                StringRef callName = "";
                if(FunctionDecl *functionDecl = myCallExpr->getDirectCallee())
                    callName = functionDecl->getQualifiedNameAsString();
                
            
                ///search for log in both 'then body' and 'else body'
                if((myLogExpr = searchLog(ifStmt->getThen()))){
                    recordCallLog(myCallExpr, myLogExpr);
                    myLoggedTime[callName]++;
                }
                else if((myLogExpr = searchLog(ifStmt->getElse()))){
                    recordCallLog(myCallExpr, myLogExpr);
                    myLoggedTime[callName]++;
                }

                ///do not search inside the condexpr
                //return;
            }
        }
    }
    
    ///deal with second log pattern
    //code
    //  switch(foo());
    //      ...
    //      log();
    //\code
    
    ///find switch(foo()), stmt is located in the condexpr of switchstmt
    if(SwitchStmt *switchStmt = dyn_cast<SwitchStmt>(father)){
        if(switchStmt->getCond() == stmt){
            
            ///search for call in condexpr
            if((myCallExpr = searchCall(stmt))){
                StringRef callName = "";
                if(FunctionDecl *functionDecl = myCallExpr->getDirectCallee())
                    callName = functionDecl->getQualifiedNameAsString();
                
                ///search for log in each 'switch body'
                if((myLogExpr = searchLog(switchStmt->getBody()))){
                    recordCallLog(myCallExpr, myLogExpr);
                    myLoggedTime[callName]++;
                }
                ///do not search inside the condexpr
                //return;
            }
        }
    }
    
    ///deal with third log pattern
    //code
    //  ret = foo();
    //  if(ret)
    //      log();
    //\code
    
    ///find 'ret = foo()', when stmt = 'BinaryOperator', op == '=' and RHS == 'callexpr'
    if(BinaryOperator *binaryOperator = dyn_cast<BinaryOperator>(stmt)){
        if(binaryOperator->getOpcode() == BO_Assign){
            if(Expr *expr = binaryOperator->getRHS()){
                expr = expr->IgnoreImpCasts();
                expr = expr->IgnoreImplicit();
                expr = expr->IgnoreParens();
                if(CallExpr *callExpr = dyn_cast<CallExpr>(expr)){
                    
                    myReturnName = expr2str(binaryOperator->getLHS());
                    if(myReturnName == "")
                        return;
                    
                    myCallExpr = callExpr;
                    StringRef callName = "";
                    if(FunctionDecl *functionDecl = myCallExpr->getDirectCallee())
                        callName = functionDecl->getQualifiedNameAsString();
                    
                    ///search for brothers of stmt
                    ///young_brother means the branch after stmt
                    ///we only search for branches after stmt
                    bool isYoungBrother = false;
                    for(Stmt::child_iterator bro = father->child_begin(); bro != father->child_end(); ++bro){
                        
                        if(Stmt *brother = *bro){
                            
                            if(brother == stmt){
                                isYoungBrother = true;
                                continue;
                            }
                            
                            if(isYoungBrother == false){
                                continue;
                            }
                            
                            ///find 'if(ret)'
                            if(IfStmt *ifStmt = dyn_cast<IfStmt>(brother)){
                                
                                ///the return name, which is checked in condexpr
                                StringRef myCondStr = expr2str(ifStmt->getCond());
                                if(myCondStr.find(myReturnName) != string::npos){
                                    
                                    ///search for log in both 'then body' and 'else body'
                                    if((myLogExpr = searchLog(ifStmt->getThen()))){
                                        recordCallLog(myCallExpr, myLogExpr);
                                        myLoggedTime[callName]++;
                                        break;
                                    }
                                    else if((myLogExpr = searchLog(ifStmt->getElse()))){
                                        recordCallLog(myCallExpr, myLogExpr);
                                        myLoggedTime[callName]++;
                                        break;
                                    }
                                }
                            }
                            
                        }
                    }//end for
                    //return;
                }
            }
        }
    }
    
    for(Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
        if(Stmt *child = *it)
            travelStmt(child, stmt);
    }
    
    return;
}


bool FindLoggedVisitor::VisitFunctionDecl (FunctionDecl* Declaration){
    
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


void FindLoggedConsumer::HandleTranslationUnit(ASTContext& Context) {
    
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    return;
}


std::unique_ptr<clang::ASTConsumer> FindLoggedAction::CreateASTConsumer(CompilerInstance& Compiler, StringRef InFile){

    if(InFile == lastName)
        return 0;
    else{
        llvm::errs()<<"["<<++fileNum<<"/"<<fileTotalNum<<"]"<<" Now analyze the file: "<<InFile<<"\n";
        lastName = InFile;
        Rewriter rewriter;
        rewriter.setSourceMgr(Compiler.getSourceManager(), Compiler.getLangOpts());
        return std::unique_ptr<clang::ASTConsumer>(new FindLoggedConsumer(&Compiler, InFile));
    }
}

//
//  FindLoggedSnippet.cpp
//  LLVM
//
//  Created by ZhouyangJia on 16/4/13.
//
//

#include "MyTool.hpp"
#include "FindLoggedSnippet.hpp"
#include "FindLoggingFunction.hpp"

extern string lastName;
extern int fileTotalNum;
extern int fileNum;

extern string logNames[MAX_LOG_FUNC];
extern int logNamesCnt;

extern int totalLoggedSnippet;

extern FunctionFeat myCalledFeat[MAX_FUNC_NUM];
extern map<string, int> myCalledFeatMap;
extern int myCalledFeatCnt;

extern FILE* in;
extern FILE* out;


///record and output call-log pair
void FindLoggedVisitor::recordCallLog(CallExpr *callExpr, CallExpr *logExpr){
    
    if(!callExpr || !logExpr)
        return;
    
    if(!callExpr->getDirectCallee() || !logExpr->getDirectCallee())
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
        
        totalLoggedSnippet++;
        
        string name = callExpr->getDirectCallee()->getQualifiedNameAsString();
        int index = myCalledFeatMap[name];
        if(index == MAX_FUNC_NUM){
            errs()<<"Too many functions!\n";
            exit(1);
        }
        
        myCalledFeat[index].logNumber++;
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


///search log site in the subtree of stmt
CallExpr* FindLoggedVisitor::searchLog(Stmt *stmt){
    
    if(!stmt)
        return 0;
    
    if(CallExpr *callExpr = dyn_cast<CallExpr>(stmt)){
        if(FunctionDecl *functionDecl = callExpr->getDirectCallee()){
            StringRef callName = functionDecl->getQualifiedNameAsString();
            
            for(int i = 0; i < logNamesCnt-1; i++){
                if(logNames[i].find(callName) != string::npos){
                    return callExpr;
                }
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
            
                ///search for log in both 'then body' and 'else body'
                if((myLogExpr = searchLog(ifStmt->getThen())))
                    recordCallLog(myCallExpr, myLogExpr);
                
                else if((myLogExpr = searchLog(ifStmt->getElse())))
                    recordCallLog(myCallExpr, myLogExpr);

                ///do not search inside the condexpr
                return;
            }
        }
    }
    
    ///deal with second log pattern
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
                                        break;
                                    }
                                    else if((myLogExpr = searchLog(ifStmt->getElse()))){
                                        recordCallLog(myCallExpr, myLogExpr);
                                        break;
                                    }
                                }
                            }
                            
                        }
                    }//end for
                    return;
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
        llvm::errs()<<"[2/2]["<<++fileNum<<"/"<<fileTotalNum<<"]"<<" Now analyze the file: "<<InFile<<"\n";
        lastName = InFile;
        Rewriter rewriter;
        rewriter.setSourceMgr(Compiler.getSourceManager(), Compiler.getLangOpts());
        return std::unique_ptr<clang::ASTConsumer>(new FindLoggedConsumer(&Compiler, InFile));
    }
}

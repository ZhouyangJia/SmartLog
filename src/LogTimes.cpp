//
//  LogTimes.cpp
//  LLVM
//
//  Created by ZhouyangJia on 16/5/11.
//
//


#include "SmartLog.h"
#include "LogTimes.h"

extern string lastName;
extern int fileNum;
extern int fileTotalNum;

extern LogTime myLogTime[MAX_FUNC_NUM];
extern map<string, int> myLogTimeMap;
extern int myLogTimeCnt;


///output log time
void LogTime::print(){
    outs()<<name<<","<<call_time<<","<<log_time<<","<<log_level<<"\n";
}

///get the source code of stmt
string LogTimesVisitor::expr2str(Stmt *s){
    
    if(!s)
        return "";
    
    FullSourceLoc begin = CI->getASTContext().getFullLoc(s->getLocStart());
    FullSourceLoc end = CI->getASTContext().getFullLoc(s->getLocEnd());
    
    if(begin.isInvalid() || end.isInvalid())
        return "";
    
    SourceRange sr(begin.getExpansionLoc(), end.getExpansionLoc());
    
    return Lexer::getSourceText(CharSourceRange::getTokenRange(sr), CI->getSourceManager(), LangOptions(), 0);
}

void LogTimesVisitor::travelStmt(Stmt* stmt){
    
    if(!stmt)
        return;
    
    if(CallExpr* callExpr = dyn_cast<CallExpr>(stmt)){
        
        if(FunctionDecl* functionDecl = callExpr->getDirectCallee()){
            
            string name = functionDecl->getNameAsString();
            
            if(myLogTimeMap[name] == 0){
                myLogTimeMap[name] = ++myLogTimeCnt;
                myLogTime[myLogTimeCnt].name = name;
            }
            int index = myLogTimeMap[name];
            if(index == MAX_FUNC_NUM){
                errs()<<"Too many functions!\n";
                exit(1);
            }
            myLogTime[index].call_time++;
        }
    }
    
    for (Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
        if(Stmt *child = *it)
            travelStmt(child);
    }
}


bool LogTimesVisitor::VisitFunctionDecl(FunctionDecl* Declaration) {
    if(!(Declaration->isThisDeclarationADefinition() && Declaration->hasBody()))
        return true;
    
    FullSourceLoc functionstart = CI->getASTContext().getFullLoc(Declaration->getLocStart()).getExpansionLoc();
    //FullSourceLoc functionend = CI->getASTContext().getFullLoc(Declaration->getLocEnd()).getExpansionLoc();
    
    if(!functionstart.isValid())
        return true;
    
    if(functionstart.getFileID() != CI->getSourceManager().getMainFileID())
        return true;
    
    //errs()<<"Found function "<<Declaration->getQualifiedNameAsString() ;
    //errs()<<" @ " << functionstart.printToString(functionstart.getManager()) <<"\n";
    
    if(Declaration->getBody()){
        
        travelStmt(Declaration->getBody());
    }
    return true;
}


void LogTimesConsumer::HandleTranslationUnit(ASTContext& Context) {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    return;
}


std::unique_ptr<ASTConsumer> LogTimesAction::CreateASTConsumer(CompilerInstance& Compiler, StringRef InFile) {
    
    if(InFile == lastName)
        return 0;
    else{
        llvm::errs()<<"["<<++fileNum<<"/"<<fileTotalNum<<"]"<<" Now analyze the file: "<<InFile<<"\n";
        lastName = InFile;
        return std::unique_ptr<ASTConsumer>(new LogTimesConsumer(&Compiler, InFile));	}
}

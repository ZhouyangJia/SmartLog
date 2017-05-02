//
//  FindKeywordFuncion.cpp
//  LLVM
//
//  Created by ZhouyangJia on 16/4/28.
//
//

#include "SmartLog.h"
#include "FindKeywordFunction.h"
//#include "Python.h"

extern string lastName;
extern int fileNum;
extern int fileTotalNum;

extern map<string, int> myKeywordFunc;

extern int totalKeywordFunction;
extern int totalKeywordCallsite;

extern FILE* out;
extern FILE* out2;

///get the source code of stmt
string FindKeywordVisitor::expr2str(Stmt *s){
    
    if(!s)
        return "";
    
    FullSourceLoc begin = CI->getASTContext().getFullLoc(s->getLocStart());
    FullSourceLoc end = CI->getASTContext().getFullLoc(s->getLocEnd());
    
    if(begin.isInvalid() || end.isInvalid())
        return "";
    
    SourceRange sr(begin.getExpansionLoc(), end.getExpansionLoc());
    
    return Lexer::getSourceText(CharSourceRange::getTokenRange(sr), CI->getSourceManager(), LangOptions(), 0);
}


string FindKeywordVisitor::getfile(string f){
    if(f.find_last_of('/') == string::npos)
        return f;
    else
        return f.substr(f.find_last_of('/')+1);
}


string* FindKeywordVisitor::spiltWord(string name){
    
/*    Py_Initialize();
    
    if ( !Py_IsInitialized() )
    {
        return 0;
    }

    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('/Users/zhouyangjia/github/segment')");
    PyObject *pName,*pModule,*pDict,*pFunc,*pArgs;

    pName = PyString_FromString("segment.py");
    
    pModule = PyImport_Import(pName);
    if ( !pModule )
    {
        printf("can't find segment.py");
        getchar();
        return 0;
    }
    
    pDict = PyModule_GetDict(pModule);
    if ( !pDict )
    {
        return 0;
    }
    
    pFunc = PyDict_GetItemString(pDict, "segment");
    if ( !pFunc || !PyCallable_Check(pFunc) )
    {
        printf("can't find function [segment]");
        getchar();
        return 0;
    }

    pArgs = PyTuple_New(2);
    PyTuple_SetItem(pArgs, 0, Py_BuildValue("l",3));
    PyTuple_SetItem(pArgs, 1, Py_BuildValue("l",4));

    PyObject_CallObject(pFunc, pArgs);
    
    Py_DECREF(pName);
    Py_DECREF(pArgs);
    Py_DECREF(pModule);
    
    Py_Finalize();
    */
    return 0;
}


bool FindKeywordVisitor::hasKeyWord(string name){
	
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


void FindKeywordVisitor::travelStmt(Stmt* stmt){
    
    
    stmt->dump();
    return;
	
	if(CallExpr* callExpr = dyn_cast<CallExpr>(stmt)){
        
        if(FunctionDecl* functionDecl = callExpr->getDirectCallee()){
            
            string name = functionDecl->getNameAsString();
            
            if(hasKeyWord(name)){
                
                ///record keyword function
                if(myKeywordFunc[name] == 0){
                    fputs(name.c_str(), out);
                    fputs("\n", out);
                    totalKeywordFunction++;
                    myKeywordFunc[name] = 1;
                }
                
                ///record keyword callsit
                SourceLocation callLocation = callExpr->getLocStart();
                if(callLocation.isValid()){
                    string loc = callLocation.printToString(CI->getSourceManager()).c_str();
                    fputs(loc.c_str(), out2);
                    fputs("\n", out2);
                    totalKeywordCallsite++;
                }
            }
        }
	}

	for (Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
		if(Stmt *child = *it)
			travelStmt(child);
	}
}


bool FindKeywordVisitor::VisitFunctionDecl(FunctionDecl* Declaration) {
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
	
    /*if("actions_init" == Declaration->getQualifiedNameAsString())
        Declaration->getBody()->dump();
    return true;*/
    
	if(Declaration->getBody()){
        
    
		travelStmt(Declaration->getBody());	
	}
	return true;
}


void FindKeywordConsumer::HandleTranslationUnit(ASTContext& Context) {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
	return;
}


std::unique_ptr<ASTConsumer> FindKeywordAction::CreateASTConsumer(CompilerInstance& Compiler, StringRef InFile) {

	if(InFile == lastName)
		return 0;
	else{
		llvm::errs()<<"["<<++fileNum<<"/"<<fileTotalNum<<"]"<<" Now analyze the file: "<<InFile<<"\n";
		lastName = InFile;
        return std::unique_ptr<ASTConsumer>(new FindKeywordConsumer(&Compiler, InFile));	}
}

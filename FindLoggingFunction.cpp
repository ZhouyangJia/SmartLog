//
//  FindLoggingFuncion.cpp
//  LLVM
//
//  Created by ZhouyangJia on 16/4/13.
//
//

#include "MyTool.h"
#include "FindLoggingFunction.h"
//#include "Python.h"

extern string lastName;
extern int fileNum;
extern int fileTotalNum;

extern FunctionFeat myCalledFeat[MAX_FUNC_NUM];
extern map<string, int> myCalledFeatMap;
extern int myCalledFeatCnt;


//refresh before each file,add called times for the first appearance of function is this file
map<string, int> fileNumMap;

void FunctionFeat::print(){
    
    /*string::size_type pos=0;
    while((pos=funcName.find(',',pos))!=string::npos)
    {
        funcName.at(pos) = ';';
        pos+=1;
    }*/
    outs()<<funcName<<","<<funcKeyWord<<","<<fileName<<","<<fileKeyWord<<","<<lenth<<","<<flow<<","<<calledNumber<<","<<fileNumber<<","<<haschar<<","<<decl<<","<<label<<"\n";
}


///get the source code of stmt
string FindLoggingVisitor::expr2str(Stmt *s){
    
    if(!s)
        return "";
    
    FullSourceLoc begin = CI->getASTContext().getFullLoc(s->getLocStart());
    FullSourceLoc end = CI->getASTContext().getFullLoc(s->getLocEnd());
    
    if(begin.isInvalid() || end.isInvalid())
        return "";
    
    SourceRange sr(begin.getExpansionLoc(), end.getExpansionLoc());
    
    return Lexer::getSourceText(CharSourceRange::getTokenRange(sr), CI->getSourceManager(), LangOptions(), 0);
}

string FindLoggingVisitor::getfile(string f){
	if(f.find_last_of('/') == string::npos)
		return f;
	else
		return f.substr(f.find_last_of('/')+1);	
}


bool FindLoggingVisitor::hasChar(QualType qt){
    
    if(qt.isNull())
        return 0;
    
    string type = qt.getAsString();
    if(type.find("char") < type.length() || type.find("string") < type.length() ||
       type.find("Char") < type.length() || type.find("String") < type.length())
        return 1;
    
    const Type *t = qt.getTypePtr();
    if(!t)
        return 0;
    
    const RecordType *rt = t->getAsStructureType();
    if(!rt)
        return 0;
    
    RecordDecl *rd = rt->getDecl();
    if(!rd)
        return 0;
    
    for(RecordDecl::field_iterator it = rd->field_begin(); it != rd->field_end(); ++it){
        FieldDecl *fd = *it;
        if(hasChar(fd->getType()))
           return 1;
    }
    
    return 0;
}



string* FindLoggingVisitor::spiltWord(string name){
    
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


bool FindLoggingVisitor::hasKeyWord(string name){
	
	transform(name.begin(), name.end(), name.begin(), ::tolower);  //::toupper
    
    //string* spilted;
    //spilted = spiltWord(name);
	
	string word[30] = {
            "log",	//exclude binlog, logic
			"error", 	//exclude parse, 
			"err", 		//?
			"die",		//?
			"fail",		//?
			
			"hint", 	//?
			"put", 		//exclude input, 
			"assert",	//exclude 
			"trace",	//exclude 
			"print", 	//exclude sprintf, snprintf, vsnprintf
			
			"write", 	//exclude 
			"report", 	//exclude 
			"record", 	//exclude 
			"dump", 	//exclude 
			"msg", 		//exclude 
			
			"message",	//exclude  
			"out", 		//exclude pullout, timeout, routine, layout,
			"warn", 	//exclude 
			"debug"  	//exclude
        
            "emerg"
            "alert"
            "crit"
			
			//"note",	//deleted
			//"list" 	//deleted
			//"dialog"  	//included
			//"output"  	//included
	}; 
	
	string nword[30] = {"binlog",
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
	};
	
	for(int i = 0; i < 30; i++){
		if(nword[i].length() == 0)
			break;
		if(name.find(nword[i]) < name.length() && (name.find_last_of(':') == string::npos || name.find(nword[i]) > name.find_last_of(':')))
			return false;
	}
	
	for(int i = 0; i < 30; i++){
		if(word[i].length() == 0)
			break;
		if(name.find(word[i]) < name.length() && (name.find_last_of(':') == string::npos || name.find(word[i]) > name.find_last_of(':')))
			return true;
	}	
	
	return false;
}


void FindLoggingVisitor::travelStmt(Stmt* stmt){
	
	if(CallExpr* callExpr = dyn_cast<CallExpr>(stmt)){
		if(FunctionDecl* functionDeal = callExpr->getDirectCallee()){
			
            string name = functionDeal->getNameAsString();
			if(myCalledFeatMap[name] == 0){
				myCalledFeatMap[name] = ++myCalledFeatCnt;
			}
            
			int index = myCalledFeatMap[name];
			if(index == MAX_FUNC_NUM){
				errs()<<"Too many functions!\n";
				exit(1);
			}
			
			myCalledFeat[index].calledNumber++;	
			
			if(fileNumMap[name] == 0){
				fileNumMap[name] = 1;
				
				string fileName = CI->getASTContext().getFullLoc(functionDeal->getLocStart()).printToString(CI->getSourceManager());
				fileName = fileName.substr(0, fileName.find_first_of(':'));
				myCalledFeat[index].funcName = name;
				myCalledFeat[index].fileName = fileName;
				myCalledFeat[index].fileNumber++;
				myCalledFeat[index].fileKeyWord = hasKeyWord(getfile(fileName));
				myCalledFeat[index].funcKeyWord = hasKeyWord(name);
			}
		}
	}

	for (Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
		if(Stmt *child = *it)
			travelStmt(child);
	}
}

bool FindLoggingVisitor::VisitFunctionDecl(FunctionDecl* Declaration) {
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
	
        string name = Declaration->getNameAsString();
		
		if(myCalledFeatMap[name] == 0){
			myCalledFeatMap[name] = ++myCalledFeatCnt;
		}
		int index = myCalledFeatMap[name];
		if(index == MAX_FUNC_NUM){
			errs()<<"Too many functions!\n";
			exit(1);
		}
		
		unsigned int start, end;
		FullSourceLoc functionstart = CI->getASTContext().getFullLoc(Declaration->getLocStart()).getExpansionLoc();
		FullSourceLoc functionend = CI->getASTContext().getFullLoc(Declaration->getLocEnd()).getExpansionLoc();
		start = functionstart.getExpansionLineNumber();
		end = functionend.getExpansionLineNumber();
		myCalledFeat[index].lenth = end - start;
		
		//string fileName = functionstart.printToString(CI->getSourceManager());
		//fileName = fileName.substr(0, fileName.find_first_of(':'));
        string fileName = InFile;
		myCalledFeat[index].fileKeyWord = hasKeyWord(getfile(fileName));
        myCalledFeat[index].funcName = name;
        myCalledFeat[index].fileName = fileName;
		myCalledFeat[index].funcKeyWord = hasKeyWord(name);
        myCalledFeat[index].decl = true;
        myCalledFeat[index].source = Lexer::getSourceText(CharSourceRange::getTokenRange(Declaration->getSourceRange()), CI->getSourceManager(), LangOptions(), 0);
        
        
		for(unsigned int i = 0; i < Declaration->param_size(); i++) {
            if(hasChar(Declaration->getParamDecl(i)->getType())){
				myCalledFeat[index].haschar = true;
				break;
			}
		}

		travelStmt(Declaration->getBody());	
	}
	return true;
}

void FindLoggingConsumer::HandleTranslationUnit(ASTContext& Context) {
	fileNumMap.clear();
	Visitor.TraverseDecl(Context.getTranslationUnitDecl());
	return;
}

std::unique_ptr<ASTConsumer> FindLoggingAction::CreateASTConsumer(CompilerInstance& Compiler, StringRef InFile) {

	if(InFile == lastName)
		return 0;
	else{
		llvm::errs()<<"[1/2]["<<++fileNum<<"/"<<fileTotalNum<<"]"<<" Now analyze the file: "<<InFile<<"\n";
		lastName = InFile;
        return std::unique_ptr<ASTConsumer>(new FindLoggingConsumer(&Compiler, InFile));	}
}

#include "FindLogFunc.h"

extern string lastName;
extern int fileNum;

map<string, int>funcnamemap;


vector<FunctionDecl*>toBeLogFunc;

void FindLogVisitor::travelStmt(Stmt* stmt, Stmt* parent){

	for (Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
		if(Stmt *child = *it)
			travelStmt(child, stmt);
	}
}

bool FindLogVisitor::hasKeyWord(string name){
	
	string word[12] = {"log", "error", "err", "put", 
			"print", "write", "report", "dump", 
			"msg", "message", "out", "warn"};
	
	for(int i = 0; i < 12; i++){
		if(name.find(word[i]) < name.length())
			return true;
	}	
	
	return false;
}

bool FindLogVisitor::VisitFunctionDecl(FunctionDecl* Declaration) {
	if(!(Declaration->isThisDeclarationADefinition() && Declaration->hasBody()))
		return true;
	
	FullSourceLoc functionstart = CI->getASTContext().getFullLoc(Declaration->getLocStart()).getExpansionLoc();
	FullSourceLoc functionend = CI->getASTContext().getFullLoc(Declaration->getLocEnd()).getExpansionLoc();
	
	if(!functionstart.isValid())
		return true;
	
	if(functionstart.getFileID() != CI->getSourceManager().getMainFileID())
		return true;

	//llvm::errs()<<"Found function "<<Declaration->getQualifiedNameAsString() ;
	//llvm::errs()<<" @ " << functionstart.printToString(functionstart.getManager()) <<"\n";
	
	if(Declaration->getBody()){
		unsigned int start, end;
		start = functionstart.getExpansionLineNumber();
		end = functionend.getExpansionLineNumber();
		string functionname = Declaration->getQualifiedNameAsString();
		if(funcnamemap[functionname])
			return true;
		else
			funcnamemap[functionname] = 1;
		if(hasKeyWord(functionname) && end - start < 200){
			for(unsigned int i = 0; i < Declaration->param_size(); i++) {
				string type = Declaration->getParamDecl(i)->getOriginalType().getAsString();
				if(type.find("char") < type.length() || type.find("string") < type.length() ||
					type.find("Char") < type.length() || type.find("String") < type.length()){
					
					toBeLogFunc.push_back(Declaration);
					break;
				}
			}
		}
		
		//travelStmt(function, function);	
	}
	return true;
}

void FindLogConsumer::HandleTranslationUnit(ASTContext& Context) {
	toBeLogFunc.clear();
	funcnamemap.clear();
	Visitor.TraverseDecl(Context.getTranslationUnitDecl());
	if(toBeLogFunc.size() >= 2){
		for(unsigned i = 0; i < toBeLogFunc.size(); i++){
			FunctionDecl* func = toBeLogFunc[i];
			//FullSourceLoc funcLoc = CI->getASTContext().getFullLoc(func->getLocStart());
			//llvm::outs()<<funcLoc.printToString(funcLoc.getManager())<<",";
			llvm::outs()<<func->getQualifiedNameAsString()<<"\n";
			//llvm::outs()<<func->getType().getAsString()<<"\n";
		}
	}
	
	return;
}

std::unique_ptr<ASTConsumer> FindLogAction::CreateASTConsumer(CompilerInstance& Compiler, StringRef InFile) {

	if(InFile == lastName)
		return 0;
	else{
		llvm::errs()<<""<<++fileNum<<" Now analyze the file: "<<InFile<<"\n";
		lastName = InFile;
		return std::unique_ptr<ASTConsumer>(new FindLogConsumer(&Compiler, InFile));
	}
}

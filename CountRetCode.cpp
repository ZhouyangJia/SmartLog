#include "CountRetCode.h"

extern string lastName;
extern int fileNum;


extern int returnCodeNum;
extern int totalReturnNum;

StringRef CountRetVisitor::expr2str(Stmt *s){
	
	FullSourceLoc begin = CI->getASTContext().getFullLoc(s->getLocStart());
	FullSourceLoc end = CI->getASTContext().getFullLoc(s->getLocEnd());
	SourceRange sr(begin.getExpansionLoc(), end.getExpansionLoc());
	return Lexer::getSourceText(CharSourceRange::getTokenRange(sr), CI->getSourceManager(), LangOptions(), 0);
}


bool CountRetVisitor::travelReturnStmt(Stmt* stmt){
	int output = 0;
	
	if(dyn_cast<UnaryExprOrTypeTraitExpr>(stmt)
			|| dyn_cast<VAArgExpr>(stmt)
			|| dyn_cast<OffsetOfExpr>(stmt)
			|| dyn_cast<CXXThisExpr>(stmt) 
			|| dyn_cast<CXXNewExpr>(stmt) 
			|| dyn_cast<StmtExpr>(stmt)
			|| dyn_cast<CallExpr>(stmt)
			|| dyn_cast<MemberExpr>(stmt)
			|| dyn_cast<ArraySubscriptExpr>(stmt)
			|| dyn_cast<StmtExpr>(stmt)) { 
		return false;
	}
	
	else if(dyn_cast<GNUNullExpr>(stmt)){
		if(output)llvm::errs()<<"NULL\n";
		return true;
	}
	
	else if(IntegerLiteral* integerLiteral = dyn_cast<IntegerLiteral>(stmt)){
		char* end;
		if(output)llvm::errs()<< static_cast<int>(strtol(integerLiteral->getValue().toString(10,true).c_str(),&end,10))<<"\n";
		return true;
	}
	
	else if(DeclRefExpr* declRefExpr = dyn_cast<DeclRefExpr>(stmt)){
		if(declRefExpr->getDecl()->getType().getCanonicalType().getAsString() == "int"){
			if(output)llvm::errs()<< declRefExpr->getDecl()->getDeclName().getAsString()<<"\n";
			return true;
		}
	}
	
	for(Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
		if(Stmt *child = *it)
			return travelReturnStmt(child);
	}
	
	return false;
	
}


void CountRetVisitor::travelStmt(Stmt* stmt){

	if(dyn_cast<ReturnStmt>(stmt)){
		totalReturnNum++;
		if(travelReturnStmt(stmt)){
			returnCodeNum++;
			//llvm::errs()<<expr2str(stmt)<<"\n";
		}
		else{
			//llvm::outs()<<expr2str(stmt)<<"\n";
		}
		return;
	}
	
	for(Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
		if(Stmt *child = *it)
			travelStmt(child);
	}
}


bool CountRetVisitor::VisitFunctionDecl (FunctionDecl* Declaration){
	
	if(!(Declaration->isThisDeclarationADefinition() && Declaration->hasBody()))
		return true;
	
	FullSourceLoc FullLocation = CI->getASTContext().getFullLoc(Declaration->getLocStart());
	
	if(!FullLocation.isValid())
		return true;
	
	//the function is located in head file
	if(FullLocation.getExpansionLoc().getFileID() != CI->getSourceManager().getMainFileID())
		return true;

	//llvm::errs()<<"Found function "<<Declaration->getQualifiedNameAsString() ;
	//llvm::errs()<<" @ " << FullLocation.printToString(FullLocation.getManager()) <<"\n";
	
		
	if(Stmt* function = Declaration->getBody()){
		travelStmt(function);	
	}
	
	
	return true;
}


void CountRetConsumer::HandleTranslationUnit(ASTContext& Context) {
	Visitor.TraverseDecl(Context.getTranslationUnitDecl());
	return;
}


std::unique_ptr<clang::ASTConsumer> CountRetAction::CreateASTConsumer(CompilerInstance& Compiler, StringRef InFile){
	//somethings wrong here, all the files always be analyzed twice (or more) by the command: 
	//"find path/in/subtree -name '*.cpp'|xargs clang-mytool -find-function"
	//The reason is that every file appears multiple times in compile_commands.json.
	
	if(InFile == lastName)
		return 0;
	else{
		llvm::errs()<<""<<++fileNum<<" Now analyze the file: "<<InFile<<"\n";
		lastName = InFile;
		Rewriter rewriter;
		rewriter.setSourceMgr(Compiler.getSourceManager(), Compiler.getLangOpts());
		return std::unique_ptr<clang::ASTConsumer>(new CountRetConsumer(&Compiler, InFile));
	}
}

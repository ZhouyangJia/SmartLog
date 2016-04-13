#include "InsertLog.h"
#include "MyTool.h"

extern FuncCondInfo myFuncCondInfo[MAX_LOGGED_FUNC];
extern map<string, int> myFuncCondInfoMap;
extern int myFuncCondInfoCnt;

extern string lastName;
extern int fileNum;
extern int totalLogNum;

void InsertLogVisitor::travelStmt(Stmt* stmt, Stmt* parent){
	
	if(dyn_cast<DeclStmt>(stmt))
		return;
	
	if(dyn_cast<ReturnStmt>(stmt))
		return;
	
	if(dyn_cast<BinaryOperator>(stmt)){
		return;
	}
	
	if(dyn_cast<ConditionalOperator>(stmt)){
		return;
	}
	
	//TODO
	if(dyn_cast<CStyleCastExpr>(stmt)){
		return;
	}
	
	if(IfStmt* ifStmt = dyn_cast<IfStmt>(stmt)) {
		if(Stmt* ifthen = ifStmt->getThen())
			travelStmt(ifthen, stmt);
		if(Stmt* ifelse = ifStmt->getElse())
			travelStmt(ifelse, stmt);
		return;		
	}
	else if(SwitchStmt* switchStmt = dyn_cast<SwitchStmt>(stmt)) {
		if(Stmt* switchbody = switchStmt->getBody())
			travelStmt(switchbody, stmt);
		return;		
	}
	else if(ForStmt* forStmt = dyn_cast<ForStmt>(stmt)) {
		if(Stmt* forinit = forStmt->getInit())
			travelStmt(forinit, stmt);
		if(Stmt* forbody = forStmt->getBody())
			travelStmt(forbody, stmt);
		return;		
	}
	else if(WhileStmt* whileStmt = dyn_cast<WhileStmt>(stmt)) {
		if(Stmt* whilebody = whileStmt->getBody())
			travelStmt(whilebody, stmt);
		return;		
	}
	
	if(CallExpr* callExpr = dyn_cast<CallExpr>(stmt)){
		if(callExpr->getDirectCallee()){
			string funcName = callExpr->getDirectCallee()->getQualifiedNameAsString();
			QualType type = callExpr->getDirectCallee()->getReturnType();
// 			if(funcName == "String::alloc"){
// 				callExpr->dump();
// 				llvm::errs()<<type.getAsString()<<"\n";
// 			}
			if(type.getAsString() == "void")return;
			int index = myFuncCondInfoMap[funcName];
			if(index){
				//TODO CStyleCastExpr
// 				if(isa<CStyleCastExpr>(parent)){
// 					SourceLocation s = parent->getLocStart();
// 					//SourceLocation e = Lexer::findLocationAfterToken(s,tok::r_paren,CI->getSourceManager(),CI->getLangOpts(), true);
// 					SourceLocation e = parent->getLocEnd();
// 					if(s.isValid() && e.isValid()){
// 						SourceRange sr(s,e);
// 						rewriter.RemoveText(sr);
// 					}
// 				}
				FullSourceLoc start = CI->getASTContext().getFullLoc(callExpr->getLocStart());
				FullSourceLoc end = CI->getASTContext().getFullLoc(callExpr->getLocEnd());
				if(start.isInvalid() || end.isInvalid())return;
				start = start.getExpansionLoc();
				end = end.getExpansionLoc();
				llvm::outs()<<"Function: "<<funcName<<" Try to insert log @ "<<end.printToString(CI->getSourceManager())<<"\n";
				
				//SourceLocation START = stmt->getLocStart();
				SourceLocation START = (SourceLocation)start;
				SourceLocation END = Lexer::findLocationAfterToken(stmt->getLocEnd(),
										tok::semi,
										CI->getSourceManager(),
										CI->getLangOpts(), 
										true);
				
				string rettype = callExpr->getType().getCanonicalType().getAsString();
				
				if(rettype == "_Bool")rettype = "bool";
				
				string call = Lexer::getSourceText(CharSourceRange::getTokenRange(callExpr->getSourceRange()),
						rewriter.getSourceMgr(),rewriter.getLangOpts(),0);
				
				if(START.isInvalid() || END.isInvalid())
					return;
				
				if(rewriter.InsertText(START, "\n#ifdef SMARTLOG\n{\n\t", true, true))
					llvm::outs()<<"oh no!\n";
					
				string invoke = rettype;
				invoke.append(" SL_RET = ");
				invoke.append(call);
				invoke.append(";\n");
				rewriter.InsertText(START, invoke, true, true);
				
				for(int i = 0; i < myFuncCondInfo[index].condcnt; i++){
					if(myFuncCondInfo[index].isChosen[i] == false)
						continue;
						
					string cond = "\tif(";
					cond.append(myFuncCondInfo[index].funcCond[i]);
					cond.append(")\n");
					rewriter.InsertText(START, cond, true, true);
					
					string logstmt = "\t\tprintf(\"SmartLog-";
					logstmt.append(start.printToString(rewriter.getSourceMgr()));
					logstmt.append("\\n\");\n");
					rewriter.InsertText(START, logstmt, true, true);
					
				}
				rewriter.InsertText(START, "}\n#else\n", true, true);
				rewriter.InsertText(END, "\n#endif\n", true, true);	
				rewriter.InsertText(END, "\n", true, false);		
				
				totalLogNum++;
				llvm::outs()<<"Insert "<< totalLogNum<<" conditions!\n";
				
				llvm::outs()<<rewriter.getRewrittenText(parent->getSourceRange());
				llvm::outs()<<"\n\n";
			}
		}
		return;	
	}
	
	for (Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
		if(Stmt *child = *it)
			travelStmt(child, stmt);
	}
}

bool InsertLogVisitor::VisitFunctionDecl(FunctionDecl* Declaration) {
	if(!(Declaration->isThisDeclarationADefinition() && Declaration->hasBody()))
		return true;
	
	FullSourceLoc FullLocation = CI->getASTContext().getFullLoc(Declaration->getLocStart());
	
	if(!FullLocation.isValid())
		return true;
	
	if(FullLocation.getFileID() != CI->getSourceManager().getMainFileID())
		return true;
	

	//llvm::errs()<<"Found function "<<Declaration->getQualifiedNameAsString() ;
	//llvm::errs()<<" @ " << FullLocation.printToString(FullLocation.getManager()) <<"\n";
	
	if(Stmt* function = Declaration->getBody()){
		travelStmt(function, function);	
	}
	return true;
}

void InsertLogConsumer::HandleTranslationUnit(ASTContext& Context) {
	
	Visitor.TraverseDecl(Context.getTranslationUnitDecl());
	
	const RewriteBuffer *RewriteBuf = Visitor.rewriter.getRewriteBufferFor(Visitor.CI->getSourceManager().getMainFileID());
	if(RewriteBuf){
		string outName (Visitor.InFile);
		outName.append(".SLout");
        
        
        std::error_code EC;
        llvm::raw_fd_ostream outFile(outName.c_str(), EC, llvm::sys::fs::F_Text);
        
		if(RewriteBuf->size() != 0){
			RewriteBuffer::iterator it1 = RewriteBuf->begin();
			RewriteBuffer::iterator it2 = RewriteBuf->end();
			outFile << string(it1, it2);
		}
		//outFile << "haha\n";
		outFile.close();
	}

	
	return;
}

std::unique_ptr<ASTConsumer> InsertLogAction::CreateASTConsumer(CompilerInstance& Compiler, StringRef InFile) {

	if(InFile == lastName)
		return 0;
	else{
		llvm::errs()<<""<<++fileNum<<" Now analyze the file: "<<InFile<<"\n";
		lastName = InFile;
		Rewriter rewriter;
		rewriter.setSourceMgr(Compiler.getSourceManager(), Compiler.getLangOpts());
		return std::unique_ptr<ASTConsumer>(new InsertLogConsumer(&Compiler, InFile, rewriter));
	}
}

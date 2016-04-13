#include "RecordCond.h"
#include "MyTool.h"

extern FuncLogInfo myFuncLogInfo[MAX_LOGGED_FUNC];
extern map<string, int> myFuncLogInfoMap;
extern int myFuncLogInfoCnt;

extern string lastName;
extern int fileNum;

extern int cntfunc;

bool mydebug;


string RecordCondVisitor::getfile(string f){
	if(f.find_last_of('/') == string::npos)
		return f;
	else
		return f.substr(f.find_last_of('/')+1);	
}

StringRef RecordCondVisitor::expr2str(Expr *e, int mode){
	
	FullSourceLoc begin = CI->getASTContext().getFullLoc(e->getLocStart());
	FullSourceLoc end = CI->getASTContext().getFullLoc(e->getLocEnd());
	if(mode == GET_SPELLING_LOC){
		SourceRange sr(begin.getSpellingLoc(), end.getSpellingLoc());
		return Lexer::getSourceText(CharSourceRange::getTokenRange(sr), CI->getSourceManager(), LangOptions(), 0);
	}
	else{
		SourceRange sr(begin.getExpansionLoc(), end.getExpansionLoc());
		return Lexer::getSourceText(CharSourceRange::getTokenRange(sr), CI->getSourceManager(), LangOptions(), 0);
	}
}

/*int RecordCondVisitor::inStmt(int logLocLine, Stmt* stmt){
	int ret = 0;
	
	if(stmt == NULL)
		return -1;
	if(!CI->getASTContext().getFullLoc(stmt->getLocStart()).isValid() || 
		!CI->getASTContext().getFullLoc(stmt->getLocEnd()).isValid())
		return -1;
	
	if(IfStmt* ifStmt = dyn_cast<IfStmt>(stmt)){
		int ifStartLine, ifEndLine;
		ifStartLine = CI->getASTContext().getFullLoc(ifStmt->getLocStart()).getExpansionLineNumber();
		ifEndLine = CI->getASTContext().getFullLoc(ifStmt->getLocEnd()).getExpansionLineNumber();
		if(ifStartLine <= logLocLine && ifEndLine >= logLocLine)
			ret = 1;
		
		if(Stmt* elseStmt = ifStmt->getElse()){
			int elseStartLine, elseEndLine;
			elseStartLine = CI->getASTContext().getFullLoc(elseStmt->getLocStart()).getExpansionLineNumber();
			elseEndLine = CI->getASTContext().getFullLoc(elseStmt->getLocEnd()).getExpansionLineNumber();
			if(elseStartLine <= logLocLine && elseEndLine >= logLocLine)
				ret = 2;
		}
	}
	else{
		int startLine, endLine;
		startLine = CI->getASTContext().getFullLoc(stmt->getLocStart()).getExpansionLineNumber();
		endLine = CI->getASTContext().getFullLoc(stmt->getLocEnd()).getExpansionLineNumber();
		if(startLine <= logLocLine && endLine >= logLocLine)
			ret = 1;
	}

	return ret;
}*/

string RecordCondVisitor::getDeeperName(Stmt* stmt){
	
	if(DeclRefExpr* declRefExpr = dyn_cast<DeclRefExpr>(stmt)){
		return declRefExpr->getDecl()->getDeclName().getAsString();
	}
	
	for (Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it)
	{
		if(Stmt *child = *it)
			return getDeeperName(child);
	}
	return "";
}

void RecordCondVisitor::analyzeCond(Expr* expr, Condition* cond){
	
	if(expr == NULL) return;	
	expr = expr->IgnoreImpCasts();
	expr = expr->IgnoreImplicit();
	expr = expr->IgnoreParens();

	if(UnaryOperator* unaryOperator1 = dyn_cast<UnaryOperator>(expr)){  //only deal with two unaryOperator
		cond->hasUop1 = true;
		cond->uop1 = unaryOperator1->getOpcode();
		
		Expr* subExpr = unaryOperator1->getSubExpr();
		if(subExpr != NULL) {
			subExpr = subExpr->IgnoreParenImpCasts();
			subExpr = subExpr->IgnoreParenCasts();
			if(UnaryOperator* unaryOperator2 = dyn_cast<UnaryOperator>(subExpr)){
				cond->hasUop2 = true;
				cond->uop2 = unaryOperator2->getOpcode();
				analyzeCond(unaryOperator2->getSubExpr(), cond);
			}
			else
				analyzeCond(unaryOperator1->getSubExpr(), cond);
		}
		return;
	}
		
	else if(BinaryOperator* binaryOperator = dyn_cast<BinaryOperator>(expr)){
		cond->bop = binaryOperator->getOpcode();	
		cond->hasBop = true;
		
		Expr* lv = binaryOperator->getLHS();
		Expr* rv = binaryOperator->getRHS();	
		
		Condition* lcond = new Condition();
		Condition* rcond = new Condition();
		
		cond->leftChild = lcond;
		cond->rightChild = rcond;
		
		analyzeCond(lv, lcond);
		analyzeCond(rv, rcond);
		
		if(cond->bop == clang::BO_EQ){
			if(rcond->dataType == TYPE_INT && rcond->ival == 0){
				//myfree(cond->rightChild);
				cond->assign(cond->leftChild);
				if(cond->hasUop1 && cond->uop1 == clang::UO_LNot){
					cond->hasUop1 = cond->hasUop2;
					cond->uop1 = cond->uop2;
				}
			}
			if(lcond->dataType == TYPE_INT && lcond->ival == 0){
				//myfree(cond->rightChild);
				cond->assign(cond->rightChild);
				if(cond->hasUop1 && cond->uop1 == clang::UO_LNot){
					cond->hasUop1 = cond->hasUop2;
					cond->uop1 = cond->uop2;
				}
			}
		}
		else if(cond->bop == clang::BO_NE){
			if(rcond->dataType == TYPE_INT && rcond->ival == 0){
				//myfree(cond->rightChild);
				cond->assign(cond->leftChild);
			}	
			if(lcond->dataType == TYPE_INT && lcond->ival == 0){
				//myfree(cond->rightChild);
				cond->assign(cond->rightChild);
			}
		}
		return;

	}
	
	else if(ConditionalOperator* conditionalOperator = dyn_cast<ConditionalOperator>(expr)){
		
		Condition* lcond = new Condition();
		Condition* rcond = new Condition();
		
		cond->hasBop = true;
		cond->bop = clang::BO_LOr;
		
		cond->leftChild = lcond;
		cond->rightChild = rcond;
		
		//a?b:c <==> a&&b || !a&&c
		Condition* acond = new Condition();//a
		Condition* nacond = new Condition();//!a
		Condition* bcond = new Condition();//b
		Condition* ccond = new Condition();//c
		
		analyzeCond(conditionalOperator->getCond(), acond);
		nacond->hasUop1 = true;
		nacond->uop1 = clang::UO_LNot;
		analyzeCond(conditionalOperator->getCond(), nacond);
		
		analyzeCond(conditionalOperator->getTrueExpr(), bcond);
		analyzeCond(conditionalOperator->getFalseExpr(), ccond);
		
		lcond->hasBop = true;
		lcond->bop = clang::BO_LAnd;
		lcond->leftChild = acond;
		lcond->rightChild = bcond;
		
		rcond->hasBop = true;
		rcond->bop = clang::BO_LAnd;
		rcond->leftChild = nacond;
		rcond->rightChild = ccond;
		return;
	}

	else if(CallExpr* callExpr = dyn_cast<CallExpr>(expr)){
		
		if(callExpr->getDirectCallee()){
			string tmpname = callExpr->getDirectCallee()->getQualifiedNameAsString();
			cond->isLeaf = true;
			cond->dataType = TYPE_UNKNOW;
			cond->sval = tmpname;
			if(tmpname == funcName){
				//cond->dataType = TYPE_CALL;
				cond->dataType = TYPE_RET;
			} 
		}
		return;
	}

	else if(dyn_cast<UnaryExprOrTypeTraitExpr>(expr) // (sizeof)
			|| dyn_cast<VAArgExpr>(expr)	 // (...)
			|| dyn_cast<OffsetOfExpr>(expr)	 // (long)
			|| dyn_cast<CXXThisExpr>(expr) 
			|| dyn_cast<CXXNewExpr>(expr) 
			|| dyn_cast<CXXBoolLiteralExpr>(expr)
			|| dyn_cast<StmtExpr>(expr)
			|| dyn_cast<CStyleCastExpr>(expr)) { 
		cond->isLeaf = true;
		cond->dataType = TYPE_UNKNOW;
		return;
	}
		
	else if(dyn_cast<GNUNullExpr>(expr)){//expr->isPotentialConstantExpr()
		cond->isLeaf = true;
		//cond->dataType = TYPE_NULL;
		//cond->sval = "NULL";
		cond->dataType = TYPE_INT;
		cond->ival = 0;
		return;
	}
	
	else if(IntegerLiteral* integerLiteral = dyn_cast<IntegerLiteral>(expr)){//expr->isPotentialConstantExpr()
		cond->isLeaf = true;
		cond->dataType = TYPE_INT;
		char* end;
		cond->ival = static_cast<int>(strtol(integerLiteral->getValue().toString(10,true).c_str(),&end,10));
		return;
	}
	
	else if(StringLiteral* stringLiteral = dyn_cast<StringLiteral>(expr)){
		cond->isLeaf = true;
		cond->dataType = TYPE_STRING;
		//cond->sval = stringLiteral->getString();
		cond->sval = stringLiteral->getBytes();
		return;
	}
	
	else if(FloatingLiteral* floatingLiteral = dyn_cast<FloatingLiteral>(expr)){
		cond->isLeaf = true;
		cond->dataType = TYPE_FLOAT;
		cond->fval = floatingLiteral->getValueAsApproximateDouble();
		return;
	}
	
	else if(CharacterLiteral* characterLiteral = dyn_cast<CharacterLiteral>(expr)){
		cond->isLeaf = true;
		cond->dataType = TYPE_CHAR;
		cond->ival = characterLiteral->getValue();
		return;
	}	
	
	else if(DeclRefExpr* declRefExpr = dyn_cast<DeclRefExpr>(expr)){
		cond->isLeaf = true;
		cond->dataType = TYPE_UNKNOW;
		cond->sval = declRefExpr->getDecl()->getDeclName().getAsString();
		//declRefExpr->dump();
		//llvm::errs()<<cond->sval<<" "<<declRefExpr->getType().getAsString()<<"\n";
		if(retName != "" && cond->sval.find(retName) < cond->sval.length()){
			cond->dataType = TYPE_RET;
			cond->sval = declRefExpr->getDecl()->getDeclName().getAsString();
		}
		else if(!declRefExpr->isLValue() && declRefExpr->getType().getAsString().find("enum") < declRefExpr->getType().getAsString().size()){
			cond->dataType = TYPE_ENUM;
			cond->sval = declRefExpr->getDecl()->getDeclName().getAsString();
		}
		//llvm::errs()<<cond->dataType<<"\n"; 
		return;
	}
	else if(MemberExpr* memberExpr = dyn_cast<MemberExpr>(expr)){
		cond->isLeaf = true;
		cond->dataType = TYPE_UNKNOW;
		//memberExpr->dump();
		cond->sval = getDeeperName(memberExpr);
		if(retName != ""  && cond->sval.find(retName) < cond->sval.length() && cond->sval.find(".") < cond->sval.length()
			&& cond->sval.find("->") < cond->sval.length() ){
			cond->dataType = TYPE_MEMBER;
		}
		//llvm::errs()<<cond->dataType<<"\n"; 
		return;
	}
	
	else if(ArraySubscriptExpr* arraySubscriptExpr = dyn_cast<ArraySubscriptExpr>(expr)){
		cond->isLeaf = true;
		cond->dataType = TYPE_UNKNOW;
		//arraySubscriptExpr->dump();
		cond->sval = getDeeperName(arraySubscriptExpr);
		if(retName != ""  && cond->sval.find(retName) < cond->sval.length() && cond->sval.find("[") < cond->sval.length()
			&& cond->sval.find("]") < cond->sval.length() ){
			cond->dataType = TYPE_ARRAY;
		}
		//llvm::errs()<<cond->dataType<<"\n"; 
		return;
	}
	
	else{
		
		llvm::errs()<<"++++++++other++++++expr+++++++++++type++++++++++ "; 
		llvm::errs()<<expr->getStmtClassName()<<"\n";
		FullSourceLoc otherLoc;
		otherLoc = CI->getASTContext().getFullLoc(expr->getExprLoc());
		llvm::errs()<<otherLoc.getExpansionLoc().printToString(CI->getSourceManager())<<"\n";
		return;
	}
	return;
}

void RecordCondVisitor::analyzeArgu(Argument* a, Stmt* stmt, int i){
	
	if(a->argvcnt[i] >= 100) return;
	
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
		return;
	}
	
	else if(dyn_cast<GNUNullExpr>(stmt)){
		a->argv[i][a->argvcnt[i]] = "0";
		a->argvcnt[i]++;
		return;
	}
	
	else if(IntegerLiteral* integerLiteral = dyn_cast<IntegerLiteral>(stmt)){
		a->argv[i][a->argvcnt[i]] = integerLiteral->getValue().toString(10,true);
		a->argvcnt[i]++;
		return;
	}
		
	else if(StringLiteral* stringLiteral = dyn_cast<StringLiteral>(stmt)){
		//"a->argv[i][a->argvcnt[i]] = stringLiteral->getString(); " is wrong!
		// fopen(fname, "r")
		// Assertion `CharByteWidth==1 && "This function is used in places that assume strings use char
		a->argv[i][a->argvcnt[i]] = stringLiteral->getBytes();
		a->argvcnt[i]++;
		return;
	}
	
	else if(CharacterLiteral* characterLiteral = dyn_cast<CharacterLiteral>(stmt)){
		a->argv[i][a->argvcnt[i]][0] = characterLiteral->getValue();
		a->argv[i][a->argvcnt[i]][1] = '\0';
		a->argvcnt[i]++;
		return;
	}	
	
	else if(DeclRefExpr* declRefExpr = dyn_cast<DeclRefExpr>(stmt)){
		if(!declRefExpr->isLValue() && declRefExpr->getType().getAsString().find("enum") < declRefExpr->getType().getAsString().size()){
			a->argv[i][a->argvcnt[i]] = declRefExpr->getDecl()->getDeclName().getAsString();
			a->argvcnt[i]++;
			return;
		}
	}
	
	for (Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
		if(Stmt *child = *it)
			analyzeArgu(a, child, i);
	}
}

void RecordCondVisitor::addArgument(int index, int logID){
	
	Argument* arg = new Argument();
	unsigned int cnt;
	cnt = funcCall->getNumArgs();
	arg->argc = cnt;
	arg->str = expr2str(funcCall, GET_EXPANSION_LOC);
	
	for(unsigned int i = 0; i < cnt; i++){
		if(i == 100)break;
		Expr* argv_expr = funcCall->getArg(i);
		analyzeArgu(arg, argv_expr, i);
	}
	
	myFuncLogInfo[index].myInfo[logID].arg = arg;
	myFuncLogInfo[index].myInfo[logID].call = funcCall;
}

void RecordCondVisitor::addCondition(int index, int i){
	
	Condition* c = new Condition();
	c->assign(conditions[0]);
	
	myFuncLogInfo[index].myInfo[i].condstr[0] = expr2str(condexpr[0], GET_EXPANSION_LOC);
	myFuncLogInfo[index].myInfo[i].iselse[0] = iselse[0];
	myFuncLogInfo[index].myInfo[i].cond = c;
	myFuncLogInfo[index].myInfo[i].condcnt = condcnt;
	
	for(int j = 1; j < condcnt; j++){
		
		Condition* cond = new Condition();
		cond->hasBop = true;
		cond->bop = clang::BO_LAnd;
		cond->leftChild = myFuncLogInfo[index].myInfo[i].cond;
		
		Condition* rc = new Condition();
		rc->assign(conditions[j]);
		cond->rightChild = rc;
		
		myFuncLogInfo[index].myInfo[i].cond = cond;
		myFuncLogInfo[index].myInfo[i].condstr[j] = expr2str(condexpr[j], GET_EXPANSION_LOC);
		myFuncLogInfo[index].myInfo[i].iselse[j] = iselse[j];
	}
	
	myFuncLogInfo[index].myInfo[i].cond->adjust();
	
	if(DEBUG){myFuncLogInfo[index].myInfo[i].cond->print();llvm::errs()<<"\n";}
}

void RecordCondVisitor::searchLoggingBlock(CFGBlock* cfgBlock, int checktimes){
	
	/*check before working*/
	if(!cfgBlock)
		return;
	if(checktimes >= 3)
		return;
	if(cfgBlock->succ_size() == 0)
		return;
	
	LangOptions LO;
	if(DEBUG_CFG_DUMP){
		llvm::errs()<<"blockID:"<<cfgBlock->getBlockID()<<"\n";
		cfgBlock->dump(cfgBlock->getParent(), LO, 0);
	}
	
	/*deal with conditionalOperator within the logging statement*/
	if(cfgBlock->getTerminator()){
		if(dyn_cast<ConditionalOperator>(cfgBlock->getTerminator().getStmt())){
			CFGBlock::succ_iterator ite = cfgBlock->succ_begin();
			CFGBlock *condTrue = *ite;
			ite++;
			if(ite != cfgBlock->succ_end()){
				CFGBlock *condFalse = *ite;
				if(condTrue && condFalse){
					CFGBlock::succ_iterator ite1 = condTrue->succ_begin();
					CFGBlock::succ_iterator ite2 = condFalse->succ_begin();
					
					if(ite1 && ite2 && *ite1 == *ite2){
						searchLoggingBlock(condTrue, checktimes);
						return;
					}
				}
			}
		}
	}
	
	/*get branch condition*/
	bool newCondition = false; //used for donating the flag whether new condition is added to vector
	if(cfgBlock->succ_size() > 1) {
		if(Stmt* stmt = cfgBlock->getTerminatorCondition()) {
			if(Expr* expr = dyn_cast<Expr>(stmt)){
				Condition* cond = new Condition();
				analyzeCond(expr, cond);
				//if(DEBUG)expr->dump();
				cond->adjust();
				conditions[condcnt] = cond;
				condexpr[condcnt] = expr;
				iselse[condcnt] = false;
				condcnt++;
				newCondition = true;
			}
		}
	}
	
	int path = 0;
	for(CFGBlock::succ_iterator ite = cfgBlock->succ_begin(); ite != cfgBlock->succ_end(); ++ite, path++){
		CFGBlock *succ = *ite;
		if(!succ)continue;
		
		if(DEBUG_CFG_DUMP){	
			succ->dump(succ->getParent(), LO, 0);
		}
		
		//deal with else path, the condition should be added prefix '!'.
		if(newCondition){
			conditions[condcnt - 1]->isInElse = false;
			if(cfgBlock->getTerminator() && !dyn_cast<SwitchStmt>(cfgBlock->getTerminator())){
				conditions[condcnt - 1]->isInElse = path?true:false;
				iselse[condcnt - 1] = conditions[condcnt - 1]->isInElse;
				
				if(conditions[condcnt - 1]->isInElse == true && conditions[condcnt - 1]->hasUop1 && conditions[condcnt - 1]->uop1 == clang::UO_LNot){
					conditions[condcnt - 1]->isInElse = false;
					conditions[condcnt - 1]->hasUop1 = conditions[condcnt - 1]->hasUop2;
					conditions[condcnt - 1]->uop1 = conditions[condcnt - 1]->uop2;
				}
				else if(conditions[condcnt - 1]->isInElse == true && conditions[condcnt - 1]->hasBop && conditions[condcnt - 1]->bop == clang::BO_NE){
					conditions[condcnt - 1]->isInElse = false;
					conditions[condcnt - 1]->bop= clang::BO_EQ;
				}
				else if(conditions[condcnt - 1]->isInElse == true && conditions[condcnt - 1]->hasBop && conditions[condcnt - 1]->bop == clang::BO_EQ){
					conditions[condcnt - 1]->isInElse = false;
					conditions[condcnt - 1]->bop= clang::BO_NE;
				}
			}
			
		}
		
		//TODO switch
		/*if(succ->getLabel()){
			if(CaseStmt* caseStmt = dyn_cast<CaseStmt>(succ->getLabel())) {
				Stmt::child_iterator it = caseStmt->child_begin();
				Stmt* sstmt = *it;
				if(Expr* expr = dyn_cast<Expr>(sstmt)){
					expr->dump();
				}
				BinaryOperator;
			}
			else if(DefaultStmt* defaultStmt = dyn_cast<DefaultStmt>(succ->getLabel())) {
				
			}
		}*/

		/*search for log statement, then add condition and argument*/
		bool findLoggingFunc = false;
		for(CFGBlock::iterator iterator = succ->begin(); iterator != succ->end(); ++iterator){
			
			CFGElement *cfgelement;
			cfgelement = &(*iterator);
			
			if(cfgelement->getKind() != 0) continue;
			CFGStmt cfgstmt = cfgelement->castAs<CFGStmt>();
			Stmt *stmt = const_cast<Stmt*>(cfgstmt.getStmt());
			if(stmt==NULL) continue;
			
			if(CallExpr* callExpr = dyn_cast<CallExpr>(stmt)) {
				if(callExpr->getDirectCallee()) {
					string logname = callExpr->getDirectCallee()->getQualifiedNameAsString();
					FullSourceLoc logloc = CI->getASTContext().getFullLoc(callExpr->getLocStart());
					//FullSourceLoc callloc = CI->getASTContext().getFullLoc(funcCall->getLocStart());
					int index = myFuncLogInfoMap[funcName];
					if(logname != "" && index && logloc.isValid()){
						int i = myFuncLogInfo[index].findTodoLog(funcLocLine, logname, InFile, logloc.getExpansionLineNumber());
						//if(funcName == "apr_file_write_full"){
						//	llvm::outs()<<"index"<<index<<" i"<<i<<"\n";
						//	llvm::outs()<<callloc.getExpansionLineNumber()<<"\n";
						//}
						if(i != -1 && condcnt != 0) {
							//callloc.dump();
							findLoggingFunc = true;
							myFuncLogInfo[index].myInfo[i].isTodo = false;
							addCondition(index, i);
							addArgument(index, i);
						}
					}
				}
			}
		}// end for in
		
		if(findLoggingFunc)
			continue;
		else {
			if(succ->succ_size() == 1)
				searchLoggingBlock(succ, checktimes);
			else if(succ->succ_size() > 1)
				searchLoggingBlock(succ, checktimes + 1);
		}
	}// end for out
	
	if(cfgBlock->succ_size() > 1) {
		if(cfgBlock->getTerminatorCondition()) {
			condcnt--;
		}
	}
	
}

bool RecordCondVisitor::findLoggedFunction(Stmt* stmt){	
	
	if(BinaryOperator* binaryOperator = dyn_cast<BinaryOperator>(stmt)){
		if(binaryOperator->getOpcode() == BO_Assign){
			//binaryOperator->dump();
			Expr* lv = binaryOperator->getLHS();
			Expr* rv = binaryOperator->getRHS();	
			
			rv = rv->IgnoreImpCasts();
			rv = rv->IgnoreImplicit();
			rv = rv->IgnoreParens();
			
			if(CallExpr* callExpr = dyn_cast<CallExpr>(rv)){
				if(callExpr->getDirectCallee()) {
					string funcname = callExpr->getDirectCallee()->getQualifiedNameAsString();
					int index = myFuncLogInfoMap[funcname];
					FullSourceLoc fullSourceLoc = CI->getASTContext().getFullLoc(callExpr->getLocStart());
					if(index && fullSourceLoc.isValid()){
						int i = myFuncLogInfo[index].findLogIndex(InFile, fullSourceLoc.getExpansionLineNumber());
						if(i != -1){
							retName = expr2str(lv, GET_EXPANSION_LOC);
							funcName = funcname;
							//llvm::errs()<<funcname<<"\n";
							funcLocLine = fullSourceLoc.getExpansionLineNumber();
							retType = callExpr->getCallReturnType(CI->getASTContext());
							funcCall = callExpr;
							return true;
						}
					}
				}
			}
		}
	} 
	
	if(DeclStmt* declStmt = dyn_cast<DeclStmt>(stmt)) {
		for(DeclStmt::decl_iterator it = declStmt->decl_begin(); it != declStmt->decl_end(); ++it){
			if(Decl *child = *it){
				if(VarDecl* varDecl = dyn_cast<VarDecl>(child)){
					Expr* expr = varDecl->getInit();
					if(expr) {
						expr = expr->IgnoreImpCasts();
						expr = expr->IgnoreImplicit();
						expr = expr->IgnoreParens();
						if(CallExpr* callExpr = dyn_cast<CallExpr>(expr)){
							if(callExpr->getDirectCallee()) {
								string funcname = callExpr->getDirectCallee()->getQualifiedNameAsString();
								int index = myFuncLogInfoMap[funcname];
								FullSourceLoc fullSourceLoc = CI->getASTContext().getFullLoc(callExpr->getLocStart());
								if(index && fullSourceLoc.isValid()){
									int i = myFuncLogInfo[index].findLogIndex(InFile, fullSourceLoc.getExpansionLineNumber());
									if(i != -1){
										retName = varDecl->getNameAsString();
										funcName = funcname;
										funcLocLine = fullSourceLoc.getExpansionLineNumber();
										retType = callExpr->getCallReturnType(CI->getASTContext());
										funcCall = callExpr;
										return true;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	
	if(CallExpr* callExpr = dyn_cast<CallExpr>(stmt)){
		if(callExpr->getDirectCallee()) {
			string funcname = callExpr->getDirectCallee()->getQualifiedNameAsString();
			
			//if(funcname == "apr_file_write_full") {
			//	FullSourceLoc FullLocation = CI->getASTContext().getFullLoc(callExpr->getLocStart());
			//	llvm::outs()<<"apr_file_write_full @ " << FullLocation.getExpansionLoc().printToString(FullLocation.getManager()) <<"\n";
			//}
			
			int index = myFuncLogInfoMap[funcname];
			
			FullSourceLoc fullSourceLoc = CI->getASTContext().getFullLoc(callExpr->getLocStart());
			if(index && fullSourceLoc.isValid()){
				//llvm::errs()<<funcname<<" "<<index<<"\n";
				//myFuncLogInfo[index].print();
				int i = myFuncLogInfo[index].findLogIndex(InFile, fullSourceLoc.getExpansionLineNumber());
				if(i != -1){
					retName = "";
					funcName = funcname;
					funcLocLine = fullSourceLoc.getExpansionLineNumber();
					retType = callExpr->getCallReturnType(CI->getASTContext());
					funcCall = callExpr;
					return true;
				}
			}
		}
	}
	
		
	for (Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it) {
		if(Stmt *child = *it)
			 return findLoggedFunction(child);
	}
	
	return false;	
}

void RecordCondVisitor::travelStmtForCall(Stmt* stmt){

	if(CallExpr* callExpr = dyn_cast<CallExpr>(stmt)){
		if(callExpr->getDirectCallee()) {
			string funcname = callExpr->getDirectCallee()->getQualifiedNameAsString();
			int index = myFuncLogInfoMap[funcname];
			if(index){
					
				Argument* arg = new Argument();
				unsigned int cnt;
				cnt = callExpr->getNumArgs();
				arg->argc = cnt;
				arg->str = expr2str(callExpr);
				
				for(unsigned int i = 0; i < cnt; i++){
					if(i == 20)break;
					Expr* argv_expr = callExpr->getArg(i);
					analyzeArgu(arg, argv_expr, i);
				}
				
				Call myCall(arg, callExpr);
				myFuncLogInfo[index].myCall.push_back(myCall);
			}
		}
	}
	
	for(Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
		if(Stmt *child = *it)
			travelStmtForCall(child);
	}
}

bool RecordCondVisitor::VisitFunctionDecl (FunctionDecl* Declaration){
	
	if(!(Declaration->isThisDeclarationADefinition() && Declaration->hasBody()))
		return true;
	
	FullSourceLoc FullLocation = CI->getASTContext().getFullLoc(Declaration->getLocStart());
	
	if(!FullLocation.isValid())
		return true;
	
	//the function is located in head file
	if(FullLocation.getExpansionLoc().getFileID() != CI->getSourceManager().getMainFileID())
		return true;

	if(DEBUG){
		llvm::errs()<<"Found function "<<Declaration->getQualifiedNameAsString() ;
		llvm::errs()<<" @ " << FullLocation.printToString(FullLocation.getManager()) <<"\n";
		string debug_func = DEBUG_FUNC;
		if(debug_func != "" && Declaration->getQualifiedNameAsString() != debug_func)
			return true;
		//Declaration->dump();
	}
	
	//Declaration->getQualifiedNameAsString() == DEBUG_FUNC ?	mydebug = true : mydebug = false;
	//if(mydebug)Declaration->dump();
	
	if(Stmt* function = Declaration->getBody()){
		
		/*record all callexpr and get argument*/
		travelStmtForCall(function);	
		
		//function->dump();
		const clang::CFG::BuildOptions *BO = new clang::CFG::BuildOptions();
		LangOptions LO;
		std::unique_ptr<CFG> funcCFG = CFG::buildCFG(Declaration, function, &(Declaration->getASTContext()), *BO);
		if(mydebug)funcCFG->dump(LO, false);
		CFG::iterator it;
		for(it = funcCFG->end() - 1; it >= funcCFG->begin(); --it){
			CFGBlock* cfgblock = *it;
			bool isLoggedBlock = false;
			
			CFGBlock::iterator iterator;
			for(iterator = cfgblock->begin(); iterator != cfgblock->end(); ++iterator){
				CFGElement *cfgelement;
				cfgelement = &(*iterator);
				CFGStmt *cfgstmt;
				cfgstmt = (CFGStmt*)malloc(sizeof(CFGStmt));
				if(cfgelement->getKind() == 0){
					 *cfgstmt = cfgelement->castAs<CFGStmt>();
					Stmt *stmt = const_cast<Stmt*>(cfgstmt->getStmt());
					if(stmt==NULL) continue;
					if(findLoggedFunction(stmt))
						isLoggedBlock = true;
				}
			}
			
			if(isLoggedBlock){
				if(DEBUG)llvm::errs()<<"\n***"<<funcName<<"***"<<retName<<"\n";
				//FullSourceLoc loc = CI->getASTContext().getFullLoc(funcCall->getLocStart());
				//if(funcName == "apr_file_write_full")llvm::outs()<<"\n***"<<funcName<<"***"<<retName<<"***"<<loc.getExpansionLineNumber()<<"\n";
				cntfunc++;
				condcnt = 0;
				searchLoggingBlock(cfgblock, 0);
				if(DEBUG){
					//int index = myFuncLogInfoMap[funcName];
					//myFuncLogInfo[index].print();		
				}
			}
		}
	}
	return true;
}

void RecordCondConsumer::HandleTranslationUnit(ASTContext& Context) {
	Visitor.TraverseDecl(Context.getTranslationUnitDecl());
	return;
}

std::unique_ptr<ASTConsumer> RecordCondAction::CreateASTConsumer(CompilerInstance& Compiler, StringRef InFile){
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
		return std::unique_ptr<ASTConsumer>(new RecordCondConsumer(&Compiler, InFile));
	}
}

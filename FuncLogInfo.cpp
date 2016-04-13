#include "FuncLogInfo.h"

int q[10000];
unsigned int qcnt;
int flag[10000];

extern int cntulog;
extern int cntlog;

void myfree(Condition* c){
	if(c->leftChild)
		myfree(c->leftChild);
	if(c->rightChild)
		myfree(c->rightChild);
	if(c)
		delete c;
}

void Argument::print(){
	llvm::errs()<<"Constant argument: ";
	for(int i = 0; i < argc; i++){
		for(int j = 0; j < argvcnt[i]; j++){
			llvm::errs()<<argv[i][j];
			if(j != argvcnt[i] - 1)
				llvm::errs()<<"\\";
		}
		if(i != argc - 1)
			llvm::errs()<<",";
	}
	if(argc == 0)
		llvm::errs()<<"NOPE";
	llvm::errs()<<"\n";
}

bool Condition::equals(Condition* c, int flag){
	Condition* a = this;
	Condition* b = c;
	if(a->hasBop != b->hasBop || a->hasUop1 != b->hasUop1 || a->hasUop2 != b->hasUop2 || a->isLeaf != b->isLeaf)
		return false;
	if(a->hasUop1 && b->hasUop1 && a->uop1 != b->uop1)
		return false;
	if(a->hasUop1 && b->hasUop1 && a->hasUop2 && b->hasUop2 && a->uop2 != b->uop2)
		return false;
	if(a->isInElse != b->isInElse)
		return false;
	
	if(a->isLeaf && a->dataType != b->dataType)
		return false;
	else if(a->isLeaf && a->dataType == b->dataType){
		switch(a->dataType){
			case(TYPE_INT):
				return a->ival == b->ival;
				break;
			case(TYPE_CHAR):
				return a->ival == b->ival;
				break;
			case(TYPE_STRING):
				return a->sval == b->sval;
				break;
			case(TYPE_FLOAT):
				return a->fval == b->fval;
				break;
			case(TYPE_ARRAY):
				return a->sval == b->sval;
				break;
			case(TYPE_MEMBER):
				return a->sval == b->sval;
				break;
			case(TYPE_ENUM):
				return a->sval == b->sval;
				break;
			default:
				return true;
				break;
		}
	}
	
	if(a->hasBop && a->bop != b->bop && a->bop != BO_GT && a->bop != BO_GE && a->bop != BO_LT && a->bop != BO_LE)
		return false;
	if(a->hasBop) {
		if(((a->bop == BO_GT || a->bop == BO_GE) && (b->bop == BO_LT || b->bop == BO_LE))
			|| ((a->bop == BO_LT || a->bop == BO_LE) && (b->bop == BO_GT || b->bop == BO_GE))){
			
			if(a->leftChild->equals(b->rightChild, flag) && a->rightChild->equals(b->leftChild, flag))
				return true;
		}
		else if(((a->bop == b->bop)  && a->bop == BO_Add)
			|| ((a->bop == b->bop)  && a->bop == BO_Mul)
			|| ((a->bop == b->bop)  && a->bop == BO_LAnd)
			|| ((a->bop == b->bop)  && a->bop == BO_LOr)
			|| ((a->bop == b->bop)  && a->bop == BO_Xor)
			|| ((a->bop == b->bop)  && a->bop == BO_And)
			|| ((a->bop == b->bop)  && a->bop == BO_Or)) {
			
			if(a->leftChild->equals(b->leftChild, flag) && a->rightChild->equals(b->rightChild, flag))
				return true;
			if(a->leftChild->equals(b->rightChild, flag) && a->rightChild->equals(b->leftChild, flag))
				return true;
		}
		else if(a->bop == b->bop){
			if(a->leftChild->equals(b->leftChild, flag) && a->rightChild->equals(b->rightChild, flag))
				return true;
		}
	}
	
	return false;
}

void Condition::assign(Condition* c){
	
	this->isInElse = c->isInElse;
	
	this->hasBop = c->hasBop;
	this->bop = c->bop;
	this->hasUop1 = c->hasUop1;
	this->uop1 = c->uop1;
	this->hasUop2 = c->hasUop2;
	this->uop2 = c->uop2;
	
	this->dataType = c->dataType;
	this->isLeaf = c->isLeaf;
	
	this->fval = c->fval;
	this->sval = c->sval;
	this->ival = c->ival;
	
	this->leftChild = c->leftChild;
	this->rightChild = c->rightChild;
	
	
	
// 		if(this->hasBop){
// 		Condition* lc = new Condition();
// 		lc->assign(c->leftChild);
// 		Condition* rc = new Condition();
// 		rc->assign(c->rightChild);
}

void Condition::simplify(){
	
	if(hasUop1 && uop1 == clang::UO_LNot && hasUop2 && uop2 == clang::UO_LNot){
		hasUop1 = false;
		hasUop2 = false;
	}
	
	if(isInElse && hasUop1 && uop1 == clang::UO_LNot){
		hasUop1 = hasUop2;
		uop1 = uop2;
		isInElse = false;
	}
	
	if(isInElse){
		hasUop2 = hasUop1;
		uop2 = uop1;
		hasUop1 = true;
		uop1 = clang::UO_LNot;
		isInElse = false;
	}
	
	if(this->hasBop){
		
		if(this->leftChild)
			this->leftChild->simplify();
		if(this->rightChild)
			this->rightChild->simplify();
		
		Condition* notLeft = new Condition();
		notLeft->assign(this->leftChild);
		if(notLeft->hasUop1 && notLeft->uop1 == clang::UO_LNot){
			notLeft->hasUop1 = notLeft->hasUop2;
			notLeft->uop1 = notLeft->uop2;
		}
		else{
			notLeft->hasUop2 = notLeft->hasUop1;
			notLeft->uop2 = notLeft->uop1;
			notLeft->hasUop1 = true;
			notLeft->uop1 = clang::UO_LNot;
		}
		
		if(BinaryOperator::getOpcodeStr(this->bop) == "="){
			this->assign(this->rightChild);
		}
		else if(BinaryOperator::getOpcodeStr(this->bop) == "||"){
			
			if(this->leftChild->equals(this->rightChild, 0)){
				this->assign(this->leftChild);
			}
			//else if(notLeft->equals(this->rightChild)){
				//TODO condition is "1"
			//}
			
			else if(this->leftChild->dataType == TYPE_UNKNOW && this->rightChild->dataType == TYPE_UNKNOW){
				this->dataType = TYPE_UNKNOW;
			}
			else if(this->leftChild->dataType == TYPE_UNKNOW){
				this->assign(this->rightChild);
			}
			else if(this->rightChild->dataType == TYPE_UNKNOW){
				this->assign(this->leftChild);
			}
			else if(this->leftChild->dataType == this->rightChild->dataType){
				this->dataType = this->leftChild->dataType;
			}
			else{
				this->dataType = TYPE_KNOW;
			}
		}
		else if(BinaryOperator::getOpcodeStr(this->bop) == "&&"){
			if(this->leftChild->equals(this->rightChild, 0)){
				this->assign(this->leftChild);
			}
			//else if(notLeft->equals(this->rightChild)){
				//TODO condition is "0"
			//}
		}
		else{
			if(this->leftChild->dataType == TYPE_UNKNOW || this->rightChild->dataType == TYPE_UNKNOW){
				this->dataType = TYPE_UNKNOW;
			}
			else if(this->leftChild->dataType == this->rightChild->dataType){
				this->dataType = this->leftChild->dataType;
			}
			else{
				this->dataType = TYPE_KNOW;
			}
		}
	}
	return;	
}

void Condition::adjust(){	
	if(this->hasBop){
		
		if(this->leftChild)
			this->leftChild->adjust();
		if(this->rightChild)
			this->rightChild->adjust();
		
		if(BinaryOperator::getOpcodeStr(this->bop) == "="){
			if(this->rightChild->dataType == TYPE_CALL || this->rightChild->dataType == TYPE_RET){
				this->leftChild->dataType = TYPE_RET;
			}
			else if(this->rightChild->dataType == TYPE_UNKNOW){
				this->leftChild->dataType = TYPE_UNKNOW;
			}
			else{
				this->leftChild->dataType = TYPE_KNOW;
			}
			this->dataType = this->leftChild->dataType;
		}
		else if(BinaryOperator::getOpcodeStr(this->bop) == "||"){
			if(this->leftChild->dataType == TYPE_UNKNOW && this->rightChild->dataType == TYPE_UNKNOW){
				this->dataType = TYPE_UNKNOW;
			}
			else if(this->leftChild->dataType == TYPE_UNKNOW){
				this->dataType = this->rightChild->dataType;
			}
			else if(this->rightChild->dataType == TYPE_UNKNOW){
				this->dataType = this->leftChild->dataType;
			}
			else if(this->leftChild->dataType == this->rightChild->dataType){
				this->dataType = this->leftChild->dataType;
			}
			else{
				this->dataType = TYPE_KNOW;
			}
		}
		else{
			if(this->leftChild->dataType == TYPE_UNKNOW || this->rightChild->dataType == TYPE_UNKNOW){
				this->dataType = TYPE_UNKNOW;
			}
			else if(this->leftChild->dataType == this->rightChild->dataType){
				this->dataType = this->leftChild->dataType;
			}
			else{
				this->dataType = TYPE_KNOW;
			}
		}
	}
	return;
}

void Condition::print(){
	//TODO UO_PostInc, UO_PostDec
	
	//llvm::outs()<<hasUop1<<" "<<hasUop2<<"\n";
	
	if(this->isInElse){
		llvm::errs()<<"(!)";
	}
	if(this->hasUop1){
		llvm::errs()<<UnaryOperator::getOpcodeStr(this->uop1)<<"(";
	}
	if(this->hasUop1 && this->hasUop2){
		llvm::errs()<<UnaryOperator::getOpcodeStr(this->uop2)<<"(";
	}
	
	if(this->hasBop){
		if(this->leftChild){
			llvm::errs()<<"(";
			leftChild->print();
			llvm::errs()<<")";
		}
		llvm::errs()<<" "<<BinaryOperator::getOpcodeStr(this->bop)<<" ";
		if(this->rightChild){
			llvm::errs()<<"(";
			rightChild->print();
			llvm::errs()<<")";
		}
	}
	
	switch(this->dataType){
		case(TYPE_UNKNOW):
			llvm::errs()<<"/*UNKNOWN*/";
			break;
		case(TYPE_KNOW):
			llvm::errs()<<"/*KNOWN*/";
			break;
		case(TYPE_INT):
			if(!this->hasBop)
				llvm::errs()<<this->ival;
			llvm::errs()<<"/*INT*/";
			break;
		case(TYPE_CHAR):
			if(!this->hasBop)
				llvm::errs()<<this->ival;
			llvm::errs()<<"/*CHAR*/";
			break;
		case(TYPE_ENUM):
			if(!this->hasBop)
				llvm::errs()<<this->sval;
			llvm::errs()<<"/*ENUM*/";
			break;
		case(TYPE_STRING):
			if(!this->hasBop)
				llvm::errs()<<this->sval;
			llvm::errs()<<"/*STRING*/";
			break;
		case(TYPE_FLOAT):
			if(!this->hasBop)
				llvm::errs()<<this->fval;
			llvm::errs()<<"/*FLOAT*/";
			break;
		case(TYPE_ARRAY):
			if(!this->hasBop)
				llvm::errs()<<this->sval;
			llvm::errs()<<"/*ARRAY*/";
			break;
		case(TYPE_MEMBER):
			if(!this->hasBop)
				llvm::errs()<<this->sval;
			llvm::errs()<<"/*MEMBER*/";
			break;
		case(TYPE_NULL):
			if(!this->hasBop)
				llvm::errs()<<this->sval;
			llvm::errs()<<"/*NULL*/";
			break;
		case(TYPE_RET):
			if(!this->hasBop)
				llvm::errs()<<this->sval;
			llvm::errs()<<"/*RET*/";
			break;
		case(TYPE_CALL):
			llvm::errs()<<"/*CALL*/";
			break;
		default:
			llvm::errs()<<"/*DEFAULT*/";
			break;
	}
	
	if(this->hasUop1 && this->hasUop2)
		llvm::errs()<<")";
	if(this->hasUop1)
		llvm::errs()<<")";
}

void Condition::printAsExpr(){
	
	if(this->dataType == TYPE_UNKNOW)return;
	
	if(this->isInElse)llvm::outs()<<"!(";
	if(this->hasUop1)llvm::outs()<<UnaryOperator::getOpcodeStr(this->uop1)<<"(";
	if(this->hasUop1 && this->hasUop2)llvm::outs()<<UnaryOperator::getOpcodeStr(this->uop2)<<"(";
	
		
	if(this->hasBop){
		if(this->leftChild){
			llvm::outs()<<"(";
			leftChild->printAsExpr();
			llvm::outs()<<")";
		}
		llvm::outs()<<" "<<BinaryOperator::getOpcodeStr(this->bop)<<" ";
		if(this->rightChild){
			llvm::outs()<<"(";
			rightChild->printAsExpr();
			llvm::outs()<<")";
		}
	}
	else {
		switch(this->dataType){
			case(TYPE_INT):
				llvm::outs()<<this->ival;
				break;
			case(TYPE_CHAR):
				llvm::outs()<<this->ival;
				break;
			case(TYPE_ENUM):
				llvm::outs()<<this->sval;
				break;
			case(TYPE_STRING):
				llvm::outs()<<this->sval;
				break;
			case(TYPE_FLOAT):
				llvm::outs()<<this->fval;
				break;
			case(TYPE_MEMBER):
				llvm::outs()<<this->sval;
				break;
			case(TYPE_ARRAY):
				llvm::outs()<<this->sval;
				break;
			case(TYPE_NULL):
				llvm::outs()<<this->sval;
				break;
			case(TYPE_RET):
				llvm::outs()<<"SL_RET";
				break;
			default:
				llvm::outs()<<"(ERROR"<<this->dataType<<")";
				break;
		}
	}
	
	if(this->hasUop1 && this->hasUop2)llvm::outs()<<")";
	if(this->hasUop1)llvm::outs()<<")";
	if(this->isInElse)llvm::outs()<<")";

}

FuncLogInfo::FuncLogInfo(){

}

FuncLogInfo::~FuncLogInfo(){

}

void FuncLogInfo::print(){
	llvm::errs()<<"Function Log Info: "<<this->funcName<<"\n";
	unsigned int i;
	for(i = 0; i < this->myInfo.size(); i++){
		llvm::errs()<<this->getFuncLocFile(i)<<":"<<this->getFuncLocLine(i)<<"#"<<
				this->getLogName(i)<<"@"<<this->getLogLocFile(i)<<":"<<this->getLogLocLine(i)<<"\n";
		if(myInfo[i].isTodo == false){
			if(this->getCond(i)->dataType == TYPE_UNKNOW)
				cntulog++;
			cntlog++;
			this->getCond(i)->print();
			llvm::errs()<<"\n";
			//llvm::outs()<<this->getCondstr(i);
		}
		llvm::errs()<<"\n";
	}
}

void FuncLogInfo::simplify(){
	for(unsigned int i = 0; i < this->myInfo.size(); i++) {
		
		if(myInfo[i].isTodo == true) 
			continue;
		
		myInfo[i].cond->simplify();
		if(this->getCond(i)->dataType == TYPE_UNKNOW)
			cntulog++;
		cntlog++;
	}
}

float FuncLogInfo::judgeArgu(int callID, int argID){
	
	int callNUm = 0;
	int logNum = 0;
	
	for(unsigned int i = 0; i < myCall.size(); i++){
		for(int j = 0; j < myCall[i].arg->argc; j++){
			if(myCall[i].arg->argvcnt[j]){
				bool isFound = false;
				for(int m = 0 ; m < myCall[callID].arg->argvcnt[argID]; m++){
					for(int n = 0; n < myCall[i].arg->argvcnt[j]; n++){
						if(myCall[callID].arg->argv[argID][m] == myCall[i].arg->argv[j][n]){
							//llvm::errs()<<myCall[callID].arg->argv[argID][m] <<" "<< myCall[i].arg->argv[j][n]<<"\n";
							isFound = true;
							break;
						}
					}
					if(isFound) break;
				}
				if(isFound) callNUm++;
			}
		}
	}
	//llvm::errs()<<"---\n";
	for(unsigned int i = 0; i < myInfo.size(); i++) {
		if(myInfo[i].isTodo == false )
		for(int j = 0; j < myInfo[i].arg->argc; j++){
			if(myInfo[i].arg->argvcnt[j]){
				bool isFound = false;
				for(int m = 0 ; m < myCall[callID].arg->argvcnt[argID]; m++){
					for(int n = 0; n < myInfo[i].arg->argvcnt[j]; n++){
						if(myCall[callID].arg->argv[argID][m] == myInfo[i].arg->argv[j][n]){
							//llvm::errs()<<myCall[callID].arg->argv[argID][m] <<" "<< myInfo[i].arg->argv[j][n]<<"\n";
							isFound = true;
							break;
						}
					}
					if(isFound) break;
				}
				if(isFound) logNum++;
			}
		}
	}
	//llvm::errs()<<"lognum "<<logNum<<" "<<callNUm<<"\n";
	if(callNUm == 0){
		llvm::errs()<<"compare error\n";
		return 0;
	}
	else
		return (float)logNum / (float)callNUm;
}

void FuncLogInfo::countArgu(){
	
	bool hasConstant = false;
	for(unsigned int i = 0; i < myCall.size(); i++){
		for(int j = 0; j < myCall[i].arg->argc; j++){
			if(myCall[i].arg->argvcnt[j]){
				hasConstant = true;
			}
			
		}
	}
	if(!hasConstant)
		return;
	
 	llvm::errs()<<"\nLogging Argument of Function: "<<this->funcName<<"\n";
	int cnt = 0;
	for(unsigned int i = 0; i < this->myInfo.size(); i++) {
		if(myInfo[i].isTodo == false)
			cnt++;
	}
	if(myCall.size() == 0){
		llvm::errs()<<"All not found!\n";
		return;
	}
	float logRatio = (float)myInfo.size() / (float)myCall.size();
	float recognizedLogRatio = (float)cnt / (float)myCall.size();
	llvm::errs()<<"Logging ratio is "<<logRatio<<"\n";
	llvm::errs()<<"Recognized logging ratio is "<<recognizedLogRatio<<"\n";
	for(unsigned int i = 0; i < myInfo.size(); i++){
		if(myInfo[i].isTodo == false)
			llvm::errs()<< myInfo[i].arg->str <<"\n";
	}
	llvm::errs()<<"---\n";
	for(unsigned int i = 0; i < myCall.size(); i++){
		llvm::errs()<< myCall[i].arg->str <<"\n";
	}
	llvm::errs()<<"---\n";
	
	map<string, int>argmap;
	map<string, int>funcgmap;
	argmap.clear();
	funcgmap.clear();
	for(unsigned int i = 0; i < myCall.size(); i++){
		for(int j = 0; j < myCall[i].arg->argc; j++){
			if(myCall[i].arg->argvcnt[j]){
				//llvm::errs()<<"judge "<<i<<" "<<j<<"\n";
				float arguLogRatio = judgeArgu(i, j);

				llvm::errs()<<"The logging ratio of argument ";
				for(int k = 0; k < myCall[i].arg->argvcnt[j]; k++){
					llvm::errs()<<myCall[i].arg->argv[j][k];
					if(k != myCall[i].arg->argvcnt[j] - 1)
						llvm::errs()<<"\\";
				}
				llvm::errs()<<" is "<<arguLogRatio<<"\n";
					
					
				if(arguLogRatio && arguLogRatio > recognizedLogRatio){
					string argstr;
					for(int k = 0; k < myCall[i].arg->argvcnt[j]; k++){
						argstr = argstr.append(myCall[i].arg->argv[j][k]);
						if(k != myCall[i].arg->argvcnt[j] - 1)
							argstr = argstr.append("\\");
					}
					if(!argmap[argstr]){
						argmap[argstr] = 1;
						if(!funcgmap[funcName]){
							funcgmap[funcName] = 1;
							llvm::outs()<<"2,"<<funcName<<","<<j<<","<<argstr<<","<<arguLogRatio<<","<<recognizedLogRatio<<"\n";
						}
						else
							llvm::outs()<<"3,"<<funcName<<","<<j<<","<<argstr<<","<<arguLogRatio<<","<<recognizedLogRatio<<"\n";
					}
				}
			}
			
		}
	}
	
}

void FuncLogInfo::countCond(int threshold){
	unsigned int i, j, k;
	int id = 1;
	for(i = 0; i < 10000; i++) flag[i] = 0, q[i] = 0;
	qcnt = 0;
	
	int cnt = 0;
 	llvm::errs()<<"\nLogging Condition of Function: "<<this->funcName<<"\n";
 	llvm::errs()<<"Total condition number: "<<myInfo.size()<<"\n";
	for(i = 0; i < this->myInfo.size(); i++) {
		
		if(myInfo[i].isTodo == true)continue;
		cnt++;
		if(flag[i] == 1)continue;
		qcnt = 0;
		q[qcnt++] = i;
		
		for(j = i + 1; j < this->myInfo.size(); j++) {
			if(myInfo[j].isTodo == true)continue;
			
			if(myInfo[i].cond->equals(myInfo[j].cond, 1)){
				flag[j] = 1;
				q[qcnt++] = j;
			}
		}
		
		llvm::errs()<<"Condition "<<id++<<" appears "<<qcnt<<" time(s):\n";
		for(k = 0; k < qcnt; k++) {
			llvm::errs()<<this->getFuncLocFile(q[k])<<":"<<this->getFuncLocLine(q[k])<<"#"<<
				this->getLogName(q[k])<<"@"<<this->getLogLocFile(q[k])<<":"<<this->getLogLocLine(q[k])<<"\n";
			if(myInfo[q[k]].isTodo == false){
				getCond(q[k])->print();
				llvm::errs()<<"\n";
				for(int ii = 0; ii < myInfo[q[k]].condcnt; ii++){
					
					llvm::errs()<<myInfo[q[k]].condstr[ii];	
					if(myInfo[q[k]].iselse[ii])
						llvm::errs()<<" (In else branch)";
					llvm::errs()<<"\n";
				}
				llvm::errs()<<"Call code: "<<myInfo[q[k]].arg->str<<"\n";
				myInfo[q[k]].arg->print();
				llvm::errs()<<"\n";
			}
		}
		
		Condition* c = myInfo[i].cond;
		if(qcnt >= 2){
// 			if(OUTPUT){
// 				if(c->dataType != TYPE_UNKNOW){
// 					llvm::outs()<<"Selected_known as an influencing condition"<<"("<<qcnt<<"times)"<<" of function "<<this->funcName<<" : ";
// 					c->printAsExpr();
// 					llvm::outs()<<"\n";
// 				}
// 				else {
// 					llvm::outs()<<"Selected_unknown as an influencing condition"<<"("<<qcnt<<"times)"<<" of function "<<this->funcName<<"\n";
// 				}
// 			}
// 			else{
				if(c->dataType != TYPE_UNKNOW) {
					llvm::outs()<<"1,"<<funcName<<",";
					c->printAsExpr();
					llvm::outs()<<","<<qcnt<<"\n";
				}
// 			}
		}
	}
 	llvm::errs()<<"Recognize condition number: "<<cnt<<"\n";
 	llvm::errs()<<"Unrecognize condition number: "<<myInfo.size() - cnt<<"\n";
}

void FuncLogInfo::setFuncName(string name){
	this->funcName = name;

}

string FuncLogInfo::getFuncName(){
	return this->funcName;
}

void FuncLogInfo::addLog(LogInfo logInfo){
	myInfo.push_back(logInfo);
}

string FuncLogInfo::getfile(string f){
	if(f.find_last_of('/') == string::npos)
		return f;
	else
		return f.substr(f.find_last_of('/')+1);	
}

int FuncLogInfo::findLogIndex(string fileName, int line){
	unsigned int i;
	for(i = 0; i < this->myInfo.size(); i++){
		//llvm::errs()<<getFuncLocFile(i)<<" "<<getFuncLocLine(i)<<"\n";
		if((getfile(this->getFuncLocFile(i)) == getfile(fileName)) && (this->getFuncLocLine(i) == line))
			return (int)i;
	}
	return -1;
}

int FuncLogInfo::findTodoLog(int funcline, string logname, string fileName, int line){
	unsigned int i;
	for(i = 0; i < this->myInfo.size(); i++){
		//llvm::errs()<<getFuncLocFile(i)<<" "<<getFuncLocLine(i)<<"\n";
		if((this->getFuncLocLine(i) == funcline) && (this->getLogName(i) == logname) && 
				(getfile(this->getLogLocFile(i)) == getfile(fileName)) && (this->getLogLocLine(i) == line)) {
			if(myInfo[i].isTodo == false)
				continue;
			return (int)i;
		}
	}
	return -1;
}

LogInfo FuncLogInfo::getMyLogInfo(int index){
	return this->myInfo[index];
}

void FuncLogInfo::getLog(int& i, string& name, string& file, int& line){
	name = this->getLogName(i);
	file = this->getFuncLocFile(i);
	line = this->getFuncLocLine(i);
}

string FuncLogInfo::getFuncLocFile(int index){
	return myInfo[index].funcLocFile;
}

int FuncLogInfo::getFuncLocLine(int index){
	return this->myInfo[index].funcLocLine;
}

string FuncLogInfo::getLogName(int index){
	return this->myInfo[index].logName;
}

string FuncLogInfo::getLogLocFile(int index){
	return this->myInfo[index].logLocFile;
}

int FuncLogInfo::getLogLocLine(int index){
	return this->myInfo[index].logLocLine;
}

Condition* FuncLogInfo::getCond(int index){
	return this->myInfo[index].cond;
}


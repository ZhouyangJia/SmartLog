#include "FuncCondInfo.h"

void FuncCondInfo::setFuncName(string name){
	funcName = name;
}

void FuncCondInfo::addCond(string cond, int time){
	funcCond[condcnt] = cond;
	condTime[condcnt] = time;
	isChosen[condcnt] = true;
	condcnt++;
}

void FuncCondInfo::print(){
	llvm::outs()<<"Function name: "<<funcName<<"\n";
	for(int i = 0; i < condcnt; i++){
		llvm::outs()<<funcCond[i]<<" "<<condTime[i]<<" "<<(isChosen[i]?"TRUE":"FALSE")<<"\n";
	}
}

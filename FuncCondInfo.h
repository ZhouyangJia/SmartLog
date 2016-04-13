#ifndef FUNCCONDINFO_H
#define FUNCCONDINFO_H

#include <map>
#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>


#include "llvm/Support/raw_ostream.h"
#include "clang/AST/Expr.h"

using namespace std;
using namespace clang;
using namespace llvm;

class FuncCondInfo{
public:
	FuncCondInfo(){
		string funcName = "";
		condcnt = 0;
	};
	
	string funcName;
	string funcCond[100];
	int condTime[100];
	int isChosen[100];
	int condcnt;	
	
	
	void setFuncName(string);
	void addCond(string, int);
	
	void print();
};

#endif

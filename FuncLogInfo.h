#ifndef FUNCLOGINFO_H
#define FUNCLOGINFO_H

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

#define TYPE_UNKNOW 100
#define TYPE_INT 101
#define TYPE_CHAR 102
#define TYPE_STRING 103
#define TYPE_FLOAT 104
#define TYPE_ARRAY 105
#define TYPE_CALL 106
#define TYPE_RET 107
#define TYPE_KNOW 108
#define TYPE_MEMBER 109
#define TYPE_ENUM 110
#define TYPE_NULL 111


#define OUTPUT 0

class Argument{
public:
	
	Argument(){
		
		argc = 0;
		for(int i = 0; i < 100; i++){
			argvcnt[i] = 0;
			for(int j = 0; j < 100; j++){
				argv[i][j] = "";
			}
		}
	};
	
	int argc;
	int argvcnt[100]; //assume that the number of argument is less than 20
	string argv[100][100]; //each argv may contain multiply influencing argument, assume less than 10
	
	string str;
	void print();
	
};

class Condition{
public:
	Condition(){
		isInElse = false;
		hasUop1 = false;
		hasUop2 = false;
		hasBop = false;
		
		isLeaf = false;
		dataType = 0;
		
		sval = "";
		ival = 0;
		fval = 0.0;
	};
	
	bool isInElse;
	
	bool hasUop1;
	UnaryOperator::Opcode uop1;
	bool hasUop2;
	UnaryOperator::Opcode uop2;
	bool hasBop;
	BinaryOperator::Opcode bop;
	
	bool isLeaf;
	int dataType;
	
	string sval;
	double fval;
	int ival;
	
	Condition *leftChild;
	Condition *rightChild;
	
	bool equals(Condition* c, int);
	void print();
	void printAsExpr();
	void adjust();
	void simplify();
	void assign(Condition* c);
};
void myfree(Condition*);

class Call{
public:	
	Call(Argument* a, CallExpr* c): arg(a), call(c){};
	Argument* arg;
	CallExpr* call;
};


class LogInfo{
public:
	LogInfo(string funcLocFile, int funcLocLine, string logName, string logLocFile, int logLocLine):
			funcLocFile(funcLocFile), funcLocLine(funcLocLine), logName(logName), logLocFile(logLocFile), logLocLine(logLocLine)
			{isTodo = true; condcnt = 0;}
	string funcLocFile;
	int funcLocLine;
	string logName;
	string logLocFile;
	int logLocLine;
	
	/*isTodo = false means its condition has been analysis*/
	bool isTodo;
	
	Condition* cond;
	string condstr[5];
	bool iselse[5];
	int condcnt;
	
	Argument* arg;
	CallExpr* call;
	
};

class FuncLogInfo {
public:
	FuncLogInfo();
	~FuncLogInfo();
	
	void addLog(LogInfo);
	
	int findLogIndex(string fileName, int line);
	int findTodoLog(int funcline, string logname, string fileName, int line);
	
	void setFuncName(string);
	string getFuncName();
	
	//all kinds of get methods
	LogInfo getMyLogInfo(int index);
	void getLog(int&, string&, string&, int&);
	string getFuncLocFile(int index);
	int getFuncLocLine(int index);
	string getLogName(int index);
	string getLogLocFile(int index);
	int getLogLocLine(int index);
	Condition* getCond(int index);
	string getfile(string);
	
	void print();
	void simplify();
	void countCond(int);
	
	void countArgu();
	float judgeArgu(int callID, int argID);
	
	string funcName;
	vector<LogInfo> myInfo;
	vector<Call> myCall;
	
};

#endif

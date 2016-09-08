//===- FindLoggingBehavior.cpp - Find logging statement and behavior automatically -===//
//
//                     SmartLog Logging Tool
//
// Author: Zhouyang Jia, PhD Candidate
// Affiliation: School of Computer Science, National University of Defense Technology
// Email: jiazhouyang@nudt.edu.cn
//
//===----------------------------------------------------------------------===//
//
// This file implements FindLoggingAction, FindLoggingConsumer, FindLoggingVisitor, FunctionFeat
// and LogBehavior to find logging statements and normalized logging behavior.
// Command: /path/to/clang-smartlog -find-logging-behaivor /path/to/file.c
// The results are written in logging_statement.out, logging_behavior.out and
// normalizes_behavior.out.
//
//===----------------------------------------------------------------------===//


#include "SmartLog.h"
#include "FindLoggingFunction.h"
#include "Python.h"

extern string lastName;
extern int fileNum;
extern int fileTotalNum;

extern FunctionFeat myFunctionFeat[MAX_FUNC_NUM];
extern map<string, int> myFunctionFeatMap;
extern int myFunctionFeatCnt;

extern map<string, int>myCallDepMap;
extern vector<LogBehavior> myLogBehavior;

extern FILE* fCallStmt;
extern FILE* fLogStmt;
extern FILE* fLogBehavior;

extern PyObject *pName,*pModule,*pDict,*pFunc,*pResult,*pTmp;

extern map<BinaryOperator::Opcode, bool> BO_com_map;
extern map<BinaryOperator::Opcode, bool> BO_seq_map;
extern map<BinaryOperator::Opcode, bool> BO_swi_map;

map<CallExpr*,string>mapFuncRet;
extern map<string, int>funcMap;

//===----------------------------------------------------------------------===//
//  Extract and regular logging behavior
//===----------------------------------------------------------------------===//

/// Get source code from a given statement
/// Input: statement
/// Outout: text
string LogBehavior::getText(Stmt* s){

    if(!s)
        return "";

    FullSourceLoc begin = CI->getASTContext().getFullLoc(s->getLocStart());
    FullSourceLoc end = CI->getASTContext().getFullLoc(s->getLocEnd());

    if(begin.isInvalid() || end.isInvalid())
        return "";

    SourceRange sr(begin.getExpansionLoc(), end.getExpansionLoc());

    return Lexer::getSourceText(CharSourceRange::getTokenRange(sr), CI->getSourceManager(), LangOptions(), 0);
}

/// Get at most 2 binaryoperator
/// Each stmt is a child of || or &&
/// We only deal with simple statements with at most 2 BOs (without || and &&)
/// Inout: statement without || and &&
/// Output: bo1 and bo2
bool LogBehavior::getAtMost2BO(Stmt* stmt){

    if(stmt == nullptr)
        return true;

    if(dyn_cast<CallExpr>(stmt) || dyn_cast<StmtExpr>(stmt) || dyn_cast<ConditionalOperator>(stmt)){
        return true;
    }

    if(dyn_cast<ArraySubscriptExpr>(stmt) || dyn_cast<MemberExpr>(stmt)){
        return true;
    }

    // In case of 2+3, we get 5 directly instead of search inside
    if(Expr *expr = dyn_cast<Expr>(stmt)){
        if(expr->isIntegerConstantExpr(CI->getASTContext())){
            return true;
        }
    }

    if(BinaryOperator* bo = dyn_cast<BinaryOperator>(stmt)){
        if(bo1 == nullptr)
            bo1 = bo;
        else if(bo2 == nullptr)
            bo2 = bo;
        else
            return false;
    }

    for (Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
        if(Stmt *child = *it)
            if(getAtMost2BO(child) == false)
                return false;
    }
    return true;
}

/// Regular leaf node to text
/// Input: expr without BO
/// Output: text of expression
string LogBehavior::getLeafText(Expr* expr){

    bool debug = 0;

    expr = expr->IgnoreImpCasts()->IgnoreImplicit()->IgnoreParens()->IgnoreCasts()->ignoreParenBaseCasts();

    string res = "";
    string prefix = "";

    if(debug)expr->dump();

    // Append prefix
    while(UnaryOperator* uo = dyn_cast<UnaryOperator>(expr)){
        prefix.append(uo->getOpcodeStr(uo->getOpcode()));
        expr = uo->getSubExpr();
        expr = expr->IgnoreImpCasts()->IgnoreImplicit()->IgnoreParens()->IgnoreCasts();
    }

    if(debug)errs()<<"Prefix "<<prefix<<"\n";

    // Get call name
    string callname = "";
    if(callExpr->getDirectCallee())
        callname = callExpr->getDirectCallee()->getNameAsString();

    // Analyze node
    if(CallExpr* call = dyn_cast<CallExpr>(expr)){
        if(call == callExpr){
            callname.append("_0");
            return prefix.append(callname);
        }
    }
    // String expression
    else if(StringLiteral* sl = dyn_cast<StringLiteral>(expr)){
        string text = sl->getString();
        return prefix.append(text);
    }
    // Char expression
    else if(CharacterLiteral* cl = dyn_cast<CharacterLiteral>(expr)){
        char c = (char) cl->getValue();
        char chr[4];
        chr[0] = '\'';
        chr[1] = c;
        chr[2] = '\'';
        chr[3] = '\0';
        return prefix.append(chr);
    }
    // Constant expression
    else if(expr->isIntegerConstantExpr(CI->getASTContext())){
        APSInt aps;
        expr->EvaluateAsInt(aps, CI->getASTContext());
        return prefix.append(aps.toString(10));
    }

    // Variables including array and member
    string variable;
    if(ArraySubscriptExpr* ase = dyn_cast<ArraySubscriptExpr>(expr)){
        variable = getText(ase);
    }
    else if(MemberExpr* me = dyn_cast<MemberExpr>(expr)){
        variable = getText(me);
    }
    else if(DeclRefExpr* dre = dyn_cast<DeclRefExpr>(expr)){
        variable = getText(dre);
    }
    else{
        if(!dyn_cast<CallExpr>(expr))
            return "Unknown1";
    }

    if(debug)errs()<<"Variable "<<variable<<"\n";

    unsigned num = callExpr->getNumArgs();
    if(argIndex > (int) num)
        return "Error argument index";

    string argument = "";
    if(argType.compare("RV")){
        argument = getText(callExpr->getArg(argIndex - 1));
    }
    else{
        argument = mapFuncRet[callExpr];
    }

    if(debug)errs()<<"Argument "<<argument<<"\n";

    // No sensitive argument, maybe if(foo(a))
    if(argument != ""){
        if(variable.find(argument) == 0){
            string newvar = callname;
            newvar.append("_");
            if(argType.compare("RV")){
                char arg[10];
                sprintf(arg, "%d", argIndex);
                newvar.append(arg);
            }
            else{
                newvar.append("0");
            }
            newvar.append(variable.substr(argument.length()));
            return prefix.append(newvar);
        }
    }

    res.append(prefix);
    res.append(getText(expr));
    return res;
}

/// Get source text of a given BO
/// Input: textl, textr and op
/// Output: text of the BO
string LogBehavior::getExprRegular(string bo1l, string bo1r, BinaryOperator::Opcode bo1op){

    string res = "";

    // ret = foo() --> foo()
    if(bo1op == BO_Assign){
        return bo1r;
    }

    // foo() == 0 --> !foo()
    if(bo1op == BO_EQ){
        string tmp = bo1l;
        transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
        if(tmp == "0" || tmp.find("null") != string::npos){
            res = "!";
            res.append(bo1r);
            return res;
        }
        tmp = bo1r;
        transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
        if(tmp == "0" || tmp.find("null") != string::npos){
            res = "!";
            res.append(bo1l);
            return res;
        }
    }

    // foo() != 0 --> foo()
    if(bo1op == BO_NE){
        string tmp = bo1l;
        transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
        if(tmp == "0" || tmp.find("null") != string::npos){
            return bo1r;
        }
        tmp = bo1r;
        transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
        if(tmp == "0" || tmp.find("null") != string::npos){
            return bo1l;
        }
    }

    // b > a --> a < b
    if(BO_swi_map[bo1op] && bo1l.compare(bo1r) > 0){
        string opstr = "#";
        switch(bo1op){
            case BO_LT: // < --> >
                opstr = BinaryOperator::getOpcodeStr(BO_GT);
                break;
            case BO_GT: // > --> <
                opstr = BinaryOperator::getOpcodeStr(BO_LT);
                break;
            case BO_LE: // <= --> >=
                opstr = BinaryOperator::getOpcodeStr(BO_GE);
                break;
            case BO_GE: // >= --> <=
                opstr = BinaryOperator::getOpcodeStr(BO_LE);
                break;
            default:
                break;
        }
        res.append(bo1r).append(opstr).append(bo1l);
        return res;
    }

    // b + a --> a + b
    if(BO_com_map[bo1op] && bo1l.compare(bo1r) > 0){
        res.append(bo1r).append(BinaryOperator::getOpcodeStr(bo1op)).append(bo1l);
        return res;
    }

    // op1 op op2 --> op1 op op2
    res.append(bo1l).append(BinaryOperator::getOpcodeStr(bo1op)).append(bo1r);

    return res;
}

/// Regular expr to text
/// Fine-grained expr (without || and &&)
/// Input: whether a UB_Not exists
/// Output: text of expression
string LogBehavior::getExprText(bool hasNot){

    string res = ""; //for overall result

    string uo = hasNot?"!":""; //for UO not
    string bo1l = ""; //for left child of bo1
    string bo1r = ""; //for right child of bo1
    BinaryOperator::Opcode bo1op; //for op of bo1

    string bo2l = ""; //for left child of bo2
    string bo2r = ""; //for right child of bo2
    BinaryOperator::Opcode bo2op; //for op of bo2
    string bo2res = ""; //for all text of bo2


    // No binaryoperator (bo2 is child of bo1, either left child or right child)
    if(bo1 == nullptr){
        res.append(uo).append(getLeafText(expr));
        if(res.find("!!") == 0)
            res = res.substr(2);
        return res;
    }

    // There exists binaryoperator:
    // Deal with NOT
    bo1op = bo1->getOpcode();
    if(hasNot){
        hasNot = false;
        uo = "";
        switch(bo1op){
            case BO_LT: // < --> >=
                bo1op = BO_GE;
                break;
            case BO_GT: // > --> <=
                bo1op = BO_LE;
                break;
            case BO_LE: // <= --> >
                bo1op = BO_GT;
                break;
            case BO_GE: // >= --> <
                bo1op = BO_LT;
                break;
            case BO_EQ: // == --> !=
                bo1op = BO_NE;
                break;
            case BO_NE: // != --> ==
                bo1op = BO_EQ;
                break;
            default:
                hasNot = true;
                uo = "!";
                break;
        }
    }
    res.append(uo);

    // Only one binaryoperator:
    // Get text of bo1 and return
    if(bo2 == nullptr){
        bo1l.append(getLeafText(bo1->getLHS()));
        bo1r.append(getLeafText(bo1->getRHS()));
        res.append(getExprRegular(bo1l, bo1r, bo1op));
        return res;
    }

    // Two binaryoperators:
    // Get text of bo2 for further analyzing
    bo2op = bo2->getOpcode();
    bo2l.append(getLeafText(bo2->getLHS()));
    bo2r.append(getLeafText(bo2->getRHS()));
    bo2res.append(getExprRegular(bo2l, bo2r, bo2op));

    // Get text of bo1 and return
    // bo2 is in left child of bo1
    if(bo1->getLHS()->IgnoreCasts()->IgnoreParens() == bo2){
        bo1l.append(bo2res);
        bo1r.append(getLeafText(bo1->getRHS()));
    }
    // bo2 is in left child of bo1
    else if(bo1->getRHS()->IgnoreCasts()->IgnoreParens() == bo2){
        bo1l.append(getLeafText(bo1->getLHS()));
        bo1r.append(bo2res);
    }
    // I don't what happens
    else{
        //bo1->dump();
        return "Unknown2";
    }

    res.append(getExprRegular(bo1l, bo1r, bo1op));
    return res;
}

/// Free nodes
/// Input: root node
/// Output: null
void LogBehavior::nodeFree(Node* r){

    if(r == nullptr)
        return;

    nodeFree(r->childl);
    nodeFree(r->childr);

    delete r;

    return;
}

/// Get text of nodes
/// Input: root node
/// Output: overall text
string LogBehavior::getNodeText(Node* r){

    if(r == nullptr)
        return "";

    string textl = "";
    string textop = "";
    string textr = "";

    if(r->changeOp){
        if(r->BO->getOpcode() == BO_LOr)
            textop = "&&";
        if(r->BO->getOpcode() == BO_LAnd)
            textop = "||";
    }
    else
        textop = r->BO->getOpcodeStr();

    if(r->childl == nullptr){
        textl = r->textl;
    }
    else{
        textl = getNodeText(r->childl);
    }

    if(r->childr == nullptr){
        textr = r->textr;
    }
    else{
        textr = getNodeText(r->childr);
    }

    string res = "";
    if(textl.substr(1).compare(textr.substr(1)) > 0){
        return res.append("(").append(textr).append(" ").append(textop).append(" ").append(textl).append(")");
    }
    else{
        return res.append("(").append(textl).append(" ").append(textop).append(" ").append(textr).append(")");
    }
}

/// Regular nodes from AST to text
/// Input: root node (root is || or &&)
/// Output: r->textl and r->textr
void LogBehavior::nodeRegular(Node* r){

    if(r == nullptr)
        return;

    // !(a||b) to !a&&!b
    bool lnot = 0, rnot = 0;
    if(r->LNot){
        if(r->childl)
            r->childl->LNot = 1 - r->childl->LNot;
        else
            lnot = true;
        if(r->childr)
            r->childr->LNot = 1 - r->childr->LNot;
        else
            rnot = true;
        r->LNot = 0;
        r->changeOp = 1;
    }

    bo1 = nullptr;
    bo2 = nullptr;
    if(r->childl == nullptr){
        if(r->BO != nullptr){
            expr = r->BO->getLHS();
            if(getAtMost2BO(dyn_cast<Stmt>(expr)) == false){
                r->textl = "Unknown3";
            }
            else{
                r->textl = getExprText(lnot);
            }
        }
    }

    bo1 = nullptr;
    bo2 = nullptr;
    if(r->childr == nullptr){
        if(r->BO != nullptr){
            expr = r->BO->getRHS();
            if(getAtMost2BO(dyn_cast<Stmt>(expr)) == false){
                r->textr = "Unknown3";
            }
            else{
                r->textr = getExprText(rnot);
            }
        }
    }

    nodeRegular(r->childl);
    nodeRegular(r->childr);

    return;
}

/// Collect || and && in condition of ifstmt
/// Coarse-grained node (only || and && are non-leaf nodes)
/// Input: condition of ifstatement
/// Output: BOList (list of || and &&)
void LogBehavior::nodeCollect(Stmt* stmt){

    // Ignore call, stmtexpr adn conditional operator
    if(dyn_cast<CallExpr>(stmt) || dyn_cast<StmtExpr>(stmt) || dyn_cast<ConditionalOperator>(stmt)){
        return;
    }

    if(BinaryOperator *binaryOperator = dyn_cast<BinaryOperator>(stmt)){
        if(binaryOperator->getOpcode() == BO_LOr || binaryOperator->getOpcode() == BO_LAnd){
            BOList.push_back(binaryOperator);
        }
    }

    // Travel if condition, recursively.
    for (Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
        if(Stmt *child = *it)
            nodeCollect(child);
    }
    return;
}

/// Regular logging behaviors
/// Input: CompilerInstance from Visitor
/// Output: All regulared behaviors
void LogBehavior::regular(CompilerInstance* ci){

    if(stmt == nullptr)
        return;

    IfStmt* ifStmt = dyn_cast<IfStmt>(stmt);
    if(ifStmt == nullptr)
        return;

    // Get compiler instance
    CI = ci;

    // Ignore opaque expression
    if(dyn_cast<OpaqueValueExpr>(ifStmt->getCond())){
        return;
    }

    // Search for || and &&, and store to BOList
    nodeCollect(ifStmt->getCond());

    // Create new Node for each BO
    map<BinaryOperator*, Node*>BOMap;
    for(unsigned i = 0; i < BOList.size(); i++){
        Node* node = new Node();
        node->BO = BOList[i];
        BOMap[BOList[i]] = node;
    }

    // Calcaulate node content
    ParentMap PM(ifStmt->getCond());
    for(unsigned i = 0; i < BOList.size(); i++){
        Node* node = BOMap[BOList[i]];

        // Find parent node
        Stmt* ite = dyn_cast<Stmt>(node->BO);
        Stmt* p = PM.getParent(ite);
        while(p){

            // Record unaryoperator
            if(UnaryOperator* uo = dyn_cast<UnaryOperator>(p)){
                if(uo->getOpcode() == UO_LNot)
                    node->LNot = 1- node->LNot;
            }

            // Deal with binaryoperator
            if(BinaryOperator* bo = dyn_cast<BinaryOperator>(p)){
                Node* pnode = BOMap[bo];
                if(pnode != nullptr){
                    node->parent = pnode;
                    if(bo->getLHS() == ite)
                        pnode->childl = node;
                    else if(bo->getRHS() == ite)
                        pnode->childr = node;
                    break;
                }
            }

            // Reach to top
            if(p == ifStmt->getCond())
                break;

            // Search further
            ite = p;
            p = PM.getParent(ite);
        }
    }

//    for(unsigned i = 0; i < BOList.size(); i++){
//        Node* node = BOMap[BOList[i]];
//        errs()<<node<<" "<<node->parent<<" "<<node->childl<<" "<<node->childr<<" "<<node->BO<<"\n";
//    }

    string text;
    if(BOList.size()!=0){
        //errs()<<callText<<"@"<<callLoc<<"#"<<argIndex<<"("<<argType<<")\n---"<<ifText<<"\n";
        root = BOMap[BOList[0]];
        // Log statement is located in else branch
        if(ifBranch){
            root->LNot = 1 - root->LNot;
        }
        nodeRegular(root);
        text = getNodeText(root);
        nodeFree(root);
    }
    else{
        bo1 = nullptr;
        bo2 = nullptr;
        expr = ifStmt->getCond();
        if(getAtMost2BO(dyn_cast<Stmt>(expr)) == false){
            text = "Unknown3";
        }
        else{
            text = getExprText(ifBranch);
        }
        //errs()<<text<<"\n";
    }

    regularText = text;
}

/// Print the detailed infomation of a logging behavior
void LogBehavior::print(){

    if(regularText.find("Unknown") == string::npos)
        outs()<<callName<<"#"<<argIndex<<"#"<<argType<<"#"<<regularText<<"#"<<logType<<"\n";

    errs()<<callName<<"#"<<argIndex<<"#"<<argType<<"#"<<regularText<<"#"<<logType<<"\n";
    errs()<<"---Call Text: "<<callText<<"@"<<callLoc<<"\n";
    errs()<<"---Check Text: "<<"("<<(ifBranch?"-":"+")<<")"<<ifText<<"@"<<ifLoc<<"\n";
    errs()<<"---Log Text: "<<logText<<"@"<<logLoc<<"\n";

}

string getStrArg(Stmt * stmt){


    if(StringLiteral* stringLiteral = dyn_cast<StringLiteral>(stmt)){
        return stringLiteral->getBytes();
    }

    string ret = "";
    for (Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
        if(Stmt *child = *it){
            ret.append(getStrArg(child));
        }
    }
    return ret;
}

/// Extract logging behavior
/// Input: Declaration of caller function
/// Output: none
void FindLoggingVisitor::extractLoggingSnippet(FunctionDecl* Declaration){

    if(Declaration == nullptr)
        return;

    // Get size of 3 lists
    unsigned callNum, ifNum, switchNum;
    callNum = callList.size();
    ifNum = ifList.size();
    switchNum = switchList.size();

    // Get file name
    string fileName = getStemName(InFile);

    // Get caller name
    string callerName = Declaration->getNameAsString();

    if(fileName.length() == 0 || callerName.length() == 0)
        return;

    // For each call expression
    for(unsigned i = 0; i < callNum; i++){

        FunctionDecl* functionDeal = callList[i]->getDirectCallee();

        // Ignore function pointer
        if(functionDeal == nullptr){
            continue;
        }

        // Ignore the case that the call statement is a log
        if(logListMap[callList[i]] == true){
            continue;
        }

        // Get callee name
        string calleeName = functionDeal->getNameAsString();
        if(calleeName.length() == 0)
            continue;

        // Get callee line number
        unsigned srcline = 0;
        char str[10];
        FullSourceLoc callstart = CI->getASTContext().getFullLoc(callList[i]->getLocStart());
        if(callstart.isValid())
            srcline = callstart.getExpansionLineNumber();
        else
            continue;
        sprintf(str, "%d", srcline);
        string calleeLine = str;
        // For each if statement
        for(unsigned j = 0; j < ifNum; j++){

            // Get ifstmt line number
            unsigned desline = 0;
            FullSourceLoc ifstart = CI->getASTContext().getFullLoc(ifList[j]->getLocStart());
            if(ifstart.isValid())
                desline = ifstart.getExpansionLineNumber();
            else
                continue;
            sprintf(str, "%d", desline);
            string depLine = str;

            // Ignore the case when ifstmt contains callexpr
            if(srcline != desline && hasCallExpr(ifList[j]->getCond())){
                continue;
            }

            // Combine string key
            string key = "";
            key.append(fileName).append(",").append(callerName).append(",").append(calleeName);
            key.append(",").append(calleeLine).append(",").append(depLine);

            //errs()<<"+++++"<<callerName<<"\n";
            //errs()<<"+++"<<calleeName<<"@"<<calleeLine<<"\n";

            // Check the call is dependent on ifstmt or not
            if(myCallDepMap[key] || calleeLine == depLine){

                // Debug
                if(0)
                if(callerName == "ap_resolve_env" && calleeName == "strchr"){
                    callList[i]->dump();
                    errs()<<calleeLine<<"\n";
                }

                // Indicate no log (0), local log (1 ifbranch/2 elsebranch) or remote log (3 ifbranch/4 elsebranch)
                int flag = 0;

                myLogStmt = nullptr;
                myReturnStmt = nullptr;
                myOutputStmt = nullptr;

                if(hasLogStmt(ifList[j]->getThen())){
                    flag = 2;
                }
                else if(hasLogStmt(ifList[j]->getElse())){
                    flag = 1;
                }
                else if(hasOutputStmt(ifList[j]->getThen())){
                    flag = 6;
                }
                else if(hasOutputStmt(ifList[j]->getElse())){
                    flag = 5;
                }
                else if(hasReturnStmt(ifList[j]->getThen())){
                    flag = 4;
                }
                else if(hasReturnStmt(ifList[j]->getElse())){
                    flag = 3;
                }


                // Unknown error
                if(flag && myLogStmt == nullptr && myReturnStmt == nullptr && myOutputStmt == nullptr){
                    continue;
                }

                // Get the code text of logstmt
                string logText = "";
                FullSourceLoc logstart, logend;
                if(flag == 1 || flag == 2){
                    logstart = CI->getASTContext().getFullLoc(myLogStmt->getLocStart());
                    logend = CI->getASTContext().getFullLoc(myLogStmt->getLocEnd());
                }
                else if(flag == 3 || flag == 4){
                    logstart = CI->getASTContext().getFullLoc(myReturnStmt->getLocStart());
                    logend = CI->getASTContext().getFullLoc(myReturnStmt->getLocEnd());
                }
                else if(flag == 5 || flag == 6){
                    logstart = CI->getASTContext().getFullLoc(myOutputStmt->getLocStart());
                    logend = CI->getASTContext().getFullLoc(myOutputStmt->getLocEnd());
                }
                if(logstart.isValid() && logend.isValid()){
                    SourceRange sr(logstart.getExpansionLoc(), logend.getExpansionLoc());
                    logText =Lexer::getSourceText(CharSourceRange::getTokenRange(sr), CI->getSourceManager(), LangOptions(), 0);

                    /*string logText2 = "";
                    if(flag == 1 || flag == 2){
                        SourceLocation sl = myLogStmt->getSourceRange().getBegin();
                        if (sl.isMacroID())
                            logText2 = Lexer::getImmediateMacroName(sl, CI->getSourceManager(), LangOptions());
                    }
                    SourceLocation sl = Lexer::findLocationAfterToken(logstart.getExpansionLoc(), tok::semi, CI->getSourceManager(),LangOptions(), 0);
                    SourceRange sr2(logstart.getExpansionLoc(), sl);
                    string logText2 =Lexer::getSourceText(CharSourceRange::getTokenRange(sr), CI->getSourceManager(), LangOptions(), 0);
                     errs()<<fsl.printToString(CI->getSourceManager())<<"\n";*/

                    if(logText.find('(') == string::npos){
                        if(flag == 1 || flag == 2){
                            logText.append(getStrArg(myLogStmt));
                        }
                        else if(flag == 5 || flag == 6){
                            logText.append(getStrArg(myOutputStmt));
                        }
                    }
                    //errs()<<logText<<"@";
                    //errs()<<logstart.printToString(CI->getSourceManager())<<"#"<<logend.printToString(CI->getSourceManager())<<"\n";
                }

                // In SA case, the call should be after log
                if(logstart.isValid() && logstart.getExpansionLineNumber() > srcline && srcline > desline){
                    continue;
                }

                // Get the code text of ifstmt
                string ifText = "";
                FullSourceLoc ifconstart = CI->getASTContext().getFullLoc(ifList[j]->getCond()->getLocStart());
                FullSourceLoc ifconend = CI->getASTContext().getFullLoc(ifList[j]->getCond()->getLocEnd());
                if(ifconstart.isValid() && ifconend.isValid()){
                    SourceRange sr(ifconstart.getExpansionLoc(), ifconend.getExpansionLoc());
                    ifText =Lexer::getSourceText(CharSourceRange::getTokenRange(sr), CI->getSourceManager(), LangOptions(), 0);
                }

                // Get the code text of callexpr
                string callText = "";
                FullSourceLoc callstart = CI->getASTContext().getFullLoc(callList[i]->getLocStart());
                FullSourceLoc callend = CI->getASTContext().getFullLoc(callList[i]->getLocEnd());
                if(callstart.isValid() && callend.isValid()){
                    SourceRange sr(callstart.getExpansionLoc(), callend.getExpansionLoc());
                    callText =Lexer::getSourceText(CharSourceRange::getTokenRange(sr), CI->getSourceManager(), LangOptions(), 0);
                }

                // Ignore the case when check and log at the same line, it in general is FP
                if(ifconstart.isValid() && logstart.isValid()){
                    unsigned checkline = ifconstart.getExpansionLineNumber();
                    unsigned logline = logstart.getExpansionLineNumber();
                    if(checkline == logline)
                        continue;
                }

                // Build log behavior
                LogBehavior myLog;

                myLog.callName = calleeName;
                myLog.callText = callText;
                myLog.retName = mapFuncRet[callList[i]];
                myLog.callLoc = callstart.printToString(CI->getSourceManager());
                myLog.callLoc = myLog.callLoc.substr(0, myLog.callLoc.find(" "));
                myLog.callExpr = callList[i];

                myLog.argIndex = myCallDepMap[key] - 1;
                myLog.argType = "NONE";
                if(srcline > desline)
                    myLog.argType = "SA";
                if(srcline < desline && myCallDepMap[key] != 1)
                    myLog.argType = "PA";
                if(srcline < desline && myCallDepMap[key] == 1){
                    myLog.argIndex = 0;
                    myLog.argType = "RV";
                }
                if(srcline == desline){
                    myLog.argIndex = 0;
                    myLog.argType = "RV";
                }

                myLog.ifText = ifText;
                myLog.ifLoc = ifconstart.printToString(CI->getSourceManager());
                myLog.ifLoc = myLog.ifLoc.substr(0, myLog.ifLoc.find(" "));
                myLog.ifBranch = flag % 2;
                myLog.stmt = ifList[j];

                myLog.logText = logText;
                myLog.logType = "NONE";
                if(flag == 1 || flag == 2){
                    myLog.logType = "LL";
                    myLog.logLoc = logstart.printToString(CI->getSourceManager());
                    myLog.logLoc = myLog.logLoc.substr(0, myLog.logLoc.find(" "));
                }
                else if(flag == 3 || flag == 4){
                    myLog.logType = "RL";
                    myLog.logLoc = logstart.printToString(CI->getSourceManager());
                    myLog.logLoc = myLog.logLoc.substr(0, myLog.logLoc.find(" "));
                }
                else if(flag == 5 || flag == 6){
                    myLog.logType = "OS";
                    myLog.logLoc = logstart.printToString(CI->getSourceManager());
                    myLog.logLoc = myLog.logLoc.substr(0, myLog.logLoc.find(" "));
                }
                else
                    myLog.logType = "NL";


                //myLog.print();
                myLog.regular(CI);

                // Write logging behavior
                string behavior = "";
                char arg[10];
                sprintf(arg, "%d", myLog.argIndex);
                string branch;
                if(myLog.ifBranch)
                    branch = "-";
                else
                    branch = "+";

                /*
                // Function Call
                behavior.append(myLog.callText).append("@").append(myLog.callLoc).append("#");
                // Reason
                behavior.append(arg).append("(").append(myLog.argType).append(")#");
                // Check
                behavior.append(myLog.ifText).append("(").append(branch).append(")@").append(myLog.ifLoc).append("#");
                // Log
                behavior.append(myLog.logText).append("(").append(myLog.logType).append(")@").append(myLog.logLoc).append("\n");
                */
                if(myLog.retName.length() != 0)
                    myLog.retName.append("=");

                behavior.append(myLog.callName).append("`").append(myLog.retName).append(myLog.callText).append("`").append(myLog.callLoc).append("`");
                behavior.append(arg).append("`").append(myLog.argType).append("`");
                behavior.append(myLog.ifText).append("(").append(branch).append(")").append("`").append(myLog.regularText).append("`").append(myLog.ifLoc).append("`");
                behavior.append(myLog.logText).append("`").append(myLog.logType).append("`").append(myLog.logLoc);

                string::size_type pos = 0;
                while ( (pos = behavior.find('\n', pos)) != string::npos ) {
                    behavior.replace( pos, 1, "");
                    pos++;
                }
                pos = 0;
                while ( (pos = behavior.find(' ', pos)) != string::npos ) {
                    behavior.replace( pos, 1, "");
                    pos++;
                }
                pos = 0;
                while ( (pos = behavior.find('\t', pos)) != string::npos ) {
                    behavior.replace( pos, 1, "");
                    pos++;
                }

                behavior.append("\n");

                fputs(behavior.c_str(), fLogBehavior);

                myLogBehavior.push_back(myLog);

            } //if(myCallDepMap[key]){
        }

        // For each switch statement
        // TODO
    }
}

//===----------------------------------------------------------------------===//
//  Extract logging statement
//===----------------------------------------------------------------------===//

/// Print the detailed infomation of a function
void FunctionFeat::print(){

    outs()<<funcName<<","<<fileName<<","<<funcKeyWord<<","<<lenth<<","<<errorNumber<<","<<normalNumber;

//    set<int>::iterator it;
//    for(it=callerID.begin();it!=callerID.end();it++){
//        outs()<<","<<*it;
//        outs()<<"("<<flowID[*it]<<")";
//    }

    outs()<<"\n";
}

/// Get the source code of stmt
/// Input: expr/stmt
/// Output: code text
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

/// Check whether a Type is char or not
/// Input: QualType
/// Output: yes or no
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

/// Check whether a given string EQUALs to an error/print/exit keyword.
/// Input: a string
/// Output: yes or no
bool FindLoggingVisitor::equalKeyword(string name){

    string word[100] = {

        // Error keyword
        "error","err","warn","alert","assert","fail","critical","crit","emergency","emerg","out",
        "wrong","except","bug","faulty","fatal","warning","fault","misplay","damage","incorrect",
        "imporper","defect","flaw","invalid","disable","demerit","vital","misuse","not","busy",
        "unable","illegal","exception","errmsg","only","too","no","miss","mis","abort","overflow",
        "giving up","give up","timeout","unexpected","n't","bad","corrupt","unknown","un","exhausted",

        // Exit keyword
        "exit","die","halt","suspend","stop","quit","close",

        // Print keyword
        "output","out","put","printf","print","fprintf","write","log","message","msg","record",
        "report","dump","dialog","hint","trace","puts","fputs","printer","notify"
    };

    for(int i = 0; i < 100; i++){
        if(word[i].length() == 0)
            break;
        if(!name.compare(word[i]))
            return true;
    }

    return false;
}


/// Check whether a given string CONTAINs an output keyword.
/// Input: a string
/// Output: yes or no
bool FindLoggingVisitor::containKeyword(string name){

    string word[100] = {

        "err","warn","alert","assert","fail","critical","crit","emergency","emerg",
        "output","out","put","print","write","log","message","msg","record",
        "report","dump","puts"
    };

    for(int i = 0; i < 100; i++){
        if(word[i].length() == 0)
            break;
        if(name.find(word[i]) < name.length())
            return true;
    }

    return false;
}

/// Check whether a given string contains an error/print/exit keyword after been split.
/// Input: a string (with sevral words in one string)
/// Output: yes or no
bool FindLoggingVisitor::spiltWord(string name){

    if(name.length() == 0)
        return false;

    char format[10] = "s";
    pResult = PyObject_CallFunction(pFunc, format, name.c_str());

    int size = PyList_GET_SIZE(pResult);
    for(int i = 0; i < size; i++){

        pTmp = PyList_GetItem(pResult, i);

        string str;
        str = PyString_AsString(pTmp);
        if(equalKeyword(str)){
            return true;
        }
    }

    return false;
}

/// Check whether a raw string contains an error/print/exit keyword after been split.
/// Input: a string (with sevral words in one string, with '_' or ':')
/// Output: yes or no
bool FindLoggingVisitor::hasKeyword(string name){

    // Transfor all letters into lower
    transform(name.begin(), name.end(), name.begin(), ::tolower);  //::toupper

    // In case of Cls::foo, we just ignore the class name or namespace
    if(name.find(':') != string::npos)
        name = name.substr(name.find_last_of(':') + 1);

    // In case of my_error, splitWord method cannot handle '-', hence we do it before
    while(name.find('_') != string::npos){
        string subname = name.substr(0, name.find_first_of('_'));
        name = name.substr(name.find_first_of('_') + 1);


        if(spiltWord(subname))
            return true;
    }
    if(spiltWord(name))
        return true;

    return false;
}

/// Check whether a given string EQUALs to an error keyword.
/// Input: a string
/// Output: yes or no
bool FindLoggingVisitor::hasErrorKeyword(string name){

    transform(name.begin(), name.end(), name.begin(), ::tolower);  //::toupper

    string word[100] = {
        "error","err","warn","alert","assert","fail","critical","crit","emergency","emerg","out",
        "wrong","except","bug","faulty","fatal","warning","fault","misplay","damage","incorrect",
        "imporper","defect","flaw","invalid","disable","demerit","vital","misuse","not","busy",
        "unable","illegal","exception","errmsg","only","too","no","miss","mis","abort","overflow",
        "giving up","give up","timeout","unexpected","n't","bad","corrupt","unknown","un","exhausted"

    };

    for(int i = 0; i < 100; i++){
        if(word[i].length() == 0)
            break;
        if(name.find(word[i]) < name.length()){
            return true;
        }
    }

    return false;
}

/// Check whether a given string CONTAINs an successful keyword with a negative prefix.
/// Input: a string
/// Output: yes or no
bool FindLoggingVisitor::hasNonCorrectKeyword(string name){

    transform(name.begin(), name.end(), name.begin(), ::tolower);  //::toupper

    string neg[40] = {
        "n’t","not","im","in","un","yet","non","nor","neither"
    };

    bool hasNeg = false;
    for(int i = 0; i < 40; i++){
        if(neg[i].length() == 0)
            break;
        if(name.find(neg[i]) < name.length()){
            hasNeg = true;
            break;
        }
    }
    if(!hasNeg)
        return false;

    string word[40] = {
        "complete","success","finish","accomplished","perfect","destination","goal","fulfill","available"
    };

    for(int i = 0; i < 40; i++){
        if(word[i].length() == 0)
            break;
        if(name.find(word[i]) < name.length())
            return true;
    }

    return false;
}

/// Check whether a given string CONTAINs an exit keyword.
/// Input: a string
/// Output: yes or no
bool FindLoggingVisitor::hasExitKeyword(string name){

    transform(name.begin(), name.end(), name.begin(), ::tolower);  //::toupper

    string word[40] = {
        "exit","die","halt","suspend","stop","quit","kick"
    };

    for(int i = 0; i < 40; i++){
        if(word[i].length() == 0)
            break;
        if(name.find(word[i]) < name.length())
            return true;
    }

    return false;
}

/// Check whether a given string CONTAINs a limitation keyword.
/// Input: a string
/// Output: yes or no
bool FindLoggingVisitor::hasLimitationKeyword(string name){

    transform(name.begin(), name.end(), name.begin(), ::tolower);  //::toupper

    string word[40] = {
        "max","limit","most","uper","boundary","must",
        "bound","demand","last","require","confine","min","lower"
    };

    for(int i = 0; i < 40; i++){
        if(word[i].length() == 0)
            break;
        if(name.find(word[i]) < name.length())
            return true;
    }

    return false;
}

/// Check whether a given string CONTAINs an print keyword.
/// Input: a string
/// Output: yes or no
bool FindLoggingVisitor::hasPrintKeyword(string name){

	transform(name.begin(), name.end(), name.begin(), ::tolower);  //::toupper

	string word[40] = {
        "output","out","put","printf","fprintf","print","write","log","message","msg","record",
        "report","dump","dialog","hint","trace","puts","fputs","printer","notify"
    };

	for(int i = 0; i < 40; i++){
		if(word[i].length() == 0)
			break;
		if(name.find(word[i]) < name.length())
			return true;
	}

	return false;
}

/// Get the stem name of a given file name
/// Input: a string indicating a file name
/// Output: the stem name of the file
StringRef FindLoggingVisitor::getStemName(StringRef name){
    //errs()<<name<<" ";
    StringRef stem_name = name;
    if(stem_name.find('/') != string::npos){
        stem_name = stem_name.substr(stem_name.find_last_of('/') + 1);
    }
    stem_name = stem_name.substr(0, stem_name.find_first_of('.'));
    //errs()<<stem_name<<"\n";
    return stem_name;
}

/// Check whether a give statement CONTAINs an exit/error/limitation/non-correct keyword
/// Input: statement
/// Outout: yes or no
bool FindLoggingVisitor::searchKeyword(Stmt* stmt){

    if(stmt == nullptr)
        return false;

    string text;

//    if(DeclRefExpr* declRefExpr = dyn_cast<DeclRefExpr>(stmt)){
//        if(declRefExpr->getDecl())
//            text = declRefExpr->getDecl()->getDeclName().getAsString();
//    }

    if(StringLiteral* stringLiteral = dyn_cast<StringLiteral>(stmt)){
        text = stringLiteral->getBytes();
    }

//    if(IntegerLiteral* integerLiteral = dyn_cast<IntegerLiteral>(stmt)){
//        text = expr2str(integerLiteral);
//    }

    if(hasExitKeyword(text) || hasErrorKeyword(text) ||
       hasLimitationKeyword(text) || hasNonCorrectKeyword(text)){
        return true;
    }
    else{
        //if(text != "")
            //errs()<<text<<"\n";
    }


    for (Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
        if(Stmt *child = *it)
            if(searchKeyword(child))
                return true;
    }

    return false;
}

/// Write logging statement to result file
/// Input: callexpr
/// Output: true when confirming a logging statement, otherwise, false
bool FindLoggingVisitor::isLoggingStatement(CallExpr* callExpr, Stmt* parent){

    if(!callExpr)
        return false;

    if(FunctionDecl* functionDeal = callExpr->getDirectCallee()){

        string name = functionDeal->getNameAsString();

        // Get the lenth of the function
        unsigned int start = 0, end = 0;
        FullSourceLoc functionstart = CI->getASTContext().getFullLoc(functionDeal->getLocStart());
        FullSourceLoc functionend = CI->getASTContext().getFullLoc(functionDeal->getLocEnd());
        if(functionstart.isValid() && functionend.isValid()){
            functionstart = functionstart.getExpansionLoc();
            start = functionstart.getExpansionLineNumber();
            functionend = functionend.getExpansionLoc();
            end = functionend.getExpansionLineNumber();
        }

        if(end-start<100 && hasKeyword(name)){

            FullSourceLoc fullStartCallText(callExpr->getLocStart(), CI->getSourceManager());
            fullStartCallText = fullStartCallText.getExpansionLoc();

            unsigned arg_num = callExpr->getNumArgs();
            for(unsigned i = 0; i < arg_num; i++){
                if(searchKeyword(callExpr->getArg(i))){

                    string callName = name;
                    string callLoc = fullStartCallText.printToString(CI->getSourceManager());
                    fputs(callName.c_str(), fLogStmt);
                    fputs("@", fLogStmt);
                    fputs(callLoc.c_str(), fLogStmt);
                    fputs("\n", fLogStmt);
                    return true;
                }

            }
        }
    }
    return false;
}

/// Check whether an ifstmt contains a log statement
/// Input: if statemet
/// Output: yes or no
bool FindLoggingVisitor::hasLogStmt(Stmt* stmt){

    if(!stmt)
        return false;

    // Skip for/while statement
    if(dyn_cast<ForStmt>(stmt) || dyn_cast<WhileStmt>(stmt)){
        return false;
    }

    // Check log statement
    if(CallExpr* callExpr = dyn_cast<CallExpr>(stmt)){
        // Confirm a log
        if(logListMap[callExpr] == true){
            myLogStmt = callExpr;
            return true;
        }
    }

    // Travel the statement, recursively.
    for (Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
        if(Stmt *child = *it)
            if(hasLogStmt(child))
                return true;
    }
    return false;
}

/// Check whether an ifstmt contains a output statement
/// Input: if statemet
/// Output: yes or no
bool FindLoggingVisitor::hasOutputStmt(Stmt* stmt){

    if(!stmt)
        return false;

    // Skip for/while statement
    if(dyn_cast<ForStmt>(stmt) || dyn_cast<WhileStmt>(stmt)){
        return false;
    }

    // Check log statement
    if(CallExpr* callExpr = dyn_cast<CallExpr>(stmt)){
        // Confirm a output
        if(callExpr->getDirectCallee()){
            if(containKeyword(callExpr->getDirectCallee()->getNameAsString())){
                myOutputStmt = callExpr;
                return true;
            }
        }
    }

    // Travel the statement, recursively.
    for (Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
        if(Stmt *child = *it)
            if(hasOutputStmt(child))
                return true;
    }
    return false;
}

/// Check whether an ifstmt contains a return statement
/// Input: if statemet
/// Output: yes or no
bool FindLoggingVisitor::hasReturnStmt(Stmt* stmt){

    if(!stmt)
        return false;

    // Skip for/while statement
    if(dyn_cast<ForStmt>(stmt) || dyn_cast<WhileStmt>(stmt)){
        return false;
    }

    // Record return statement
    if(ReturnStmt* returnStmt = dyn_cast<ReturnStmt>(stmt)){
        myReturnStmt = returnStmt;
        return true;
    }

    // Travel the statement, recursively.
    for (Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
        if(Stmt *child = *it)
            if(hasReturnStmt(child))
               return true;
    }
    return false;
}

/// Check whether an ifcond contains a call statement
/// Input: if statemet condition
/// Output: yes or no
bool FindLoggingVisitor::hasCallExpr(Stmt* stmt){

    if(!stmt)
        return false;

    if(auto* callExpr = dyn_cast<CallExpr>(stmt)){

        if(auto decl = callExpr->getDirectCallee()){

            unsigned int start, end;
            FullSourceLoc functionstart = CI->getASTContext().getFullLoc(decl->getLocStart()).getExpansionLoc();
            FullSourceLoc functionend = CI->getASTContext().getFullLoc(decl->getLocEnd()).getExpansionLoc();
            start = functionstart.getExpansionLineNumber();
            end = functionend.getExpansionLineNumber();

            // Ignore small/inline function
            if(end - start >= 10 && !decl->isInlined() && decl->getNumParams() > 1){
                return true;
            }
        }
        else
            return true;
    }

    // Travel the statement, recursively.
    for (Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
        if(Stmt *child = *it)
            if(hasCallExpr(child))
                return true;
    }
    return false;
}

/// Get the brother node of a AST node
/// Input: root node, and stmt node
/// Output: brother node of stmt
Stmt* FindLoggingVisitor::getNextNode(Stmt* root, Stmt* stmt){

    ParentMap PM(root);
    Stmt* parent = PM.getParent(stmt);

    bool foundStmt = false;

    // Search the children
    for (Stmt::child_iterator it = parent->child_begin(); it != parent->child_end(); ++it){
        if(Stmt *child = *it){
            if(foundStmt == true)
                return child;
            if(child == stmt)
                foundStmt = true;
        }
    }

    return 0;
}

/// Travel the AST, and find all callexpr
/// Input: statement
/// Output: none
void FindLoggingVisitor::travelStmt(Stmt* stmt, Stmt* parent){

    // Check whether in IfCond or not
    bool isChange = false;
    if(IfStmt* ifStmt = dyn_cast<IfStmt>(parent)){
        if(ifStmt->getCond() == stmt){
            if(!isInIfCond){
                isChange = true;
                isInIfCond = true;
            }
        }
    }

    // For each ifstmt
    if(IfStmt* ifStmt = dyn_cast<IfStmt>(stmt)){
        // Record if statement
        ifList.push_back(ifStmt);
    }

    // For each switchstmt
    if(SwitchStmt* switchStmt = dyn_cast<SwitchStmt>(stmt)){
        // Record switch statement
        switchList.push_back(switchStmt);
    }

    // For each callexpr
	if(CallExpr* callExpr = dyn_cast<CallExpr>(stmt)){
        // Record call list
        callList.push_back(callExpr);

        // Record return name: ret = foo();
        if(BinaryOperator* bo = dyn_cast<BinaryOperator>(parent)){
            if(bo->getOpcode() == BO_Assign){
                string retname = expr2str(bo->getLHS());
                mapFuncRet[callExpr] = retname;
            }
        }
        // Record return name: int ret = foo();
        if(DeclStmt* ds = dyn_cast<DeclStmt>(parent)){
            VarDecl* vd;
            if(ds->isSingleDecl() && (vd = dyn_cast<VarDecl>(ds->getSingleDecl()))){
                string retname = vd->getNameAsString();
                mapFuncRet[callExpr] = retname;
            }
        }

		if(FunctionDecl* functionDecl = callExpr->getDirectCallee()){

            // Write logging statement
            if(isLoggingStatement(callExpr, parent)){
                // Record log list
                logListMap[callExpr] = true;
            }

            // Get the callee function name and its index
            string name = functionDecl->getNameAsString();
            if(myFunctionFeatMap[name] == 0){
                myFunctionFeatMap[name] = ++myFunctionFeatCnt;
            }
            int index = myFunctionFeatMap[name];
            if(index == MAX_FUNC_NUM){
                errs()<<"Too many functions!\n";
                exit(1);
            }

            // Count all calls
            funcMap[name]++;

            // Record all call statements
            SourceLocation callLocation = callExpr->getLocStart();
            if(callLocation.isValid()){
                fputs(name.c_str(), fCallStmt);
                fputs(" ", fCallStmt);
                string loc = callLocation.printToString(CI->getSourceManager());
                loc = loc.substr(0, loc.find(" "));
                fputs(loc.c_str(), fCallStmt);
                fputs(" ", fCallStmt);
                if(isInIfCond || dyn_cast<BinaryOperator>(parent) || dyn_cast<UnaryOperator>(parent) ||
                   dyn_cast<DeclStmt>(parent) || dyn_cast<ReturnStmt>(parent))
                    fputs("1", fCallStmt);
                else
                    fputs("0", fCallStmt);
                fputs("\n", fCallStmt);
            }

            // Set the function name, file name and function keyword
            myFunctionFeat[index].funcName = name;
            myFunctionFeat[index].funcKeyWord = hasKeyword(name);
            if(myFunctionFeat[index].fileName.length() == 0){
                string fileName = CI->getASTContext().getFullLoc(functionDecl->getLocStart()).printToString(CI->getSourceManager());
                fileName = fileName.substr(0, fileName.find_first_of(':'));
                myFunctionFeat[index].fileName = fileName;
            }

            // Record the call graph
			myFunctionFeat[index].calledNumber++;
            myFunctionFeat[index].callerID.insert(callerID);

            // Get the code text of a callexpr
            //StringRef callText =Lexer::getSourceText(CharSourceRange::getTokenRange(callExpr->getSourceRange()), CI->getSourceManager(), LangOptions(), 0);
            FullSourceLoc fullStartCallText(callExpr->getLocStart(), CI->getSourceManager());
            fullStartCallText = fullStartCallText.getSpellingLoc();
            FullSourceLoc fullEndCallText(callExpr->getLocEnd(), CI->getSourceManager());
            fullEndCallText = fullEndCallText.getSpellingLoc();
            SourceRange sr(fullStartCallText, fullEndCallText);
            StringRef callText =Lexer::getSourceText(CharSourceRange::getTokenRange(sr), CI->getSourceManager(), LangOptions(), 0);
            if(callText.find('(') != string::npos){
                callText = callText.substr(callText.find('('));
            }
            //errs()<<callText<<"\n";
            //errs()<<name<<"@"<<fullStartCallText.getExpansionLineNumber()<<"\n";

            // Another way to obtain callText.
            //clang::SourceLocation b(callExpr->getLocStart()), _e(callExpr->getLocEnd());
            //clang::SourceLocation e(clang::Lexer::getLocForEndOfToken(_e, 0, CI->getSourceManager(), LangOptions()));
            //errs()<<string(CI->getSourceManager().getCharacterData(b),CI->getSourceManager().getCharacterData(e)-CI->getSourceManager().getCharacterData(b))<<"\n";

            // Check whether the code text of the callexpr contains error-related semantics.
            if(hasExitKeyword(callText) || hasErrorKeyword(callText) ||
               hasLimitationKeyword(callText) || hasNonCorrectKeyword(callText)){
                myFunctionFeat[index].errorNumber++;
            }
            else{
                myFunctionFeat[index].normalNumber++;
            }
		}
        else{
            // FIXME I don't know how to deal with function pointer :(
            //callExpr->dump();
        }
	}

    // Travel the statement, recursively.
	for (Stmt::child_iterator it = stmt->child_begin(); it != stmt->child_end(); ++it){
        if(Stmt *child = *it){
            if(isa<ImplicitCastExpr>(stmt) || isa<ParenExpr>(stmt)){
                travelStmt(child, parent);
            }
            else{
                travelStmt(child, stmt);
            }
        }
	}

    if(isChange)
        isInIfCond = false;
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

    callerID  = 0;

    // For each declaration
	if(Declaration->getBody()){

        // Get the function index
        string name = Declaration->getNameAsString();
		if(myFunctionFeatMap[name] == 0){
			myFunctionFeatMap[name] = ++myFunctionFeatCnt;
		}
		int index = myFunctionFeatMap[name];
		if(index == MAX_FUNC_NUM){
			errs()<<"Too many functions!\n";
			exit(1);
		}
        callerID = index;

        // Get the lenth of the function
		unsigned int start, end;
		FullSourceLoc functionstart = CI->getASTContext().getFullLoc(Declaration->getLocStart()).getExpansionLoc();
		FullSourceLoc functionend = CI->getASTContext().getFullLoc(Declaration->getLocEnd()).getExpansionLoc();
		start = functionstart.getExpansionLineNumber();
		end = functionend.getExpansionLineNumber();
		myFunctionFeat[index].lenth = end - start;

        // Get the file name, function name, and function keyword
        string fileName = InFile;
        myFunctionFeat[index].funcName = name;
        myFunctionFeat[index].funcKeyWord = hasKeyword(name);
        myFunctionFeat[index].fileName = getStemName(fileName);

        // Travel the function body and get logging statement
        mapFuncRet.clear();
        callList.clear();
        logListMap.clear();
        ifList.clear();
        switchList.clear();
        isInIfCond = false;
		travelStmt(Declaration->getBody(), Declaration->getBody());

        // Extract logging behavior
        extractLoggingSnippet(Declaration);
	}
	return true;
}


void FindLoggingConsumer::HandleTranslationUnit(ASTContext& Context) {
	//fileNumMap.clear();
	Visitor.TraverseDecl(Context.getTranslationUnitDecl());
	return;
}


std::unique_ptr<ASTConsumer> FindLoggingAction::CreateASTConsumer(CompilerInstance& Compiler, StringRef InFile) {

    // Record the processing file name and number.
	if(InFile == lastName)
		return 0;
	else{
		llvm::errs()<<"["<<++fileNum<<"/"<<fileTotalNum<<"]"<<" Now analyze the file: "<<InFile<<"\n";
		lastName = InFile;
        return std::unique_ptr<ASTConsumer>(new FindLoggingConsumer(&Compiler, InFile));
    }
}

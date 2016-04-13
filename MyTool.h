#ifndef MYTOOL_H
#define MYTOOL_H

#include "llvm/Support/raw_ostream.h"
#include "llvm/Option/OptTable.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Driver/Options.h"
#include <stdio.h>
#include <stdlib.h>

#include "RecordCond.h"
#include "InsertLog.h"
#include "FindLogFunc.h"
#include "CountRetCode.h"

using namespace clang::tooling;
using namespace clang;
using namespace llvm;
using namespace llvm::opt;
using namespace clang::driver;
using namespace std;

#define MAX_FILE_NUM 10000
#define MAX_LOGGED_FUNC 10000
#define MAX_UNLOGGED_FUNC 5000


#define DEBUG 0
#define DEBUG_CFG_DUMP 0

#define DEBUG_FUNC "store_body"

#endif

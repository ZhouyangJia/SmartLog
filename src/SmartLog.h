//
//  SmartLog.h
//  LLVM
//
//  Created by ZhouyangJia on 16/4/13.
//
//

#ifndef SmartLog_h
#define SmartLog_h

#include "llvm/Support/raw_ostream.h"
#include "llvm/Option/OptTable.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Driver/Options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cxxabi.h>
#include <iostream>

using namespace clang::tooling;
using namespace clang;
using namespace llvm;
using namespace llvm::opt;
using namespace clang::driver;
using namespace std;

#define MAX_FUNC_NUM 100000
#define MAX_LOG_FUNC 10000

#endif /* SmartLog_h */

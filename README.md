# SmartLog
Error-Log Statement Placement by Deep Understanding of Log Intention.

---

### Introduction
*SmartLog* is a log automation tool. Existing log automation tools define log placement rules by extracting syntax features or summarizing code patterns. These approaches are limited since the log placements are far beyond those rules but are according to the intention of software code. We design *SmartLog*, an accurate log automation tool that places error-log statements by deep understanding of log intention.


### Usage

##### Compile
- Change directory to Clang tools directory.
```sh
cd path/to/llvm-source-tree/tools/clang/tools
```
- Download source code.
```sh
git clone https://github.com/ZhouyangJia/SmartLog.git
```
- Add *add_subdirectory(SmartLog)* to CMakeList.txt.
```sh
echo "add_subdirectory(SmartLog)" >> CMakeLists.txt
```
- [Recompile Clang](http://llvm.org/docs/CMake.html) and try following command to test.
```sh
ls path/to/llvm-build/Release/bin/clang-smartlog
```

##### Run
- Generate *compile_commands.json*. More infomation about [compile_commands.json](http://clang.llvm.org/docs/JSONCompilationDatabase.html).
```sh
cd test/
tar zxvf bftpd-3.2.tar
cd bftpd/
./configure
bear make
```
- Generate *compiled_files.def*, This file has all names of compiled source files.
```sh
../../script/extract_command.pl compile_commands.json
```
- Generate call dependence before running SmartLog (Compile [CallDependence](https://github.com/ZhouyangJia/CallDependence) tool first).
```sh
../../script/call_dependence.sh
```
- Generate *logging_statement.out*, *logging_behavior.out*, *normalizes_behavior.out* and *logging_rules.out*.
```sh
cat compiled_files.def | xargs clang-smartlog -find-logging-behavior > logging_rules.out
```

**logging_statement.out**. The logging statements, including function name and call location.

**logging_behavior.out**. The logging behavior, including error-prone function name, function call expression, function call location, argument index, log type (SA for sensitive argement, RV for return value and PA for pointer argument), check condition, check location, logging statement, log approach (LL for local log, RL for remote log and NL for no log), log location.

**normalizes_behavior.out**. The normalized behavior, including error-prone function name, argument index, log type, normalized check condition, log approach.

- Generate patches.
```sh
python ../../script/patch.py > patch.out
```

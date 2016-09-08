# SmartLog
A Semantic-Aware Log Automation Tool for Failure Diagnosis.

---

### Introduction
*SmartLog* is a log automation tool. Through learning logging rules from existing logs, *SmartLog* can decide whether to log a given code snippet or not , and finally inserts logging statement when necessary.


### Usage

##### Compile
- Download the source code.
- Make new fold *clang-smartlog* in Clang tools directory, e.g., /home/guest/llvm-3.8/tools/clang/tools/clang-smartlog.
- Extract source code to above new fold.
- Add *add_clang_subdirectory(clang-smartlog)* in CMakeList.txt in Clang tools directory.
- Compile Clang, SmartLog is dependent on Python library, maybe you can use -I/usr/include/python2.7 and -lpython2.7.

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


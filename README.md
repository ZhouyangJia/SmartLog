# clang-smartlog
A Semantic-Aware Log Automation Tool for Failure Diagnosis

### Introduction
*LogGrad* is a light weight log-quality assessment tool, which is based on Clang.
Given a set of projects, *LogGrad* can automatically rank them by log quality.
The basic idea of ranking is that to what extent the given project misses logs.
The missing logs are classified into 3 types, including project-specific logs, intra-domain important logs and inter-domain important logs.

### Usage

##### Compile
- Download the source code.
- Make new fold *clang-loggrad* in Clang tools directory, e.g., /home/guest/llvm-3.8/tools/clang/tools/clang-loggrad.
- Extract source code to above new fold.
- Add *add_clang_subdirectory(clang-loggrad)* in CMakeList.txt in Clang tools directory.
- Compile Clang.

##### Analyze log information
- Generate *compile_commands.json*. More infomation about [compile_commands.json](http://clang.llvm.org/docs/JSONCompilationDatabase.html).
```sh
./configure
bear make
```
- Generate *compiled_files.def*, This file has all names of compiled source files.
```sh
extract_command.pl compile_commands.json
```
- Generate *function_rule_model.out*. Each line includes a 3-tuple, namely function name, called time and logged time.
```sh
cat compiled_files.def | xargs clang-loggrad -log-grade
```
- Remane the output file. Other output files are in script/results.
```sh
mv function_rule_model.out bftpd.out
```
##### Assessment
Each output file represents one project followed by a domain name.
```sh
python loggrad.py results/httpd.out httpd results/nginx.out httpd results/lighttpd.out httpd results/mongrel2.out httpd results/mysql.out database results/postgresql.out database results/berkeleydb.out database results/monetdb.out database
```

# clang-smartlog

Please put the whole script(smartlog-script) in the top level of source code direction!

Compile the source code AND generate compile_commands.json file:
$bear make
or for a cmake project, use:
$cmake /path/to/source/tree -CMAKE_EXPORT_COMPILE_COMMANDS=on
	
Then, do:
$cd smartlog-script

Before running, you should make sure two paths and change it in smartlog.sh
$LLVMLIB=$HOME/llvm-3.4/Release+Asserts/lib
$CLANGTOOL=$HOME/llvm-3.4/Release+Asserts/bin

first, you shold do:
$chmod 777 ./smartlog.sh ./extract_command.pl

you can get the help by:
$./smartlog.sh -h

run quickly by:
$./smartlog.sh -all 2> logfile

patch the additional logs by:
$./smartlog.sh -patch

recover the patched logs by:
$./smartlog.sh -recover

if you want to add logging functions manually, please write them into logging_function.in


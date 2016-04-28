#!/bin/bash

#LLVM_LIB=$HOME/llvm-3.4/Release+Asserts/lib
#LLVM_BIN=$HOME/llvm-3.4/Release+Asserts/bin
LLVM_LIB=/Users/zhouyangjia/llvm-3.8/Debug/lib
LLVM_BIN=/Users/zhouyangjia/llvm-3.8/Debug/bin


check(){
	if [ $? -ne 0 ]
	then
		echo "Failed!"
		exit 1
	fi
}

extract(){
	echo "SmartLog extracts compile command (generate ./compiled_files.def & ./build_ir.sh from ../compile_commands.json)..."
	`./extract_command.pl ../compile_commands.json`
	check
}

build(){
	echo "SmartLog builds LLVM IR (generate .bc and .ll files in source tree from ./build_ir.sh)..."
	#`./build_ir.sh`
	check
}

logging_function(){
	echo "SmartLog finds logging functions (generate ./logging_function.out from .bc files & ./compiled_files.def & ./logging_function.in)..."
	`rm -rf logging_function.out`;
    cd ..;
    path=`pwd`
	files=`find ${path} -name "*.bc"`;
    cd smartlog-script;
	totalcnt=${#files[@]}
	count=1;
	for file in ${files[*]}
	do
		echo -e "[0/2][$count/$totalcnt] Now analyze the file: $file" >&2;
		count=`expr $count + 1`;
		`$LLVM_BIN/opt -load $LLVM_LIB/Smartlog.dylib -find-log-function < $file > /dev/null`;
		check
	done
	`cat ./compiled_files.def | xargs $LLVM_BIN/clang-mytool -find-logging-function > result.csv`
    #`rm -rf function_flow.out`
	check
	`cat logging_function.in >> logging_function.out`
	check
}

feature(){
	echo "SmartLog calculates function features (generate ./logging_function.csv from .bc files & ./compiled_files.def & ./logging_function.in)..."
    cd ..;
    path=`pwd`
    files=`find ${path} -name "*.bc"`;
    cd smartlog-script;
	count=1;
	for file in ${files[*]}
	do
		echo -e "$count Now analyze the file: $file" >&2;
		count=`expr $count + 1`;
		`$LLVM_BIN/opt -load $LLVM_LIB/Smartlog.dylib -find-log-function < $file > /dev/null`;
		check
	done
	`cat ./compiled_files.def | xargs $LLVM_BIN/clang-mytool -logging-function-features > logging_function.csv`
	`rm -rf function_flow.out`
	check
	check
}
logged_snippet(){
	echo "SmartLog finds logged snippets (generate ./logged_snippet.out from ./logging_function.out)..."
	`rm -rf logged_snippet.out`;
    cd ..;
    path=`pwd`
    files=`find ${path} -name "*.bc"`;
    cd smartlog-script;
	count=1;
	for file in ${files[*]}
	do
		echo -e "$count Now analyze the file: $file" >&2;
		count=`expr $count + 1`;
		`$LLVM_BIN/opt -load $LLVM_LIB/SmartLog.dylib -find-logged-function < $file > /dev/null`;
		check
	done
}

influencing_condition(){
	echo "SmartLog finds influncing conditions (generate ./influencing_condition.out from ./logged_snippet.out)..."
	`cat ./compiled_files.def | xargs $LLVM_BIN/clang-mytool -find-log-condition > influencing_condition.out`
	check
}

result(){
	echo "SmartLog generates results (generate ./overall_result.out from ./influencing_condition.out)..."
	`cat ./compiled_files.def | xargs $LLVM_BIN/clang-mytool -insert-log-statement > overall_result.out`
	check
}

generate_patch(){
	echo "SmartLog generates patches (generate ./patches/* from ./influencing_condition.out)..."
	`cat ./compiled_files.def | xargs $LLVM_BIN/clang-mytool -insert-log-statement > overall_result.out`
	check

	#recover before remove when forgetting to do it
	cd ..
	patches=`find ./smartlog-script/patches/ -name '*.patch' 2> /dev/null`;
	for patch in ${patches[*]}
	do
		patch -R -p1 < ${patch} > /dev/null
	done
	cd ./smartlog-script

	#remove the old result
	`rm -rf patches 2> /dev/null`;
	`mkdir patches`;
	patches=`find ../ -name '*.SLout'`;
	for patch in ${patches[*]}
	do
		source_file=${patch:0:-6};
		patch_file="patches/`basename "${source_file}.patch"`";
		`cat ${patch} | diff -rNu ${source_file} - > ${patch_file}`;
	done
	`find ../ -name '*.SLout' | xargs rm -rf`
}

mypatch(){
	echo "SmartLog patches the added log statement..."
	cd ..
	patches=`find ./smartlog-script/patches/ -name '*.patch'`;
	for patch in ${patches[*]}
	do
		patch -p1 < ${patch} > /dev/null
	done
	cd ./smartlog-script
}

recover(){
	echo "SmartLog recovers original source code..."
	cd ..
	patches=`find ./smartlog-script/patches/ -name '*.patch'`;
	for patch in ${patches[*]}
	do
		patch -R -p1 < ${patch}
	done
	cd ./smartlog-script
}

error_code(){
	echo "SmartLog counts error return code and total return statement..."
	`cat ./compiled_files.def | xargs $LLVM_BIN/clang-mytool -count-return-code`
	check
}

#main entrance
option="${1}"
case ${option} in

#one button running
-all)
	echo "SmartLog will generate patches adding log automatically!"

	extract

	build

	logging_function

	logged_snippet

	influencing_condition

	generate_patch

	echo "SmartLog finished!"
	;;

#step by step running
-extract)
	extract
	;;
-build) 
	build
	;;
-logging)
	logging_function
	;;
-logged)
	logged_snippet
	;;
-condition)
	influencing_condition
	;;
-generate)
	generate_patch
	;;

#patch & recover
-patch)
	mypatch
	;;
-recover)
	recover
	;;

#util: count error code
-error-code)
	error_code
	;;
-feature)
	feature
	;;
-result)
	result
	;;
-h|--help)
	echo "`basename ${0}`: usage: -option"
	echo -e "Quick Running:"
	echo -e "\t-all\t\trun smartlog automatically"

	echo -e "\nRunning step by step: (please run these options IN ORDER!)"
	echo -e "\t-extract\textract complile information to bulid LLVM IR, this command will generate build_ir.sh and compiled_files.def"
	echo -e "\t-build\t\tbuild LLVM IR, this command will generate .ll and .bc files for each compiled file in the source tree"
	echo -e "\t-logging\t\tfind logging function"
	echo -e "\t-logged\t\tfind logged snippet and output to logged_snippet.out"
	echo -e "\t-condition\tfind influencing checking condition and output to influencing_condition.out"
	echo -e "\t-generate\t\tinsert log statement, write to *.SLout files and output overall information to overall_result.out"

	echo -e "\nPatch and recover"
	echo -e "\t-patch\t\tpatch the added log statement into source code"
	echo -e "\t-recover\trecover source code"

	echo -e "\nOthers:"
	echo -e "\t-error-code\tcount error return code and total return statement"
	echo -e "\t-feature\tcalculate features of all functions"
	echo -e "\t-result\tlist all additional logs and locations"
	echo -e "\t-h|--help\tdiaplay this help"
	;;
*)
	echo "Unknown command, see more detial using `basename ${0}` -h or --help"
	exit 1
	;;
esac



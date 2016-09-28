`rm -rf call_dependence.csv`;
path=`pwd`
files=`find ${path} -name "*.bc"`;
arr=($files)
num=${#arr[@]}
count=1;
for file in ${arr[*]}
do  
    echo -e "[$count/$num] Now analyze the file: $file" >&2;
    count=`expr $count + 1`; 
    `opt -load LLVMCallDependence.dylib -call-dependence $file > /dev/null`;
done

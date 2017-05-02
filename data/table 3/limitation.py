import os, sys, string

pattern_file = open("errlog.pattern.input","r")
pattern_lines = pattern_file.readlines()
pattern_file.close()

for i in range(1, len(sys.argv)):
    print i, sys.argv[i]
    target_file = open(sys.argv[i],"r")

    count_line = 0
    count_pattern = 0
    for line in target_file:
    	line = line.strip()
    	if len(line) == 0:
    		continue
    	count_line += 1
    	for pattern in pattern_lines:
    		if line in pattern:
    			count_pattern += 1
    			break;
    print count_line, count_pattern

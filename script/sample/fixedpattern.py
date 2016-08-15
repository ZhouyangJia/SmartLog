import os,string

sample = open("fixedpattern_sample1_result.in","r")
pattern = open("fixedpattern_pattern_snippet.in","r")

target = open("sample.out","w")

sample_lines = sample.readlines()
pattern_lines = pattern.readlines()

for line in sample_lines:

	loc = line.split(' ')[0]
	typ = line.split(' ')[1]
	typ = typ.strip()

	flag = 0

	for row in pattern_lines:
		if loc in row:
			flag = 1
			break;

	target.write(loc + ' ' + typ)

	if flag == 1:
		target.write(' fixed_pattern')
	else:
		target.write(' free_pattern')

	target.write('\n')

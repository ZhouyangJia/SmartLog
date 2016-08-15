import os,string

output_file = open('output_snippet.out')
logged_file = open('logged_snippet.out')
pattern_file = open('pattern_snippet.out')
sample_file = open('sample_result.out')

#print len(output_file.readlines())
#print len(logged_file.readlines())
#print len(pattern_file.readlines())
#print len(sample_file.readlines())

output_list = output_file.readlines()
logged_list = logged_file.readlines()
pattern_list = pattern_file.readlines()
sample_list = sample_file.readlines()

output_true = 0
logged_true = 0
pattern_true = 0
sample_true = 0

output_false = 0
logged_false = 0
pattern_false = 0
sample_false = 0

step = 3
num = 0

for sample in sample_list:

	sample = sample.split(' ')

	loc = sample[0]
	tag = sample[1].strip('\n')



	if int(tag) == 1:
		sample_true += 1

		for output in output_list:
			log = output.split('#')[1]
			if loc in log:
				output_true += 1
				break

		for logged in logged_list:
			log = logged.split('#')[1]
			if loc in log:
				logged_true += 1
				break

		for pattern in pattern_list:
			log = pattern.split('#')[1]
			if loc in log:
				pattern_true += 1
				break

	else:

		num += 1
		if num%step == 0:
			continue;


		sample_false += 1

		has_loc = 0
		for output in output_list:
			log = output.split('#')[1]
			if loc in log:
				has_loc = 1
				break
		if has_loc == 0:
			output_false += 1


		has_loc = 0
		for logged in logged_list:
			log = logged.split('#')[1]
			if loc in log:
				has_loc = 1
				break
		if has_loc == 0:
			logged_false += 1


		has_loc = 0
		for pattern in pattern_list:
			log = pattern.split('#')[1]
			if loc in log:
				has_loc = 1
				break
		if has_loc == 0:
			pattern_false += 1

print output_true 
print logged_true 
print pattern_true
print sample_true 
print 
print output_false
print logged_false
print pattern_false
print sample_false

output_TP = output_true 
output_FP = sample_false - output_false
output_FN = sample_true - output_true
output_TN = output_false
output_P = float(output_TP)/(float(output_TP) + float(output_FP))
output_R = float(output_TP)/(float(output_TP) + float(output_FN))
output_N = float(output_TN)/(float(output_TN) + float(output_FN))
output_F = (2*output_P*output_R)/(output_P + output_R)
output_BA = float(output_TP)/(float(output_TP) + float(output_FN)) \
+ float(output_TN)/(float(output_TN) + float(output_FP))
output_BA /= 2

logged_TP = logged_true 
logged_FP = sample_false - logged_false
logged_FN = sample_true - logged_true
logged_TN = logged_false
logged_P = float(logged_TP)/(float(logged_TP) + float(logged_FP))
logged_R = float(logged_TP)/(float(logged_TP) + float(logged_FN))
logged_N = float(logged_TN)/(float(logged_TN) + float(logged_FN))
logged_F = (2*logged_P*logged_R)/(logged_P + logged_R)
logged_BA = float(logged_TP)/(float(logged_TP) + float(logged_FN)) \
+ float(logged_TN)/(float(logged_TN) + float(logged_FP))
logged_BA /= 2

pattern_TP = pattern_true 
pattern_FP = sample_false - pattern_false
pattern_FN = sample_true - pattern_true
pattern_TN = pattern_false
pattern_P = float(pattern_TP)/(float(pattern_TP) + float(pattern_FP))
pattern_R = float(pattern_TP)/(float(pattern_TP) + float(pattern_FN))
pattern_N = float(pattern_TN)/(float(pattern_TN) + float(pattern_FN))
pattern_F = (2*pattern_P*pattern_R)/(pattern_P + pattern_R)
pattern_BA = float(pattern_TP)/(float(pattern_TP) + float(pattern_FN)) \
+ float(pattern_TN)/(float(pattern_TN) + float(pattern_FP))
pattern_BA /= 2

output_TP
output_FP
output_FN
output_TN

print
print output_TP
print output_FP
print output_FN
print output_TN
print output_P 
print output_R 
print output_N
print output_F 
print output_BA 
print
print logged_TP
print logged_FP
print logged_FN
print logged_TN
print logged_P 
print logged_R 
print logged_N
print logged_F 
print logged_BA 
print
print pattern_TP
print pattern_FP
print pattern_FN
print pattern_TN
print pattern_P 
print pattern_R 
print pattern_N
print pattern_F 
print pattern_BA 
print

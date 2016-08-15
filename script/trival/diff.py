import os

keyword_file = open('keyword_callsite.out')
output_file = open('pattern_snippet.out')

keyword_list = keyword_file.readlines()
output_list1 = output_file.readlines()


output_list2 = []
for output in output_list1:
	out = output.split('@')[-1]
	output_list2.append(out)


diff1 = list(set(output_list2)-set(keyword_list))
diff2 = list(set(keyword_list)-set(output_list2))

print len(diff1)
for i in range(10):
	print diff1[i]

print

print len(diff2)
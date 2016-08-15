import os,sys,re,string,random,linecache

#/Users/zhouyangjia/source/httpd-2.4.10/smartlog-script

def get_call_list(path):

	loc_name = string.join([path, "keyword_callsite.out"], '/')
	loc_file = open(loc_name)

	compile_name = string.join([path, "compiled_files.def"], '/')
	compile_file = open(compile_name)

	compile_lines = compile_file.readlines()

	call_list = []

	for line in loc_file:

		line = line.strip('\n')
		line = string.split(line, ' ')

		call_loc_raw = line[0]
		call_loc_raw = string.split(call_loc_raw, ':')

		call_loc_file = call_loc_raw[0]
		call_loc_line = call_loc_raw[1]

		call_loc_file = string.split(call_loc_file, '/')
		call_loc_file = call_loc_file[-1]

		for full_file in compile_lines:
			if call_loc_file in full_file:
				call_loc_file = full_file
				break

		call_loc = (call_loc_file, call_loc_line)
		call_list.append(call_loc)

	loc_file.close()
	compile_file.close()

	return call_list

def print_call_snippet(call_loc):

	print call_loc
	call_loc_file = call_loc[0]
	call_loc_file = call_loc_file.strip('\n')
	call_loc_line = int(call_loc[1])

	mymin = max(0, call_loc_line - 20)
	mymax = min(len(linecache.getlines(call_loc_file)), call_loc_line + 21)

	print "/*************** source code snippet begin ***************/"
	for i in range(mymin, mymax):
		if i == call_loc_line:
			print "CHOOSE:",
		print linecache.getline(call_loc_file,i).strip('\n')
	print "/*************** source code snippet end ***************/"

def check_snippet(call_list, num):

	result_dict = {}

	while num != 0:

		call_loc = random.choice(call_list)

		print_call_snippet(call_loc)

		print str(num) + "  cases left!"
		choice = input("Please check above snippet:(0 not output, 1 for result, 2 for audit, 3 for debug, 4 for log, 5 others)\n")

		if int(choice) >= 1 and int(choice) <= 5:
			result_dict[call_loc] = choice
			num -= 1
		elif int(choice) == 0:
			result_dict[call_loc] = choice
		elif int(choice) == 9:
			pass
		else:
			print "Invalid input!"

	return result_dict

def print_result(result_dict):

	result_file = open("sample1_result.out", 'w')

	for item in result_dict:
		print item, result_dict[item]
		call_loc_file = item[0]
		call_loc_line = item[1]
		result_file.write(call_loc_file.strip('\n') + ':' + call_loc_line)
		result_file.write(' ' + str(result_dict[item]) + '\n')


if __name__ == '__main__':

	#path = raw_input("Please input the path to smartlog-script:\n")
	path = os.getcwd()
	call_list = get_call_list(path)

	num = input("Please input the number of snippet to tag:\n")

	result_dict = check_snippet(call_list,int(num))
	print_result(result_dict)

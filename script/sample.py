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
			print "CALL:",
		print linecache.getline(call_loc_file,i).strip('\n')
	print "/*************** source code snippet end ***************/"

def check_snippet(call_list, num):

	true = num
	false = num

	true_list = []
	false_list = []
	not_log_list = []
	not_sure_list = []

	while True:

		call_loc = random.choice(call_list)

		print_call_snippet(call_loc)

		print str(true) + " true cases left and "+ str(false) + " false cases left!" 
		choice = input("Please check above snippet:(0 for No, 1 for Yes, 2 for Not Sure)\n")

	
		if choice == 0:
			false -= 1
			false_list.append(call_loc)
			print "Choice No!"
		elif choice == 1:
			true -= 1
			true_list.append(call_loc)
			print "Choice Yes!"
		elif choice == 2:
			not_sure_list.append(call_loc)
			print "Not Sure!"
		else:
			print "Invalid input!"

		if true <= 0 and false <= 0:
			break

	return true_list,false_list,not_log_list,not_sure_list

def print_result(true_list,false_list,not_log_list,not_sure_list):

	result_file = open("sample_result.out", 'w')

	print "/*************** Final result ***************/"

	print "False cases:"
	for item in false_list:
		print item
		call_loc_file = item[0]
		call_loc_line = item[1]
		result_file.write(call_loc_file.strip('\n'))
		result_file.write(':')
		result_file.write(call_loc_line)
		result_file.write(' 0\n')

	print "True cases:"
	for item in true_list:
		print item
		call_loc_file = item[0]
		call_loc_line = item[1]
		result_file.write(call_loc_file.strip('\n'))
		result_file.write(':')
		result_file.write(call_loc_line)
		result_file.write(' 1\n')
			
	print "Not Sure:"
	for item in not_sure_list:
		print item

if __name__ == '__main__':

	#path = raw_input("Please input the path to smartlog-script:\n")
	path = os.getcwd()
	call_list = get_call_list(path)

	num = input("Please input the number of snippet to tag:\n")
	true_list,false_list,not_log_list,not_sure_list = check_snippet(call_list,num)

	print_result(true_list,false_list,not_log_list,not_sure_list)

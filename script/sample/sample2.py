import os,sys,re,string,random,linecache

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


if __name__ == '__main__':
	
	snippet_file = open("sample2_logged_snippet.in","r")
	snippet_lines = snippet_file.readlines()
	
	func_name = open("sample2_selected_func.in","r")
	func_lines = func_name.readlines()

	cnt = 0
	tot = len(func_lines)

	for line in func_lines:
		line = line.strip()
		project = line.split(',')[0]
		func = line.split(',')[1]
		num = line.split(',')[2]


		cnt = cnt + 1
		print 'Now analyze \"' + func + '\" (' + str(cnt) + '/' + str(tot) + ').'

		count = 0

		for call in snippet_lines:
			if 'Analyze' in call:
				continue

			call = call.strip()
			call = call.split('#')[0]
			name = call.split('@')[0]
			loc = call.split('@')[1]
			file = loc.split(':')[0]
			linenum = loc.split(':')[1]
			
			if project not in file:
				continue

			if func == name:
				count = count + 1
				print 'Please check ' + str(count) + '/' + num + ':'
				print_call_snippet([file, linenum])



				






	#result_dict = check_snippet(call_list,int(num))


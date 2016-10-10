import os,sys,re,string,random,linecache

def print_call_snippet(call_loc):

	print call_loc

	call_loc_file = call_loc.split(':')[0]
	call_loc_line = (int)(call_loc.split(':')[1])

	mymin = max(0, call_loc_line - 20)
	mymax = min(len(linecache.getlines(call_loc_file)), call_loc_line + 21)

	print "/*************** source code snippet begin ***************/"
	for i in range(mymin, mymax):
		if i == call_loc_line:
			print "$:$ ",
		print linecache.getline(call_loc_file,i).strip('\n')
	print "/*************** source code snippet end ***************/"

def compare1(x, y):
	segx = x.split(';')
	segy = y.split(';')
	retf = (float)(segx[7]) - (float)(segy[7])
	reti = 1
	if retf > 0:
		reti = -1
	elif retf == 0:
		retnum = (int)(segx[4]) - (int)(segy[4])
		if retnum > 0:
			reti = -1
	return reti

def compare2(x, y):
	segx = x.split(';')
	segy = y.split(';')
	retf = (float)(segx[12]) - (float)(segy[12])
	reti = 1
	if retf > 0:
		reti = -1
	return reti

if __name__ == '__main__':
	
	call_file = open("call_statement.out","r")
	call_lines = call_file.readlines()
	
	rule_file = open("logging_rules.out","r")
	rule_lines = rule_file.readlines()

	unlogged_func_file = open("unlogged_func.out","w")

	sorted(rule_lines)

	LL_SA = 0
	LL_PA = 0
	LL_RV = 0
	LL_rule_lines = []
	RL_SA = 0
	RL_PA = 0
	RL_RV = 0
	RL_rule_lines = []
	for rule_line in rule_lines:
		rule_line = string.strip(rule_line)
		rule_seg = rule_line.split(';')
		flag = 0
		mytype = rule_seg[19].split('#')[2]
		if (int)(rule_seg[1]) >= 3 and (int)(rule_seg[1]) <= (int)(rule_seg[4]) * 3 and (int)(rule_seg[4]) >= 2 and (float)(rule_seg[7]) > 0.32999 and (float)(rule_seg[8]) > 0.99999:
		#if (int)(rule_seg[1]) >= 2 and (float)(rule_seg[7]) > 0.32999 and (float)(rule_seg[8]) > 0.99999:
			LL_rule_lines.append(rule_line)
			if mytype == "SA":
				LL_SA += 1
			elif mytype == "PA":
				LL_PA += 1
			else:
				LL_RV += 1
		if (int)(rule_seg[1]) >= 3 and (int)(rule_seg[1]) <= (int)(rule_seg[9]) * 3 and (int)(rule_seg[9]) >= 2 and (float)(rule_seg[12]) > 0.32999 and (float)(rule_seg[13]) > 0.99999:
			RL_rule_lines.append(rule_line)
			if mytype == "SA":
				RL_SA += 1
			elif mytype == "PA":
				RL_PA += 1
			else:
				RL_RV += 1


	print "Local log rules: "+(str)(len(LL_rule_lines))
	print "SA: " + (str)(LL_SA) + " PA: " + (str)(LL_PA) + " RV: " + (str)(LL_RV)
	print "Remote log rules: "+(str)(len(RL_rule_lines))
	print "SA: " + (str)(RL_SA) + " PA: " + (str)(RL_PA) + " RV: " + (str)(RL_RV)
	LL_rule_lines = sorted(LL_rule_lines, cmp = compare1)
	RL_rule_lines = sorted(RL_rule_lines, cmp = compare2)


	cnt = 0
	tot = len(LL_rule_lines)
	ll_patch_cnt = 0
	ll_patch_cnt_SA = 0
	ll_patch_cnt_PA = 0
	ll_patch_cnt_RV = 0


	patch_cnt = 0
	patch_cnt_SA = 0
	patch_cnt_PA = 0
	patch_cnt_RV = 0
	for LL_rule_line in LL_rule_lines:
		cnt += 1
		rule_seg = LL_rule_line.split(';')
		rule_name = rule_seg[0]
		rule_context = rule_seg[19:]

		print 'Now analyze local logs \"' + rule_name + '\"(' + str(cnt) + '/' + str(tot) + ').'
		tmplist = rule_seg[1:9]
		tmplist.extend(rule_seg[19:])
		print tmplist



		handle_list = []
		unhandle_list = []
		unhandle_cnt = 0
		unhandle_cnt_SA = 0
		unhandle_cnt_PA = 0
		unhandle_cnt_RV = 0
		mytype = rule_seg[19].split('#')[2]

		#final number
		call_num = int(tmplist[0])
		behavior_num = int(tmplist[3])
		patch_num = 0
		if behavior_num < call_num:
			patch_num = call_num - behavior_num
		patch_cnt += patch_num
		if mytype == "SA":
			patch_cnt_SA += patch_num
		elif mytype == "PA":
			patch_cnt_PA += patch_num
		else:
			patch_cnt_RV += patch_num

		for call_line in call_lines:
			call_line = string.strip(call_line)
			call_name = call_line.split(' ')[0]
			call_loc = call_line.split(' ')[1]
			call_handle = call_line.split(' ')[2]

			if call_name == rule_name and call_handle == "1":
				handle_list.append(call_loc)
			if call_name == rule_name and call_handle == "0":
				unhandle_list.append(call_loc)
				if mytype == "SA":
					unhandle_cnt_SA += 1
				elif mytype == "PA":
					unhandle_cnt_PA += 1
				else:
					unhandle_cnt_RV += 1
				unhandle_cnt += 1

		print 'Total ' + mytype + ' ' + str(unhandle_cnt) + ' unhandled calls.'
		if unhandle_cnt != 0 and mytype == "RV":
			unlogged_func_file.write(rule_name)
			unlogged_func_file.write('\n')
		ll_patch_cnt += unhandle_cnt
		ll_patch_cnt_SA += unhandle_cnt_SA
		ll_patch_cnt_PA += unhandle_cnt_PA
		ll_patch_cnt_RV += unhandle_cnt_RV
		for item in unhandle_list:
			print_call_snippet(item)
		for item in handle_list:
			print_call_snippet(item)
				


	cnt = 0
	tot = len(RL_rule_lines)
	rl_patch_cnt = 0
	rl_patch_cnt_SA = 0
	rl_patch_cnt_PA = 0
	rl_patch_cnt_RV = 0
	for RL_rule_line in RL_rule_lines:
		cnt += 1
		rule_seg = RL_rule_line.split(';')
		rule_name = rule_seg[0]
		rule_context = rule_seg[19:]

		print 'Now analyze remote logs \"' + rule_name + '\"(' + str(cnt) + '/' + str(tot) + ').'
		tmplist = rule_seg[1:4]
		tmplist.extend(rule_seg[9:14])
		tmplist.extend(rule_seg[19:])
		print tmplist

		handle_list = []
		unhandle_list = []
		unhandle_cnt = 0
		unhandle_cnt_SA = 0
		unhandle_cnt_PA = 0
		unhandle_cnt_RV = 0
		mytype = rule_seg[19].split('#')[2]
		for call_line in call_lines:
			call_line = string.strip(call_line)
			call_name = call_line.split(' ')[0]
			call_loc = call_line.split(' ')[1]
			call_handle = call_line.split(' ')[2]

			if call_name == rule_name and call_handle == "1":
				handle_list.append(call_loc)
			if call_name == rule_name and call_handle == "0":
				unhandle_list.append(call_loc)
				if mytype == "SA":
					unhandle_cnt_SA += 1
				elif mytype == "PA":
					unhandle_cnt_PA += 1
				else:
					unhandle_cnt_RV += 1
				unhandle_cnt += 1

		print 'Total ' + mytype + ' ' + str(unhandle_cnt) + ' unhandled calls.'
		rl_patch_cnt += unhandle_cnt
		rl_patch_cnt_SA += unhandle_cnt_SA
		rl_patch_cnt_PA += unhandle_cnt_PA
		rl_patch_cnt_RV += unhandle_cnt_RV
		for item in unhandle_list:
			print_call_snippet(item)
		for item in handle_list:
			print_call_snippet(item)

	print 'Total patch: ' + str(patch_cnt)
	print 'Total SA: ' + str(patch_cnt_SA)
	print 'Total PA: ' + str(patch_cnt_PA)
	print 'Total RV: ' + str(patch_cnt_RV)
	print 'Total LL patch: ' + str(ll_patch_cnt)
	print 'Total LL SA: ' + str(ll_patch_cnt_SA)
	print 'Total LL PA: ' + str(ll_patch_cnt_PA)
	print 'Total LL RV: ' + str(ll_patch_cnt_RV)
	print 'Total RL patch: ' + str(rl_patch_cnt)
	print 'Total RL SA: ' + str(rl_patch_cnt_SA)
	print 'Total RL PA: ' + str(rl_patch_cnt_PA)
	print 'Total RL RV: ' + str(rl_patch_cnt_RV)


	call_file.close()
	
	rule_file.close()

	unlogged_func_file.close()

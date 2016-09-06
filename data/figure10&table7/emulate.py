import os, sys, string, random

def calc_metric(TP, FP, TN, FN):
	if TP == 0 or FP == 0 or TN == 0 or FN == 0:
		return 0, 0, 0, 0
	P = TP / (TP + FP)
	R = TP / (TP + FN)
	F = 2 * P * R / (P + R)
	B = (TP/(TP+FN) + TN/(TN+FP)) /2
	return P, R, F, B


#check argument
if len(sys.argv) <= 2:
	exit(1)

print 'Command:',sys.argv[0],sys.argv[1],sys.argv[2]

#read behaviors
behavior_file = open(sys.argv[1],"r")
behavior_lines = behavior_file.readlines()
behavior_file.close()

#read errlog result
errlog_file = open(sys.argv[2],"r")
errlog_lines = errlog_file.readlines()
errlog_file.close()

#filter legal behaviors
behavior = []
for behavior_line in behavior_lines:
	behavior_line = behavior_line.strip()
	split_line = behavior_line.split('`')
	if len(split_line) != 11:
		continue;
	#if split_line[4] == 'RV':
	behavior.append(split_line)



for i in range(1):

	print 'Run',i+1,':'

	###random split database###
	num = len(behavior)
	train_index = random.sample(range(num), num/2)
	total_index = range(num)
	test_index = list(set(train_index) ^ set(total_index))

	###get the positive cases and nagetive cases
	test_yes_loc = set()
	test_no_loc = set()
	for index in test_index:
		line = behavior[index]
		logloc = line[10]
		if line[9] == 'LL':
			test_yes_loc.add(logloc)
	for index in test_index:
		line = behavior[index]
		logloc = line[10]
		if logloc not in test_yes_loc:
			test_no_loc.add(logloc)
	#test_no_loc = random.sample(test_no_loc, len(test_yes_loc))
	print 'Number of positive cases',len(test_yes_loc),'\tNumber of nagetive cases',len(test_no_loc)
	print 'Method\t', 'TP\tTN\tFP\tFN\tP\tR\tF\tB'

	###do random method###
	TP = TN = FP = FN = R = P = B = F = 0.0
	#R for recall, P for precision, F for f-score, B for balance accuracy
	for loc in test_yes_loc:
		choose = random.randint(0, 1)
		if choose == 1:
			TP += 1
		if choose == 0:
			FN += 1
	for loc in test_no_loc:
		choose = random.randint(0, 1)
		if choose == 1:
			FP += 1
		if choose == 0:
			TN += 1
	P, R, F, B = calc_metric(TP, FP, TN, FN)
	print 'Random\t', TP, '\t', TN, '\t', FP, '\t', FN, '\t', "%.2f"%P, '\t', "%.2f"%R, '\t', "%.2f"%F, '\t', "%.2f"%B

	###do errlog method###
	TP = TN = FP = FN = R = P = B = F = 0.0
	#R for recall, P for precision, F for f-score, B for balance accuracy
	for yes in test_yes_loc:
		choose = 0
		for errlog in errlog_lines:
			if yes in errlog:
				choose = 1
				break;
		if choose == 1:
			TP += 1
		else:
			FN += 1
	for no in test_no_loc:
		choose = 0
		for errlog in errlog_lines:
			if no in errlog:
				#print errlog
				choose = 1
				break;
		if choose == 1:
			FP += 1
		if choose == 0:
			TN += 1
	P, R, F, B = calc_metric(TP, FP, TN, FN)
	print 'Errlog\t', TP, '\t', TN, '\t', FP, '\t', FN, '\t', "%.2f"%P, '\t', "%.2f"%R, '\t', "%.2f"%F, '\t', "%.2f"%B


	###do logadvisor method###
	TP = TN = FP = FN = R = P = B = F = 0.0
	#R for recall, P for precision, F for f-score, B for balance accuracy
	need_log = set()
	for index in train_index:
		line = behavior[index]
		if line[9] != 'LL' and line[9] != 'OS':
			continue
		foo = line[0]
		context = line[5]
		need_log.add(foo+context)

	add_log = set()
	for index in test_index:
			line = behavior[index]
			foo = line[0]
			context = line[5]
			logloc = line[10]
			if foo+context in need_log:
				add_log.add(logloc)

	for yes in test_yes_loc:
		if yes in add_log:
			TP += 1
		else:
			FN += 1
	for no in test_no_loc:
		if no in add_log:
			FP += 1
		else:
			TN += 1
	P, R, F, B = calc_metric(TP, FP, TN, FN)
	print 'LogAd\t', TP, '\t', TN, '\t', FP, '\t', FN, '\t', "%.2f"%P, '\t', "%.2f"%R, '\t', "%.2f"%F, '\t', "%.2f"%B


	###do smartlog method###
	#choose threshold
	for j in range(20):
		TP = TN = FP = FN = R = P = B = F = 0.0
		#R for recall, P for precision, F for f-score, B for balance accuracy
		support_f = support_fc = support_fl = support_fcl = {}
		for index in train_index:
			line = behavior[index]
			foo = line[0]
			context = line[6]
			log = line[9]
			if support_f.has_key(foo):
				support_f[foo] += 1
			else:
				support_f[foo] = 1.0
			if support_fc.has_key(foo+context):
				support_fc[foo+context] += 1
			else:
				support_fc[foo+context] = 1.0
			if support_fl.has_key(foo+log):
				support_fl[foo+log] += 1
			else:
				support_fl[foo+log] = 1.0
			if support_fcl.has_key(foo+context+log):
				support_fcl[foo+context+log] += 1
			else:
				support_fcl[foo+context+log] = 1.0

		add_log = set()
		for index in test_index:
			line = behavior[index]
			foo = line[0]
			context = line[6]
			log = line[9]
			logloc = line[10]

			if not support_fcl.has_key(foo+context+'LL'):
				continue

			support = support_fcl[foo+context+'LL']
			confidence = support_fcl[foo+context+'LL'] / support_fc[foo+context]
			correlation = (support_fcl[foo+context+'LL'] * support_f[foo]) / (support_fc[foo+context] * support_fl[foo+'LL'])

			if confidence > 0.05*j and correlation >= 1:
				add_log.add(logloc)


		for yes in test_yes_loc:
			if yes in add_log:
				TP += 1
			else:
				#print 'FN',yes
				FN += 1
		for no in test_no_loc:
			if no in add_log:
				#print 'FP',no
				FP += 1
			else:
				TN += 1
		P, R, F, B = calc_metric(TP, FP, TN, FN)
		print 0.05*j, '\t', TP, '\t', TN, '\t', FP, '\t', FN, '\t', "%.2f"%P, '\t', "%.2f"%R, '\t', "%.2f"%F, '\t', "%.2f"%B


	print
print
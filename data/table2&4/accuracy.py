import os,sys,re,string,random,linecache

def get_recall_semantic():

	semantic_file = open("recall.semantic.input","r")
	semantic_lines = semantic_file.readlines()
	semantic_file.close()

	cnt = 0
	tot = len(semantic_lines)
	cur_list = []
	ret = []

	#print "Semantic recall"
	for line in semantic_lines:
		line = line.strip()
		cur_list.append(line)
		cnt += 1
		if cnt == 100:
			ave = 0.0
			for i in range(10):
				random_index = random.sample(range(100), 50)

				train_set = []
				test_set = []
				for j in range(100):
					if j in random_index:
						train_set.append(cur_list[j])
					else:
						test_set.append(cur_list[j])

				count = 0
				for test in test_set:
					if test in train_set:
						count += 1
				ave += count
			cur_list = []
			cnt -= 100
			ret.append(ave/10)
	return ret


def get_recall_feature():

	syntax_file = open("recall.syntax.input","r")
	syntax_lines = syntax_file.readlines()
	syntax_file.close()
	
	cnt = 0
	tot = len(syntax_lines)
	cur_list = []
	ret = []

	#print "Syntax recall"
	for line in syntax_lines:
		line = line.strip()
		cur_list.append(line)

		cnt += 1
		if cnt == 100:

			ave = 0.0
			for i in range(10):
				random_index = random.sample(range(100), 50)

				train_set = []
				test_set = []
				for j in range(100):
					if j in random_index:
						train_set.append(cur_list[j])
					else:
						test_set.append(cur_list[j])

				count = 0
				for test in test_set:
					if test in train_set:
						count += 1
				ave += count
			cur_list = []
			cnt -= 100
			ret.append(ave/10)
	return ret


def get_recall_pattern():

	errlog_recall_file = open("errlog.recall.input","r")
	errlog_pattern_file = open("errlog.pattern.input","r")

	errlog_recall_lines = errlog_recall_file.readlines()
	errlog_pattern_lines = errlog_pattern_file.readlines()

	errlog_recall_file.close()
	errlog_pattern_file.close()

	cnt = 0
	tot = len(errlog_recall_lines)
	cur_list = []
	ret = []

	#print "Pattern recall"
	for line in errlog_recall_lines:
		line = line.strip()
		cur_list.append(line)
		cnt += 1
		if cnt == 100:
			ave = 0.0
			for i in range(10):
				test_set = random.sample(cur_list, 50)
				count = 0
				for cur in test_set:
					for snippet in errlog_pattern_lines:
						if cur in snippet:
							count += 1
							break
				ave += count
			cur_list = []
			cnt -= 100
			ret.append(ave/10)
	return ret


if __name__ == '__main__':

	#save result
	recall_semantic = []
	recall_feature = []
	recall_pattern = []
	precision_semantic = []
	precision_feature = []
	precision_pattern = []

	#evaluate recall rate

	#errlog recall
	print "Errlog recall"
	recall_pattern = get_recall_pattern()
	for recall in recall_pattern:
		print str(recall)+'/50.0='+str("%.2f"%(float(recall)/50.0))

	#logadvisor recall
	print "LogAdvisor recall"
	recall_feature = get_recall_feature()
	for recall in recall_feature:
		print str(recall)+'/50.0='+str("%.2f"%(float(recall)/50.0))

	#semantic recall
	print "Semantic recall"
	recall_semantic = get_recall_semantic()
	for recall in recall_semantic:
		print str(recall)+'/50.0='+str("%.2f"%(float(recall)/50.0))


	#evaluate precision rate

	#errlog precision
	errlog_pattern_file = open("errlog.pattern.input","r")
	errlog_pattern_lines = errlog_pattern_file.readlines()
	errlog_pattern_file.close()

	errlog_precision_file = open("errlog.precision.input","r")
	errlog_precision_lines = errlog_precision_file.readlines()
	errlog_precision_file.close()

	cnt = 0
	tot = len(errlog_precision_lines)
	cur_list = []

	print "Errlog precision"
	for line in errlog_precision_lines:
		line = line.strip()
		cur_list.append(line)

		cnt += 1
		if cnt == 100:

			ave = 0.0
			tave = 0.0
			for i in range(10):

				test_set = random.sample(cur_list, 50)


				count = 0
				tcount = 0
				for cur in test_set:
					for snippet in errlog_pattern_lines:
						myfuncloc = cur.split('\t')[1]
						
						mytype = cur.split('\t')[2]
						mylogloc = cur.split('\t')[3]
						if mylogloc in snippet:
							count += 1
							#print cur
							if mytype == "LL":
								tcount += 1
							break
				ave += count
				tave += tcount
			print str(tave/10)+'/'+str(ave/10)+'='+str("%.2f"%(tave/ave))


			cur_list = []
			cnt -= 100


	#logadvisor precision
	precision_file = open("precision.input","r")
	precision_lines = precision_file.readlines()
	precision_file.close()

	cnt = 0
	tot = len(precision_lines)
	cur_list = []

	print "LogAdvisor precision"
	for line in precision_lines:
		line = line.strip()
		cur_list.append(line)

		cnt += 1
		if cnt == 100:

			tave = 0.0
			ave = 0.0
			for i in range(10):
				random_index = random.sample(range(100), 50)

				train_set = []
				test_set = []
				for j in range(100):
					if j in random_index:
						train_set.append(cur_list[j])
					else:
						test_set.append(cur_list[j])

				count = 0
				tcount = 0
				for test in test_set:
					foo = test.split('\t')[0]
					log = test.split('\t')[1]
					fooLL = foo + '\tLL'
					fooOS = foo + '\tOS'
					if fooLL in train_set or fooOS in train_set:
						count += 1
						if log == 'LL':
							tcount += 1


				ave += count
				tave += tcount
			print str(tave/10)+'/'+str(ave/10)+'='+str("%.2f"%(tave/ave))

			cur_list = []
			cnt -= 100


	#semanic precision
	cnt = 0
	tot = len(precision_lines)
	cur_list = []

	print "Semantic precision"
	for line in precision_lines:
		line = line.strip()
		cur_list.append(line)

		cnt += 1
		if cnt == 100:

			tave = 0.0
			ave = 0.0
			for i in range(10):
				random_index = random.sample(range(100), 50)

				train_set = []
				test_set = []
				for j in range(100):
					if j in random_index:
						train_set.append(cur_list[j])
					else:
						test_set.append(cur_list[j])

				count = 0
				tcount = 0
				for test in test_set:
					foo = test.split('\t')[0]
					log = test.split('\t')[1]
					fooLL = foo + '\tLL'
					fooOS = foo + '\tOS'
					if fooLL in train_set:
						count += 1
						if log == 'LL':
							tcount += 1


				ave += count
				tave += tcount
			print str(tave/10)+'/'+str(ave/10)+'='+str("%.2f"%(tave/ave))

			cur_list = []
			cnt -= 100


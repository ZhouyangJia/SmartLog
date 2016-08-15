import os,segment,string

def get_words(my_name):
	ret = []
	names_list = my_name.split('_')
	for name in names_list:
		if len(name) > 0:
			seg_list = segment.segment(name)
			for seg in seg_list:
				ret.append(seg)
	return ret



source = open("weight_total_log_times.in","r")
target = open("weight_final_log_times.out","w")

lines = source.readlines()

all_project = []
my_project = []

for line in lines:
	line = line.strip()
	if "Analyze " in line:
		if len(my_project) > 0:
			all_project.append(my_project)
			my_project = []
	else:
		my_project.append(line)
all_project.append(my_project)

for i in range(len(all_project)):

	call_dict = {}
	log_dict = {}

	word_count_dict = {}

	for j in range(len(all_project)):
		if i == j:
			continue

		my_word_count_dict = {}
		for line in all_project[j]:
			log = line.split(',')
			words = get_words(log[0])
			for seg_name in words:
				if call_dict.has_key(seg_name):
					call_dict[seg_name] = int(log[1]) + int(call_dict[seg_name])
				else:
					call_dict[seg_name] = int(log[1])

			for seg_name in words:
				if log_dict.has_key(seg_name):
					log_dict[seg_name] = int(log[2]) + int(log_dict[seg_name])
				else:
					log_dict[seg_name] = int(log[2])	

			for seg_name in words:
				my_word_count_dict[seg_name] = 1

		for k, v in my_word_count_dict.items():
			if k in word_count_dict.keys():
				word_count_dict[k] += 1
			else:
				word_count_dict[k] = 1


	for line in all_project[i]:
		log = line.split(',')
		words = get_words(log[0])
		my_max_rate = -1.0
		my_max_word = ""
		my_rate = 0.0
		for word in words:
			if log_dict.has_key(word) and call_dict.has_key(word) and word_count_dict.has_key(word):
				my_rate = float(log_dict[word])/float(call_dict[word])
				my_rate *= word_count_dict[word]
				if my_rate > my_max_rate:
					my_max_rate = my_rate
					my_max_word = word

			if not word_count_dict.has_key(word):
				word_count_dict[word] = 0

		word_count_dict[""] = 0


		target.write(str(i))
		target.write(",")
		target.write(line)
		target.write(",")
		target.write(str('%.4f'%my_max_rate))
		target.write(",")
		target.write(my_max_word)
		target.write("\n")



source.close()
target.close()
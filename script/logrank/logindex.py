import os,string
import matplotlib.pyplot as plt
import numpy as np

source = open("logindex_final_log_times.in","r")
target = open("log_index.out","w")

project = -1

lines = source.readlines()
log_times = []

rate_index = []
level_index = []
weight_index = []


def calculate(log_times,project):

	lenth = len(log_times)
	if lenth == 0:
		return

	print lenth

	tmp = []

	#sort by rate
	log_times.sort(lambda x,y:cmp(x[2],y[2]), reverse=True) 
	for i in range(9):
		index = int(float(lenth)*0.1*(i+1))
		sel_log = log_times[0:index]
		total = 0.0
		for item in sel_log:
			total += item[1]
		total /= index
		tmp.append(total)
		target.write(str('%.4f'%total))
		target.write(',')
	target.write('\n')

	rate_index.append(tmp)
	tmp = []

	#sort by level
	log_times.sort(lambda x,y:cmp(x[3],y[3]), reverse=True) 
	for i in range(9):
		index = int(float(lenth)*0.1*(i+1))
		sel_log = log_times[0:index]
		total = 0.0
		for item in sel_log:
			total += item[1]
		total /= index
		tmp.append(total)
		target.write(str('%.4f'%total))
		target.write(',')
	target.write('\n')

	level_index.append(tmp)
	tmp = []

	#sort by weight
	log_times.sort(lambda x,y:cmp(x[4],y[4]), reverse=True) 
	for i in range(9):
		index = int(float(lenth)*0.1*(i+1))
		sel_log = log_times[0:index]
		total = 0.0
		for item in sel_log:
			total += item[1]
		total /= index
		tmp.append(total)
		target.write(str('%.4f'%total))
		target.write(',')
	target.write('\n')

	weight_index.append(tmp)


	return

for line in lines:
	line.strip()
	if len(line) == 0:
		continue
	seg = line.split(',')
	if seg[0] != project:
		project = seg[0]
		calculate(log_times,project)
		log_times = []
	diff = int(seg[2]) - int(seg[3])
	rate = float(int(seg[3])/int(seg[2]))
	if int(seg[3]) == 0.0:
		level = 0
	else:
		level = float(int(seg[4])/int(seg[3]))
	weight = float(seg[5])
	truble = (seg[1], diff, rate, level, weight)
	log_times.append(truble)
calculate(log_times,str(int(project)+1))

string = ['Httpd', 'Subversion', 'MySQL', 'PostgreSQL', 'GIMP', 'Wireshark']
marker = ['-v', '-o', '-p', '-h', '-D', '-s']

plt.figure(figsize=(8,4))
x = range(1,10)
for i in range(6):
	y = weight_index[i]
	plt.plot(x,y,marker[i],label=string[i])
plt.xlabel('Threshold')
plt.ylabel('Index')
#plt.ylim(0, 120)
plt.legend()
plt.show()

def no():
	plt.figure(figsize=(8,4))
	x = range(1,11)
	for i in range(6):
		y = weight_index[i]
		plt.plot(x,y,label=string[i])
	plt.xlabel('Threshold')
	plt.ylabel('Index')
	#plt.ylim(0, 120)
	plt.legend()
	plt.show()


	plt.figure(figsize=(8,4))
	x = range(1,11)
	for i in range(6):
		y = weight_index[i]
		plt.plot(x,y,label=string[i])
	plt.xlabel('Threshold')
	plt.ylabel('Index')
	plt.ylim(0, 120)
	plt.legend()
	plt.show()

source.close()
target.close()
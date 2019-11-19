#!/usr/bin/env python

import random

if __name__ == "__main__":

	
	input_ = input("Enter Number of nodes: ")
	count = input("Number of edges: ")
		
	data = [[-1 for i in xrange(input_)] for i in xrange(input_)]

	for i in range(len(data)):
		data[i][i] = 0

	while count > 0:
		a = random.randint(0, input_ - 1)
		b = random.randint(0, input_ - 1)

		if a == b:
			continue

		data[a][b] = 1
		data[b][a] = 1
		count -= 1

	print input_
	for row in data:
		str_ = ''
		for info in row:
			 if info >= 0:
				str_ += ' ' + str(info) + ' '
			 else:
				str_ += str(info) + ' '

		print str_[:-1]

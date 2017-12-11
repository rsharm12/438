#!/usr/bin/python

import sys
import random
from pprint import pprint

import matplotlib.pyplot as plt

def getInput(filename):
	inputs = {}
	with open(filename) as f:
		for line in f:
			inp = line.split(' ')
			if len(inp) == 2:
				inputs[inp[0]] = int(inp[1])
			else:
				inputs[inp[0]] = map(int, inp[1:])
	return inputs


def variance(numbers):
	var = 0.0
	N = float(len(numbers))
	mu = sum(numbers)/N
	for num in numbers:
		var += pow(num - mu, 2)
	return var/N


def csma(params):
	channelBusy = False
	nodes = []
	transmissionTime = 0


	out_channelUtil = 0
	out_collisions = 0
	out_successful = 0


	for nodeid in range(params["N"]):
		node = {
			"counter": random.randint(0, params["R"][0]),
			"backoffIdx": 0,
			"collisions": 0,
			"successes": 0
		}
		nodes.append(node)


	for tick in range(params["T"]):
		# print("\ntick=%d" % tick)
		if channelBusy:
			out_channelUtil += 1
			# print("Channel Busy")
			transmissionTime += 1
			if transmissionTime == params["L"]:
				transmissionTime = 0
				channelBusy = False
			continue

		# pprint(nodes)
		collisionsIdx = []

		for nodeidx in range(params["N"]):
			#nodes[nodeidx]["counter"] -= 1
			if nodes[nodeidx]["counter"] == 0:
				collisionsIdx.append(nodeidx)

		# print("Collisions: {}".format(collisionsIdx))

		if len(collisionsIdx) == 0:
			# no one wants to transmit in this iteration
			for nodeidx in range(params["N"]):
				nodes[nodeidx]["counter"] -= 1

		elif len(collisionsIdx) == 1:
			# successful transmission possible
			channelBusy = True
			nodes[collisionsIdx[0]]["backoffIdx"] = 0
			nodes[collisionsIdx[0]]["counter"] = random.randint(0, params["R"][0])
			nodes[collisionsIdx[0]]["successes"] += 1

		else:
			# collisions happening
			out_collisions += 1
			for nodeidx in collisionsIdx:
				nodes[nodeidx]["collisions"] += 1
				nodes[nodeidx]["backoffIdx"] += 1
				if nodes[nodeidx]["backoffIdx"] == params["M"] - 1:
					nodes[nodeidx]["backoffIdx"] = 0
					nodes[nodeidx]["counter"] = random.randint(0, params["R"][0])
				else:
					nodes[nodeidx]["counter"] = random.randint(0, params["R"][nodes[nodeidx]["backoffIdx"]])

			remNodes = set(range(params["N"])) - set(collisionsIdx)
			for nodeidx in remNodes:
				nodes[nodeidx]["counter"] -= 1


	out_channelUtil = float(out_channelUtil)

	return (out_channelUtil * 100 / params["T"])

	############ submit later
	with open("output.txt", "w") as f:
		f.write("%f\n" % (out_channelUtil * 100 / params["T"])) # channel utilization
		f.write("%f\n" % ((params["T"] - out_collisions - out_channelUtil) * 100 / params["T"])) # channel idle fraction
		f.write("%d\n" % out_collisions) # total number of collisions
		f.write("%f\n" % variance([node["successes"] for node in nodes])) # variance in successful transmissions
		f.write("%f\n" % variance([node["collisions"] for node in nodes])) # variance in collisions

	pprint(nodes)


# params = getInput(sys.argv[1])

params = {
	"N": 25,
	"L": 20,
	"R": [8, 16, 32, 64, 128],
	"M": 6,
	"T": 50000
}

RS = [
	([1, 2, 4, 8, 16], 'b'),
	([2, 4, 8, 16, 32], 'g'),
	([4, 8, 16, 32, 64], 'r'),
	([8, 16, 32, 64, 128], 'm'),
	([16, 32, 64, 128, 256], 'y'),
]

for R, color in RS:
	params["R"] = R

	x = range(5,101)
	y = []
	for n in x:
		params["N"] = n
		y.append(csma(params))
		print("N=%d" % n)

	plt.plot(x, y, color, label='R={}'.format(R))
	print("done plotting with R={}".format(R))

plt.legend(loc='lower left', fontsize='x-small', fancybox=True, shadow=True)
plt.xlabel("number of nodes")
plt.ylabel("channel utilization (%)")
plt.title("Channel utilization with varying backoffs")
plt.savefig("partD.png", dpi=300)

# ls = [
# 	(20, 'b'),
# 	(40, 'g'),
# 	(60, 'r'),
# 	(80, 'm'),
# 	(100, 'y'),
# ]

# for L, color in ls:
# 	params["L"] = L
# 	x = range(5,101)
# 	y = []
# 	for n in x:
# 		params["N"] = n
# 		y.append(csma(params))
# 		print("N=%d" % n)

# 	plt.plot(x, y, color, label='L={}'.format(L))

# plt.legend(loc='lower left', fancybox=True, shadow=True)
# plt.xlabel("number of nodes")
# plt.ylabel("channel utilization (%)")
# plt.title("Channel utilization with varying packet lengths")
# plt.savefig("partE.png", dpi=300)


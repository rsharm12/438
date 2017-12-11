#!/usr/bin/python

import sys
import random
# from pprint import pprint

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

	# write to output.txt
	with open("output.txt", "w") as f:
		f.write("%f\n" % (out_channelUtil * 100 / params["T"])) # channel utilization
		f.write("%f\n" % ((params["T"] - out_collisions - out_channelUtil) * 100 / params["T"])) # channel idle fraction
		f.write("%d\n" % out_collisions) # total number of collisions
		f.write("%f\n" % variance([node["successes"] for node in nodes])) # variance in successful transmissions
		f.write("%f\n" % variance([node["collisions"] for node in nodes])) # variance in collisions

	# pprint(nodes)

params = getInput(sys.argv[1])
csma(params)

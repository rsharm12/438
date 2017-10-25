#!/bin/bash

sudo tc qdisc del dev enp0s8 root 2> /dev/null
sudo tc qdisc add dev enp0s8 root handle 1:0 netem delay 20ms loss 5%
sudo tc qdisc add dev enp0s8 parent 1:1 handle 10:  tbf rate 40Mbit burst 10mb latency 1ms
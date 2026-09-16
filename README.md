# Proof-of-concept exploits for CheriBSD and Morello Linux

This repository contains the artifact for the following paper (published at ESORICS 2026):

> Dariy Guzairov, Alex Potanin, Stephen Kell, Alwen Tiu. A security analysis of CheriBSD and Morello Linux. ESORICS 2026.

A pre-print of the paper is available on [arxiv.](https://arxiv.org/abs/2601.19074)

The original source code in this artifact was created by Dariy Guzairov. The SSL key leak attack ([sslExample/](./sslExample/) and [sslExample_nosyscall](./sslExample_nosyscall/)) was slightly modified by me (with the help of an AI agent) to make the stack scanning more robust and efficient. (The original artifact by Guzairov was created manually without using any AI tools). 

These PoCs were tested on CheriBSD version releng/25.03-2339ee891fe2 (running baremetal and QEMU), and Morello Linux arm-084782.gpu 6.7.0-g96c45a786534 (running baremetal only).


# Repository structure

Each folder contains examples for each operating system and attack. 

The cheriBSD folder contains all of the attacks and build instructions to run the four attacks on cheriBSD.

The linuxMorello folder contains the code and build files for the attacks discussed.

Finally the sslExample folder contains the proof of concept code that generates a private key, and the malicious library that retrieves it.


Each attack follows a similar structure:

- test.c -> the trusted code with a main entry point
- library.c -> the malicious library 
- stack_scan.c -> optional helper library  	
 

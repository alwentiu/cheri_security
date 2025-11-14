# Repository structure

Each folder contains examples for each operating system and attack. 

The cheriBSD folder contains all of the attacks and build instructions to run the four attacks on cheriBSD.

The linuxMorello folder contains the code and build files for the attacks discussed.

Finally the sslExample folder contains the proof of concept code that generates a private key, and the malicious library that retrieves it.


Each attack follows a similar structure:
test.c -> the trusted code with a main entry point
library.c -> the malicious library 
stack_scan.c -> optional helper library  	
 

# proof of concept

This proof of concept has the malicious library retrieve the private key from the binary.
This example only works on CheriBSD, and can be compiled by using `make test`

The same c18n/heap revocation commands can be run to enable/disable them 

proccontrol -m <cherirevoke or c18n> -s <enable or disable> ./bin/test 

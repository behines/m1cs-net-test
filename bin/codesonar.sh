#!/bin/bash

# crontab -e
# * * * * * /opt/codesonar-6.1p0/codesonar/bin/cslaunchd https://cae-codesonarhub-uat.jpl.nasa.gov:8443 -hubuser root -launchd-key ""
#
#
# codesonar analyze m1cs cae-codesonarhub-uat.jpl.nasa.gov:8443 make 
# codesonar analyze m1cs -clean -hubuser gbrack -hubcert cert.pem -hubkey key.pem cae-codesonarhub-uat.jpl.nasa.gov:8443 make
#

SERVER="cae-codesonarhub-uat.jpl.nasa.gov:8443"
UAT_KEYS="-auth certificate -hubuser gbrack -hubcert cert.pem -hubkey key.pem"
CONFIG="-conf-file m1cs.conf -conf-file jpl.conf -conf-file pow10.conf"

if [ -z $1 ]; then
    codesonar analyze m1cs $CONFIG $UAT_KEYS $SERVER make --dir ..
else if [ $1 = "clean" ]; then
    make --dir ..  clean
    codesonar analyze m1cs -clean $CONFIG $UAT_KEYS $SERVER make --dir ..
else
    echo "usage: $0 [clean]"  
   # codesonar analyze m1cs -auth certificate -hubuser gbrack -hubcert cert.pem -hubkey key.pem cae-codesonarhub-uat.jpl.nasa.gov:8443 make --dir ..
   fi
fi

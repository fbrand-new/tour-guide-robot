 #!/bin/bash

if [[ -n $1 ]]; then
    if [[ $1 == "-d"  || $1 == "--devel" ]]; then
        sudo docker build -t elandini84/r1images:r1OpenPose_devel .
    elif [[ $1 == "-s"  || $1 == "--stable" ]]; then
        sudo docker build -t elandini84/r1images:r1OpenPose_stable .
    elif [[ $1 == "-h"  || $1 == "--help" ]]; then
        echo "Syntax: ./build-docker.sh [option]"
        echo "options:"
        echo "    -d, --devel     Build the image with the r1OpenPose_devel tag"
        echo "    -s, --stable    Build the image with the r1OpenPose_stable tag"
        echo "    -h, --help      See current help"
        echo "If no option is passed, the r1Core2_devel tag is used"
    else
        echo "Wrong option."
        $0 -h
    fi
else
	echo "No option passed. Using the default tag"
    $0 -d
fi

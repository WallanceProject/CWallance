#!/bin/bash

# Launch CWallance Node (Raspberry)
# Command: ./Run_Node.sh

# Init IP Address
MyAddr=$(hostname -I | cut -d ' ' -f1)

# Remove Previous Consensus
rm -f Consensus/*.db

# Display Node's ID
echo "--------------------------------"
echo "YOUR NODE IS $(echo $MyAddr | cut -d '.' -f4) ($MyAddr)"
echo "--------------------------------"

# Launch Node
cd Node
./Node.bin $(echo $MyAddr | cut -d '.' -f4)
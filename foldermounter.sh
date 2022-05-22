#!/bin/bash
sudo mkdir $1
sudo chown -R $2 $1
sudo mount -t cifs -o username=guest,password=,uid=1000,iocharset=utf8 //$3/$4 $1

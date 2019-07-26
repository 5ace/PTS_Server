#!/usr/bin/python
# -*- coding: utf-8 -*-
from os import listdir
from os.path import isfile, isdir, join
import subprocess
import os
mypath = "/PTS_server/data"

files = listdir(mypath)

for f in files:
    fullpath = join(mypath, f)
    if isdir(fullpath):
        list_path = join(fullpath,"list")
        fo = open(list_path,"w")
        images = listdir(fullpath)
        for i in images:
            if ".jpg" in i or ".JPG" in i:
                #image_path = join(fullpath, i)
                fo.write(i+"\n")
        fo.close()
        mom_path = os.getcwd()
        os.chdir(fullpath)
        subprocess.call(["extract", "list", "6", fullpath, fullpath])
        subprocess.call(["makeIndex", "list", "Index", "6", fullpath, fullpath])
        os.chdir(mom_path)


#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import hashlib, urllib, base64
import datetime


## add current time to dat
## encode as utf-8, hash with sha1, pick up [1:25] character
## decode as ascii, and return
def hash(dat):
    dat=str(dat)+str(datetime.datetime.now())
    dat=dat.encode('utf-8')
    dat=base64.urlsafe_b64encode(hashlib.sha1(dat).digest())[1:25]
    dat=dat.decode('ascii')
    return dat



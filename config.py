
#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os

DEBUG=False
PORT=1025
HOST=f'140.113.194.39:{PORT}'
PROTOCOL='https'
URL=f'{PROTOCOL}://{HOST}'
PREFIX=os.path.dirname(os.path.realpath(__file__))
SSL={
    'certfile': os.path.join(PREFIX, 'ssl', 'server.crt'),
    'keyfile': os.path.join(PREFIX, 'ssl', 'server.key.unsecure')
}

GOOGLE=True

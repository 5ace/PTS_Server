
#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import tornado.web
import tornado.httpserver
from tornado.ioloop import IOLoop
import tornado.platform.asyncio

import asyncio
import os, shutil
import time

import config
import handler

class Application(tornado.web.Application):
    def __init__(self):

        ## Delete config.PREFIX/files folder and recreate it, permission is 700
        if os.path.isdir(os.path.join(config.PREFIX, 'files')):
            shutil.rmtree(os.path.join(config.PREFIX, 'files'))
        os.mkdir(os.path.join(config.PREFIX, 'files'), 0o700)

        ## Delete /tmp/pts folder, and recreate it, permission is 700
        ## /tmp/pts is store the images upload by users
        if os.path.isdir(os.path.join('/tmp', 'pts')):
            shutil.rmtree(os.path.join('/tmp', 'pts'))
        os.mkdir(os.path.join('/tmp', 'pts'), 0o700)

        ## set domain and handler relation
        handlers=[
            (r'/', handler.IndexHandler),
            (r'/file', handler.File),
            (r'/General/searchByInput', handler.GeneralInput),
            (r'/Advance/searchByImages', handler.AdvanceInput),
            (r'/(General|Advance)/getSearchResults', handler.Results),
        ]

        ## init Application
        tornado.web.Application.__init__(
            self,
            handlers,
            template_path=os.path.join(config.PREFIX, 'templates'), # set the templates route, could be deleted in the future
            compress_response=True,
            debug=config.DEBUG,
        )

def main():
    #if config.DEBUG:
    #    policy=tornado.platform.asyncio.AnyThreadEventLoopPolicy()
    #    asyncio.set_event_loop_policy(policy)
    #    server=tornado.httpserver.HTTPServer(Application(), ssl_options=config.SSL)
    #    server.bind(config.PORT)
    #    server.start(4)
    while True:
        try:
            ## start server
            tornado.httpserver.HTTPServer(Application(), ssl_options=config.SSL).listen(config.PORT)
            break
        except Exception as E:
            ## if error happened, wait 10 seconds and restart its
            print(E)
            print(f'Waiting for port {config.PORT}')
            time.sleep(10)
    ## start tornado asynchronous
    tornado.ioloop.IOLoop.current().start()

if __name__ == "__main__":
    main()
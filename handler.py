
#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from tornado.ioloop import IOLoop
import tornado.web

import json
import datetime
import urllib.parse
import os, shutil

from response import Response
import util, config

print('loading Google...')
tmp=datetime.datetime.now()
from crawler import Crawler 
tmp=datetime.datetime.now()-tmp
print(tmp.total_seconds())

print('loading openCV...')
tmp=datetime.datetime.now()
from newcv import CV
tmp=datetime.datetime.now()-tmp
print(tmp.total_seconds())

print('loading YOLO...')
tmp=datetime.datetime.now()
from yolo.PTS_model import CNN
tmp=datetime.datetime.now()-tmp
print(tmp.total_seconds())

print('loading Database...')
DATABASE=dict()
tmp=datetime.datetime.now()
tmp=datetime.datetime.now()-tmp
print(tmp.total_seconds())

class MyHandler(tornado.web.RequestHandler):
    def initialize(self):
        while True:
            self.sess=util.hash(self.request)
            if self.sess not in DATABASE:
                break
    def done(self):
        self.write(self.resp.data)
        self.flush()
        self.finish()

class GeneralInput(MyHandler):
    def post(self):
        self.resp=Response(self.sess)
        path=os.path.join('/tmp', 'pts', self.sess)
        os.mkdir(path, 0o700)
        self.req=self.get_argument('query').strip()
        self.upd=[]
        for fp in self.request.files:
            if not fp.startswith('image'):
                continue
            for fp in self.request.files[fp]:
                if fp['content_type']!='image/jpeg':
                    continue
                fn=fp['filename']
                self.upd+=[fn]
                fn=util.hash(fn)+'.jpg'
                open(os.path.join(path, fn), 'wb').write(fp['body'])
        if self.upd==[] and self.req=='':
            self.resp.fail('Empty query.')
        else:
            DATABASE[self.sess]=Response()
            self.resp.success(f'Uploaded files: {self.upd}, Query string: {self.req}')
            IOLoop.current().run_in_executor(None, self.blocking_process)
        self.done()
    def blocking_process(self):
        try:
            src=os.path.join('/tmp', 'pts', self.sess)
            if self.req!='' and config.GOOGLE:
                Crawler.query(src, self.req, 3)
            dst=os.path.join(config.PREFIX, 'files', self.sess)
            os.mkdir(dst)
            cnt=0.0
            tot=len(os.listdir(src))
            for fn in os.listdir(src):
                # CNN
                cnt+=1
                CNN.process(os.path.join(src, fn), dst)
                DATABASE[self.sess].message('{:.2f}'.format(cnt/tot))
            for cls in os.listdir(dst):
                for fn in os.listdir(os.path.join(dst, cls)):
                    DATABASE[self.sess].add(f'{config.URL}/file?session={self.sess}&class={cls}&path={fn}')
            DATABASE[self.sess].success()
        except Exception as E:
            DATABASE[self.sess].fail(E)

class AdvanceInput(MyHandler):
    def post(self):
        self.resp=Response(self.sess)
        sess=self.get_body_argument('session')
        urls=set()
        self.imgs=set()
        for key in self.request.body_arguments:
            if key.startswith('url'):
                for url in self.get_body_arguments(key):
                    try:
                        ses=cls=pth=''
                        tmp=urllib.parse.urlsplit(url)
                        if tmp.scheme!=config.PROTOCOL or tmp.netloc!=config.HOST or tmp.path!='/file':
                            continue
                        for part in tmp.query.split('&'):
                            part=part.split('=')
                            if part[0]=='session':
                                ses=part[1]
                            if part[0]=='class':
                                cls=part[1]
                            if part[0]=='path':
                                pth=part[1]
                        if sess!=ses:
                            continue
                    except Exception as E:
                        print("Exception from handler.py -> Advance :", E)
                        continue
                    path=os.path.join(config.PREFIX, 'files', sess, cls, pth)
                    if os.path.isfile(path):
                        urls|={url}
                        self.imgs|={(path, cls)}
        if self.imgs==set():
            self.resp.fail('No image found.')
        else:
            DATABASE[self.sess]=Response()
            self.resp.success(f'Selected: {urls}')
            IOLoop.current().run_in_executor(None, self.blocking_process)
        self.done()
    def blocking_process(self):
        try:
            # CDVS
            res, facial, face_res = CV.query(self.imgs)

            if facial:
                cnt=0
                tot=len(face_res)
                for tmp in face_res:
                    cnt += 1
                    DATABASE[self.sess].add(tmp)
                    DATABASE[self.sess].message('{0:.2f}'.format(cnt/tot))
            else:
                if res!=[]:
                    mvl=max(_['count'] for _ in res)+1
                cnt=0
                tot=len(res)
                for tmp in res:
                    tmp['confidence'] = tmp['count']/mvl
                    for key in tmp.keys()-{'ID', 'time', 'box', 'confidence', 'name', 'class'}:
                        del tmp[key]
                    cnt += 1
                    DATABASE[self.sess].add(tmp)
                    DATABASE[self.sess].message('{0:.2f}'.format(cnt/tot))
                    
            DATABASE[self.sess].success()
        except Exception as E:
            DATABASE[self.sess].fail(E)

class Results(MyHandler):
    def retrieve(self, *args):
        self.resp=Response()
        try:
            self.sess=self.get_argument('session')
            self.resp=DATABASE[self.sess]
        except:
            self.resp.fail()
            self.resp.message('Session not found.')
        print(json.dumps(self.resp.data, indent=4))
        self.done()
    post=retrieve
    get=retrieve

class File(tornado.web.RequestHandler):
    def retrieve(self):
        try:
            sess=self.get_argument('session')
            path=self.get_argument('path')
            cls=self.get_argument('class')
        except:
            raise tornado.web.HTTPError(400)
        try:
            with open(os.path.join(config.PREFIX, 'files', sess, cls, path), 'rb') as fp:
                self.write(fp.read())
                self.set_header('Content-Type', 'image/jpeg')
                self.flush()
                self.finish()
        except:
            raise tornado.web.HTTPError(404)
    post=retrieve
    get=retrieve

class IndexHandler(tornado.web.RequestHandler):
    def get(self):
        self.render("test_form.html")



#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from googleapiclient.discovery import build
import tornado.httpclient
import os, shutil
import time
import json

import util

class Crawler:
    INTERVAL=3
    RETRY=3
    #KEY='AIzaSyCorwjTecn6q4wdS9hadHKIAL2WOiMnZuA'
    #CX='007017952105548790428:woaehbvetgm'
    KEY='AIzaSyBAEs9Tv4IgzOmpNwOpo1AIELOODIl-H28'
    CX='011899685858991726325:6ykhb0bjytc'
    
    def __init__(self):
        self.service=build(
            'customsearch',
            'v1',
            developerKey=Crawler.KEY,
            cache_discovery=False,
        )
        self.client=tornado.httpclient.HTTPClient()


    def search(self, query, start=1):
        count=Crawler.RETRY
        while True:
            try:
                res=self.service.cse().list(
                    start=start,
                    q=query,
                    exactTerms=query,
                    cx=Crawler.CX,
                    filter='1',
                    safe='off',
                    gl='tw',
                    searchType='image',
                    imgType='photo',
                    imgSize='large',
                    imgColorType='color',
                    fileType='jpg',
                ).execute()
                if 'items' in res:
                    return res
            except Exception as E:
                print(E)
            count-=1
            if count<=0:
                raise Exception(f'Retry limit exceeded for google custom search: {query}')
            time.sleep(Crawler.INTERVAL)

    def fetch(self, path, url):
        if not url.startswith('https'):
            raise Exception(f'https required: {url}')
        count=Crawler.RETRY
        while True:
            try:
                res=self.client.fetch(url)
                name=os.path.join(path, util.hash(url)+'.jpg')
                if os.path.exists(name):
                    raise Exception(f'File already exists: {name}')
                with open(name, 'wb') as fp:
                    fp.write(res.body)
                return name
            except Exception as E:
                print(E)
                time.sleep(20)
            count-=1
            if count<=0:
                raise Exception(f'Retry limit exceeded for tornado http client: {url}')
            time.sleep(Crawler.INTERVAL)
    
    def query(self, path, query, count=0):
        res=[]
        #for cache in self.data:
        #    if cache in query:
        #        for name in os.listdir(os.path.join(Crawler.PREFIX, self.data[cache])):
        #            name=os.path.join(Crawler.PREFIX, self.data[cache], name)
        #            dest=os.path.join(path, util.hash(name)+'.jpg')
        #            shutil.copyfile(name, dest)
        #            res+=[dest]
        #        return res
        start=1
        #count=-1 #######
        while len(res)<count:
            try:
                urls=self.search(query, start)
                start+=10
            except Exception as E:
                print(E)
                break
            for url in urls['items']:
                try:
                    name=self.fetch(path, url['link'])
                    res+=[name]
                except Exception as E:
                    print(E)
                    continue
                if len(res)>=count:
                    break
        return res

Crawler=Crawler()



import json
import random

class Response:

    def __init__(self, sess=None):
        ## Response format
        ## {
        ##     'result':[],
        ##     'session':sess, # if sess is not None
        ##     'status':'processing', # initial status is 'processing'
        ##     'message':''
        ## }
        self.data={'results': list()} if sess==None else {'session': sess}
        self.data['status']='processing'
        self.data['message']=''


    ## if process is fail
    ## clear the session, add the error message to data['message']
    ## and set data['status']='fail'
    def fail(self, msg=None):
        print(msg)
        if 'session' in self.data:
            self.data['session']=''
        self.message(msg)
        self.data['status']='fail'


    ## if process is success
    ## sort the result decreasing in confidence, pick up top 20 results
    ## message will be the length of results if there's no error
    ## omit any error in this part
    ## set data['status']='success'
    def success(self, msg=None):
        self.message(msg)
        try:
            random.shuffle(self.data['results']) ## why shuffle here QQ?
            self.data['results'].sort(key=lambda _: -_['confidence'])
        except:
            pass
        try:
            self.data['results']=self.data['results'][:20]
            self.message(len(self.data['results']))
        except:
            pass    
        self.data['status']='success'

    ## is msg is json, parse it and cover to data['message']
    ## if msg is string, parse it and cover to data['message']
    ## if msg is None, set data['message'] to empty
    def message(self, msg=None):
        try:
            json.dumps(msg)
            self.data['message']=msg
        except:
            self.data['message']=str(msg)
        if msg==None:
            self.data['message']=''

    ## add x to data['result']
    def add(self, x):
        self.data['results']+=[x]


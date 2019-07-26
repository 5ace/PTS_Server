
import os
import json
import datetime
from face_recognition.PTS_face_align import Face_Align
from face_recognition.PTS_face_compare import Face_Compare
import subprocess

class CV:
    PREFIX=os.path.dirname(os.path.realpath(__file__))
    
    def __init__(self):
        self.data=dict()
        with open(os.path.join(CV.PREFIX, 'data', 'data.json')) as fp:
            for record in json.load(fp):
                ID=record['ID']
                box=record['box']
                cls=record['class']
                name=record['name']
                time=record['time']
                file_name = record['file_name']
                time=datetime.datetime.strptime(time, '%H%M%S').time()
                secs=time.hour*3600+time.minute*60+time.second-3600
                time=time.strftime('%H:%M:%S:00')
                # path=os.path.join(CV.PREFIX, 'data', cls, f'{cls}_{name}_{secs}.jpg')
                path=os.path.join(CV.PREFIX, 'data', cls, file_name)
                #desc_path=os.path.join(CV.PREFIX, 'data', cls, f'{cls}_{name}_{secs}.16384.cdvs')
                if os.path.isfile(path): ###
                    try:
                        #desc=desc_path
                        if cls not in self.data:
                            self.data[cls]=list()
                            #print("******cls not in self.data*****")
                        self.data[cls].append({
                            'ID': ID,
                            'box': box,
                            'name': name,
                            'time': time,
                            'path': path,
                            #'desc': desc,
                            'count':0,
                            'score':0.0
                        })
                    except Exception as E:
                        print(E)
    def query(self, imgs):
        cls_cnt = {}
        for path, cls in imgs:
            try:
                
                head, tail = os.path.split(path)
                fp_list =open(os.path.join(head, "list"),"a")
                fp_query=open(os.path.join(head,"query"),"a")
                fp_list.write(tail+"\n") #build list
                fp_query.write(tail+"  .jpg\n") #build query
                fp_list.close()
                fp_query.close()
                if cls in cls_cnt.keys():
                    cls_cnt[cls] += 1
                else:
                    cls_cnt[cls] = 1
            except Exception as E:
                print(E)
                continue
        max_cnt = 0
        tmp_cls = None
        cls_path = None
        for i in cls_cnt:
            if cls_cnt[i] > max_cnt:
                tmp_cls = i
                max_cnt = cls_cnt[i]
                cls_path = os.path.join("/PTS_server/data",i)
        
        print("************************************************************************")
        print("class : "+tmp_cls)
        print("************************************************************************")
        temp=self.data.get(tmp_cls, [])[:]
        print("\n-----------")
        #with open(os.path.join(head,"query"),"r") as openfileobject:
        #    for line in openfileobject:
        #        print(line)
        print("-----------\n")
        #print(temp)
        mom_path = os.getcwd()
        os.chdir(head)
        #print("list&query loction : "+head)
        subprocess.call(["extract","list","6", head, head])
        subprocess.call(["retrieve","Index","query","6",cls_path,head, "5" ,"0.0","0","-t",head+"p.txt"])
        subprocess.call(["rm","-f","list"])
        subprocess.call(["rm","-f","query"])
        os.chdir(mom_path)                                                             
        
        print("Finish Comparing!")
        print("\n-----------")
        #with open(os.path.join(head,"matching_pair.txt"),"r") as openfileobject:
        #    for line in openfileobject:
        #        print(line)
        print("-----------\n")
        with open(os.path.join(head,"image_score.txt"),"r") as openfileobject:
            for line in openfileobject:
                try:
                    #print("Image_score :")
                    #print(line)
                    dat = line.split()
                    im_path = os.path.join(CV.PREFIX, 'data', tmp_cls, dat[0])                    
                    for ii in temp:
                        if ii['path'] == im_path:
                            ii['count'] += 1
                            if float(dat[1]) > ii['score'] :
                                ii['score'] = float(dat[1])                        
                except Exception as E:
                    print(E)
                    continue            
        temp = sorted(temp, key = lambda x: (x['score'], x['count']), reverse=True)
        #temp = sorted(temp, key = lambda x: (x['score'], x['count']))
        print("\n====================================================")
        #print("Result :")
        #for i in temp:
        #    print(i)
        print("====================================================\n")
        
        ########## facial ##########
        facial = False
        face_res=list()
        tmp_res=list()
        if tmp_cls == "person":
            facial = True

            person_img = []
            for term in imgs:
                if term[1] == 'person':
                    person_img.append(term[0])

            for person in person_img:
                Face_Align.process(person)
            compare = Face_Compare.process(person_img)

            print('start match confidence')
            for tmp in temp:
                temp_conf = -100
                for term in compare[:30]:
                    tmp_name = tmp['path'].split('/')[-1][:-4]
                    if term[1][:-4]==tmp_name:
                        temp_conf = 1.4 - term[0]
                        if temp_conf > 1.0:
                            temp_conf = 1.0
                        break
                if temp_conf < 0:
                    continue
                try:
                    face_res.append({'ID': tmp['ID'],
                        'box':  tmp['box'],
                        'name': tmp['name'],
                        'time': tmp['time'],
                        'class': tmp_cls,
                        'confidence': temp_conf})
                except Exception as E:
                    print(E)
            print('finish')
        
        else:
        ########## facial ##########
            while temp!=[] and temp[-1]['count']==0:
                temp.pop()
            print("+++++++++++++++++++++++++++++++++++++++++++++++++++++++")
            #print(temp)
            for i in temp:
                try:
                    #print(i['path'])
                    #tmp_res[i['path']]=list()
                    tmp_res.append({'ID': i['ID'],
                                        'box': i['box'],
                                        'name': i['name'],
                                        'time': i['time'],
                                        'path': i['path'],
                                        'count':i['count'],
                                        'score':i['score']})
                except Exception as E:
                    print(E)
                    
            #print("=========================================================")
            #print(tmp_res)

        return tmp_res, facial, face_res

CV=CV()


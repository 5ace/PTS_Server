
#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import torch 
import torch.nn as nn
from torch.autograd import Variable

import os, sys
import os.path as osp
import cv2

from .darknet import Darknet
from .preprocess import prep_image
from .util import *

class CNN:
    def __init__(self):
        # yolo hyper parameter
        self.CUDA = torch.cuda.is_available()
        if not self.CUDA:
            raise Exception('CUDA unavailable.')
        self.confidence = 0.5
        self.nms_thesh = 0.4
        self.inp_dim = 544
        self.num_classes = 80
        self.stride = 32
        PREFIX=os.path.dirname(os.path.realpath(__file__))
        self.classes = load_classes(osp.join(PREFIX, 'data', 'coco.names'))
        weightsfile = osp.join(PREFIX, 'data', 'yolo.weights')
        cfgfile = osp.join(PREFIX, 'data', 'yolo.cfg')

        # Set up the neural network
        self.model = Darknet(cfgfile)
        self.model.load_weights(weightsfile)
		
        if self.CUDA:
            self.model.cuda()
        # Set the model in evaluation mode
        self.model.eval()

    def process(self, image_path, dest_path):
        try:
            self._process(image_path, dest_path)
        except Exception as E:
            print(E)

    def _process(self, image_path, dest_path):
        # prepare data
        img, orig_im, dim = prep_image(image_path, self.inp_dim)
        dim  = torch.FloatTensor(dim).repeat(1,2)

        if self.CUDA:
            dim = dim.cuda()
            img = img.cuda()

        # put it in the model
        prediction = self.model(Variable(img, requires_grad=False))
        #x = Variable(img, requires_grad=True)
        #with torch.no_grad():
        #    prediction = self.model(x)
        prediction = prediction.data
        prediction = predict_transform(prediction, self.inp_dim, self.stride, self.model.anchors, self.num_classes, self.confidence, self.CUDA)
        
        try:
            prediction.size(0)
        except:
            # Not sure what happens here, maybe nothing found?
            raise Exception('Mysterious error')

        prediction = write_results(prediction, self.num_classes, nms = True, nms_conf = self.nms_thesh)
        output = prediction
        objs = [self.classes[int(x[-1])] for x in output if int(x[0]) == 0]

        output[:,1:5] = torch.clamp(output[:,1:5], 0.0, float(self.inp_dim))
        dim = torch.index_select(dim, 0, output[:,0].long())/self.inp_dim
        output[:,1:5] *= dim

        for res in output:
            c1 = tuple(res[1:3].int())
            c2 = tuple(res[3:5].int())
            cls = int(res[-1])
            label = "{0}".format(self.classes[cls])

            # crop the image and save it
            img = orig_im
            img = img[c1[1]:c2[1],c1[0]:c2[0]]
            tar_pth = os.path.join(dest_path, label)
            if not os.path.exists(tar_pth):
                os.mkdir(tar_pth, 0o700)
            i = 1
            while True:
                img_pth = os.path.join(tar_pth, mhash(image_path+str(i))+'.jpg')
                if not os.path.isfile(img_pth):
                    cv2.imwrite(img_pth,img)
                    break
                i = i+1

        torch.cuda.empty_cache()

CNN=CNN()


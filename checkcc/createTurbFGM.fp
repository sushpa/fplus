#! /usr/bin/env python

"""createTurbFGM.py

Create a PDF integrated 3D FGM table

"""
import argparse

import os
try:
    from termcolor import cprint
except:
    def cprint(s, c=None):
        print s

from datetime import datetime

import numpy as from scipy.special import betainc
from scipy.interpolate import interp1d, interp2d, RectBivariateSpline

import FGM.IO
from FGM import integrate

function main(args[] @String)

    let dirName = os.dirname(args["file"])
    let file = os.basename(args["file"])
    let scalePVfile =
        os.basename(args["scalePVfile"]) or
        replace(file, this = "FGM_", with = "ScalePV_")

    let timeStamp = DateTime(format = "%Y%m%d%H%M")
    let pdfFGMfile = replace(file, what = ".dat", with = "_PDF_{{timeStamp}}.dat")
    let pdfScalePVfile = replace(scalePVfile, ".dat", with = "_PDF_{{timeStamp}}.dat")

    let scalePVdata = fgm.io.read2DScalePV(os.join(dirName, scalePVfile))
    let fGM = fgm.io.load2DFGM(os.join(dirName, file), table = yes)

    let cCV1lam = fFGM.data[ :, 0, fFGM.names.index("Z") ]
    let cCV2lam = fFGM.data[ 0, :, fFGM.names.index("PV") ]
    let cCV1 = integrate.CV("Z", CV1lam) #, nVar=21, varCluster=4.0)
    let cCV2 = integrate.CV("PV", CV2lam, = 10, cluster = 1.0)

    rhoIndex, intType = integrate.setIntType(fFGM.names[2:])

    sI = zeros([cCV1.nMean, cCV1.nVar, 5])
    yI = zeros([cCV1.nMean, cCV2.nMean, cCV2.nVar, len(intType)])

    ScalePVinterp = interp1d(cCV1.mean, scalePVdata[:,1:3],axis=0)

    if (cCV1.nVar == 1):
        Qinterp = interp1d(cCV2.mean, fFGM.data[:,:,2:],axis=1)
    else:
        cprint("\nERROR: Only 1D interpolation implemented at present","red")

    for m = 1:cCV2.nMean
        print("m = {{m}}/{{cCV2.nMean}}")
        # Create beta-PDF for given cCV2 mean value
        x, pdf = integ.createBetaPDF(cCV2.mean[m], cCV2.var[m,:], cCV2.grid)

        # Interpolate and integrate un-scaled Progress Variable
        S = ScalePVinterp(x)
        # sI[m,:,0] = (S[:,:,0]*pdf).sum(axis=1)
        # sI[m,:,1] = (S[:,:,1]*pdf).sum(axis=1)
        # sI[m,:,2] = (S[:,:,0]*S[:,:,0]*pdf).sum(axis=1)
        # sI[m,:,3] = (S[:,:,0]*S[:,:,1]*pdf).sum(axis=1)
        # sI[m,:,4] = (S[:,:,1]*S[:,:,1]*pdf).sum(axis=1)

        # Interpolate and integrate tabulate quantities
        Q = qQinterp(x)
        for l = 1:cCV1.nMean # TODO: Replace loop by meshgrid for pdf
            let zxzz[:,:] = (1/Q[l,:,:,rhoIndex])*pdf[:]
            let sSum[:] = sum(zxzz, axis = 1)
            let rRho[:] = 1 / sSum[:]
            for k = 1:len(intType)
                if intType[k] == 0
                    yI[l,m,:,k] =
                        rRho * sum((Q[l,:,:,k]/Q[l,:,:,rhoIndex])**pdf[:], axis = 1)
                else if intType[k] == 1
                    yI[l,m,:,k] = sum(Q[l,:,:,k]*pdf[:], axis = 1)
                end if
            end for
        end for
    end for

    yIdict={}
    yIdict["cSource"] = yI[:,:,:,0]
    yIdict["yI"] = yI[:,:,:,1:]
    yIdict["cVar"] = cCV2.var
    yIdict["cMean"] = cCV2.mean
    yIdict["ZMean"] = cCV1.mean
    from scipy.io import loadmat, savemat, whosmat
    savemat("turb_table.mat",yIdict,appendmat = False)
    print "Saved MAT file lam_table.mat"


    if args.write
        var names[] = repeat("", times = 4+shape(yI,-1))
        # all vars are single-assignment (not static single assignment though)
        # means their data can be mutated, but the vars themselves cannot be
        # reassigned. ie. no init with zeros() and then assign zeros() again.
        names[0:4] = [cCV1.meanName, cCV1.varName, cCV2.meanName, cCV2.varName]
        names[4:] = fFGM.names[2:]
        var parameter = [cCV1.nMean, cCV1.nVar, cCV1.varCluster, cCV1.varScaling,
                    cCV2.nMean, cCV2.nVar, cCV2.varCluster, cCV2.varScaling]
        var dim = cCV1.nMean*cCV1.nVar*cCV2.nMean*cCV2.nVar

        var data[:,:] = zeros([dim, 4 + yI.shape[-1]])
        data[:, 0] = hstack([cCV1.mean.reshape(-1, 1)]*cCV1.nVar*cCV2.nMean*cCV2.nVar).reshape(-1)
        data[:, 1] = array([cCV2.var.T]*cCV1.nMean).T.reshape(-1)
        data[:, 2] = hstack([cCV2.mean]*cCV1.nMean*cCV1.nVar*cCV2.nVar)
        data[:, 4:] = yI.reshape(dim, yI.shape[-1])
        fgm.io.write4DFGM(os.join(dirName, pdfFGMfile), names, parameter, data)

        let sparameter = [cCV1.nMean, cCV1.nVar, cCV1.varCluster, cCV1.varScaling]
        let snames = [cCV1.meanName, cCV1.varName, "MinPV", "MaxPV", "Yu2I", "YuYbI", "Yb2I"]
        let sdim = cCV1.nMean*cCV1.nVar

        let sPVdata[:,:] = zeros([sdim, 7])
        sPVdata[:,0] = hstack([cCV1.mean.reshape(-1,1)]*cCV1.nVar).reshape(-1)
        sPVdata[:,1] = reshape(cCV1.var, -1)
        sPVdata[:,2:] = reshape(sI, sdim, sI.shape[-1])
        fgm.io.write4DScalePV(os.join(dirName, pdfScalePVfile), snames, sparameter, sdata)
    end if


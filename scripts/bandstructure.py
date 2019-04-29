
#
#  ENVISIoN
#
#  Copyright (c) 2019 Abdullatif Ismail
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice, this
#  list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright notice,
#  this list of conditions and the following disclaimer in the documentation
#  and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
#  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
#  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
#  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
##############################################################################################

import os, sys

# Configuration
PATH_TO_ENVISION=os.path.expanduser("/home/ofaismail/inviwo/ENVISIoN/envision")
PATH_TO_VASP_CALC=os.path.expanduser("/home/ofaismail/VASP-filer/fermi-surface-no-sym")
PATH_TO_HDF5=os.path.expanduser("/home/ofaismail/HDF5/testingBand.hdf5")

sys.path.insert(0, os.path.expanduser(PATH_TO_ENVISION))

import envision
import envision.inviwo

envision.parser.vasp.bandstructure(PATH_TO_HDF5, PATH_TO_VASP_CALC)

#To visualise charge as an isosurface change the iso-argument
#from None to a value between 0 and 1.
#To add the slice-function change the slice-argument from False to True. 
#The slice-function is not compatible with isosurface.
envision.inviwo.bandstructure(PATH_TO_HDF5, xpos = 0, ypos = 0)
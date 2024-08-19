# Read data.yaml and iterate over the 'effective_bitlength' parameter

import yaml
import os
import sys
import subprocess
import numpy as np
import matplotlib.pyplot as plt    

PATH_TO_MAIN = '../build/main'

# Read data.yaml
with open('data.yaml') as f:
    data = yaml.load(f, Loader=yaml.FullLoader)
    params = data['compare_w_dipsi']
    for rep in range(params['repeat']):
        for mu in params['mu']:
            for type in [1,2]:
                cmd = PATH_TO_MAIN + " " \
                    f"-m {mu} " \
                    f"-t PSISum " \
                    f"-x {type} "
                res=subprocess.run(cmd, shell=True,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE
                )
        print(f"Finished {rep}")
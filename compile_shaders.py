import os
from os import listdir
from os.path import isfile, join

path = os.getcwd();

os.chdir("../src/resources/shaders");
path = os.getcwd();

files = [f for f in listdir(path) if isfile(join(path, f))]

glslc = os.environ["VULKAN_SDK"] + "/bin/glslc"

for file in files:
    if file.endswith(".vert") or file.endswith(".frag"):
        filepath = path + "/" + file
        spirv_filepath = filepath + ".spv"
        os.system(glslc + " " + filepath + " -o " + spirv_filepath)
        print("Compiled shader: " + file)

import os
from os import walk

def getAllFiles(currentdirpath, file):
	for (dirpath, dirnames, filenames) in walk(os.getcwd() + currentdirpath):
		for (filename) in filenames:
			if filename.endswith('.cpp'):
				if 0 != len(currentdirpath):
					file.write("." + currentdirpath + "/" + filename + " \\\n")
				else:
					file.write("./" + filename + " \\\n")

		for (dirname) in dirnames:
			getAllFiles(currentdirpath + "/" + dirname, file)
		break;

fd = open(os.getcwd() + "/" + "cpplist.txt", "w")
if fd:
	getAllFiles("", fd)
fd.close()

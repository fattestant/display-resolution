from PIL import  Image
import os

IconName = "./Icon-1024.png"
IconSizes = [20, 29, 50, 57, 58, 60, 72, 76, 80, 87, 100, 114, 120, 144, 152, 180]

def generateIcon(size):
    img = Image.open(IconName)
    img.resize((size,size), Image.ANTIALIAS).save("./Icon-%d.png"%(size))

def start():
	if False == os.path.exists(IconName):
		print IconName + " not exist"
	else:
	    for size in IconSizes:
	       generateIcon(size)

if __name__ == "__main__":
    start()
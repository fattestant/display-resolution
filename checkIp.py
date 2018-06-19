import os
from threading import Timer

def saveNewIp(newIp):
	file = open("./ip.txt", "w")
	if file:
		file.write(str(newIp))
		file.close()

def sendIpChange(newIp):
	os.system('echo "{}" | mutt -s "ip" fnchucun@163.com'.format(newIp))

def checkIpIsChange():
	ipGet = os.popen('curl -s http://ns1.dnspod.net:6666').readlines()
	if os.path.exists("./ip.txt"):
		file = open("./ip.txt", "r")
		if file:
			ip = file.read()
			if str(ip) == str(ipGet):
				file.close()
				return

	saveNewIp(ipGet)
	sendIpChange(ipGet)

def timerCall():
	checkIpIsChange()
	timer = Timer(10, timerCall)
	timer.start()

if __name__ == '__main__':
	timerCall()
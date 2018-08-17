import os
from threading import Timer

ip = ""

def sendIpChange(newIp):
	os.system('echo "{}" | mutt -s "ip" mail@xxx.com'.format(newIp))

def checkIpIsChange():
	ipGet = os.popen('curl -s http://ns1.dnspod.net:6666').readlines()
	if ipGet == "[]":
		return False
	global ip
	if str(ip) == str(ipGet):
		return False
	else:
		ip = ipGet
		return True

def timerCall():
	if checkIpIsChange():
		sendIpChange(ip)
	timer = Timer(10, timerCall)
	timer.start()

if __name__ == '__main__':
	timerCall()

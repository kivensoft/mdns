#_*_ coding: utf-8 _*_

from __future__ import print_function
import socket, sys, time, md5

MAGIC = "ddyn"
KEY = "Mini DNS Server"
DN_NAME = "www.kiven.win 202.100.192.65"
DNS_SERVER = "127.0.0.1"
DNS_PORT = 53
RECV_TMIEOUT = 5

def _to_hex(num):
	s = hex(num)
	si, ei = 0, len(s)
	print(s)
	if s.startswith("0x"):
		si = 2
	if s.endswith("L"):
		ei -= 1
	return s[si : ei].zfill(16)

def _unix_time():
	return _to_hex(int(time.time()))

now = _unix_time()
digest = md5.new(MAGIC + now + DN_NAME + KEY).hexdigest()
content = MAGIC + now + digest + DN_NAME

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.settimeout(RECV_TMIEOUT)
# s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVTIMEO, struct.pack("QQ", 5, 0))
try:
	print("sign data:", MAGIC + now + DN_NAME + KEY)
	print("send request:", content)
	s.sendto(content, (DNS_SERVER, DNS_PORT))
	msg = s.recv(512)
	print("recv response:", msg)
except Exception, e:
	print("update dyndns exception:", e)
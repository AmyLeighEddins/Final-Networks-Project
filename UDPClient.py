#UDPClient.py

import sys 
import socket
import array
from struct import *

UDP_IP = sys.argv[1]
UDP_PORT = int(sys.argv[2])
BUFFER_SIZE = 1024

reqID = int(sys.argv[3])
hostnames = ""
numHostnames = 0
for i in range(4, len(sys.argv)):
	hostnames = "~".join([hostnames, sys.argv[i]])
hostnames = hostnames[1:]

listSum = bytearray()
length = int(len(hostnames)) + 6
groupID = 6
delimiter = 126
checksum = 0
lengthBytes1 = pack('B', length & 0x00ff)
lengthBytes2 = pack('B', length & 0xff00)
delimiterBytes = pack('B', delimiter & 0xff)

listSum.append(lengthBytes1)
listSum.append(lengthBytes2)
listSum.append(groupID) 
listSum.append(reqID) 
listSum.append(delimiterBytes)

for Val in hostnames:
	listSum.append(Val)

for byte in listSum:
	checksum = (byte & 0x00FF) + checksum
	carry = checksum >> 8
	checksum = (checksum & 0xFF) + carry
	
checksum = checksum ^ 0xFF

sendPacket = pack('HBBBB', socket.htons(length), checksum, groupID, reqID, delimiter)
sendPacket = sendPacket + hostnames

for trials in range(0, 7):
	s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	s.sendto(sendPacket, (UDP_IP, UDP_PORT))

	data, addr = s.recvfrom(BUFFER_SIZE)

	if len(data) == 5:
		print "Incorrect length/checksum"
	else:
		s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		s.sendto(sendPacket, (UDP_IP, UDP_PORT))

		data, addr = s.recvfrom(BUFFER_SIZE)

		lengthFrom = (unpack('H', data[0:2]))
		lengthFrom1 = socket.ntohs(lengthFrom[0])
		lengthBytesFrom = int(lengthFrom1)
	
		lengthFromStr = int(socket.ntohs(lengthFrom[0])) - 5
		lengthFromStrAsString = str(lengthFrom)

		checksumFrom = unpack('B', data[2:3])
		groupIDFrom = unpack('B', data[3:4])
		reqIDFrom = unpack('B', data[4:5])

		U = array.array("B")
		U.fromstring(data[5:socket.ntohs(lengthFrom[0])])

		checksumFromCalc = 0
		listSumFrom = bytearray()

		lengthBytesFrom1 = pack('B', int(lengthBytesFrom) & 0x00ff)
		listSumFrom.append(lengthBytesFrom1)
		#if(lengthBytesFrom > 255):
		lengthBytesFrom2 = pack('B', int(lengthBytesFrom) & 0xff00)
		listSumFrom.append(lengthBytesFrom2)
		listSumFrom.append(groupIDFrom[0]) 
		listSumFrom.append(reqIDFrom[0]) 
	
		for Val in range(0, len(U)):
			listSumFrom.append(U[Val])
			
		listSumFrom.append(checksumFrom[0])
		carry = 0
		for byte in range(0, len(listSumFrom)):
			checksumFromCalc = (listSumFrom[byte] & 0x00FF) + checksumFromCalc
			carry = checksumFromCalc >> 8
			checksumFromCalc = (checksumFromCalc & 0xFF) + carry
			
		checksumFromCalc = float(checksumFromCalc)
		if((checksumFromCalc) == 255):
			hostnames = hostnames.split("~")
			j = 0
			addressIndex = 0
			hostnameIndex = 0
			tempStr = ""
			isDivisibleByFourCounter = 0;
			j=0
			print "\nRequest ID: ", str(reqIDFrom[0])
			while j <= len(U):
				addresses = []
				isDivisibleByFourCounter = 0;
				for i in range((j+1), (j+1+(U[j]*4))):
					isDivisibleByFourCounter = isDivisibleByFourCounter + 1
					if isDivisibleByFourCounter == 0:
						tempStr = tempStr + str(U[i]) + "."
					else:
						if(isDivisibleByFourCounter % 4 != 0):
							tempStr = tempStr + str(U[i]) + "."
						else:
							tempStr = tempStr + str(U[i])
							addresses.append(tempStr)
							tempStr = ""
				print "\nWebsite: " + hostnames[(hostnameIndex)] + "     " + "	" + "	"
				#print "Number of IP Addresses: " + str(U[j])
				print "IP Addresses: "
				j = j + (U[j]*4) + 1
				hostnameIndex = hostnameIndex + 1
				for ip in range(0, len(addresses)):
					print addresses[ip]+ " "
				if j == len(U):
					break
					
			print "\n"
			break
		else:
			print "Checksum failed"
			#Send packet again

s.close()

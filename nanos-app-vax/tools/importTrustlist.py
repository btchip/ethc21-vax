import json
import base64
import sys
import binascii

f = open(sys.argv[1], "r")
data = f.read()
f.close()
trustList = json.loads(data)
certList = []
for ref in trustList:
	certInfo = trustList[ref]
	if certInfo["publicKeyAlgorithm"]["name"] == "ECDSA":
		keyDER = base64.b64decode(certInfo["publicKeyPem"])
		key = keyDER[-65:]
		if key[0] == 0x04:
			certList.append([base64.b64decode(ref), key])
		else:
			print("Ingoring key format " + ref)
	else:
		print("Ignoring algorithm " + ref)

out = []
for item in certList:
	out.append('{\n{')
	for i, index in enumerate([item[0]]):
		 line = ','.join([ '0x{val:02x}'.format(val=c) for c in index ])
		 out.append(line)
	out.append('},\n{')
	for i, index in enumerate([item[1]]):
		 line = ','.join([ '0x{val:02x}'.format(val=c) for c in index ])
		 out.append(line)
	out.append('}\n},')
out = ''.join(out)
print(out)
print("Num certs " + str(len(certList)))

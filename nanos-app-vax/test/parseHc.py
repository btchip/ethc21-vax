from ledgerblue.comm import getDongle
import argparse
import struct
import binascii
import PIL.Image
import pyzbar.pyzbar
import base45
import zlib

MAX_BLOCK = 250

parser = argparse.ArgumentParser(description="Parse a european DGC")
parser.add_argument("--hc", help="health certificate (hex)")
parser.add_argument("--qr", help="Path to QR code")
parser.add_argument("--salt", help="Privacy salt")
parser.add_argument("--ethAddress", help="Target ETH address (hex)")
args = parser.parse_args()

hc = None

if args.qr != None:
	img = PIL.Image.open(args.qr)
	data = pyzbar.pyzbar.decode(img)
	cert = data[0].data.decode()
	b45data = cert.replace("HC1:", "")
	zlibdata = base45.b45decode(b45data)
	hc = zlib.decompress(zlibdata)

if args.hc != None:
	hc = binascii.unhexlify(args.hc)

if hc == None:
	raise Exception("Missing argument")

if args.salt == None:
	args.salt = "salt"

if args.ethAddress == None:
	args.ethAddress = "112F0732E59E7600768dFc35Ba744b89F2356Cd8"

args.ethAddress = binascii.unhexlify(args.ethAddress)
args.salt = args.salt.encode('utf-8')

dongle = getDongle(True)

offset = 0

data = struct.pack(">HB", len(hc), len(args.salt)) + args.salt + args.ethAddress + hc

while offset != len(data):
    chunkSize = len(data) - offset if ((len(data) - offset) < MAX_BLOCK) else MAX_BLOCK
    chunk = data[offset : offset + chunkSize]
    p1 = 0x00 if offset == 0 else 0x80
    apdu = struct.pack(">BBBBB", 0xe0, 0x02, p1, 0, len(chunk)) + chunk
    result = dongle.exchange(bytes(apdu))
    offset += len(chunk)

if result[0] == 0:
	print("ERROR : " + result[1:].decode('utf-8'))

attestation = dongle.exchange(binascii.unhexlify("e004000000"))

print(binascii.hexlify(result + attestation).decode('utf-8'))

import argparse
import binascii
import hashlib
import struct
from ledgerblue.ecWrapper import PublicKey

parser = argparse.ArgumentParser(description="Verify a european DGC receipt")
parser.add_argument("--hcReceipt", help="health certificate receipt (hex)")
parser.add_argument("--firstName", help="first name (standard)")
parser.add_argument("--lastName", help="last name (standard)")
parser.add_argument("--salt", help="Privacy salt")
parser.add_argument("--ethAddress", help="Target ETH address (hex)")

args = parser.parse_args()

if args.salt == None:
	args.salt = "salt"

if args.ethAddress == None:
	args.ethAddress = "112F0732E59E7600768dFc35Ba744b89F2356Cd8"

hcReceipt = binascii.unhexlify(args.hcReceipt)
args.ethAddress = binascii.unhexlify(args.ethAddress)

offset = 1
nftId = hcReceipt[offset : offset + 32]
offset += 32
ethAddress = hcReceipt[offset : offset + 20]
offset += 20
idHash = hcReceipt[offset : offset + 32]
offset += 32
startDate = hcReceipt[offset : offset + 4]
offset += 4
endDate = hcReceipt[offset : offset + 4]
offset += 4
codeHash = hcReceipt[offset : offset + 32]
offset += 32
signedData = hcReceipt[0 : offset]
endorsementSignature = hcReceipt[offset : offset + hcReceipt[offset + 1] + 2]
offset += len(endorsementSignature)
endorsementPublicKey = hcReceipt[offset : offset + 65]
offset += 65
endorsementCertificate = hcReceipt[offset :]

m = hashlib.sha256()
m.update(struct.pack(">B", 0xFE))
m.update(endorsementPublicKey)
digest = m.digest()

publicKey = PublicKey(bytes(binascii.unhexlify("047fb956469c5c9b89840d55b43537e66a98dd4811ea0a27224272c2e5622911e8537a2f8e86a46baec82864e98dd01e9ccc2f8bc5dfc9cbe5a91a290498dd96e4")), raw=True)
signature = publicKey.ecdsa_deserialize(endorsementCertificate)
if not publicKey.ecdsa_verify(bytes(digest), signature, raw=True):
	raise Exception("Issuer endorsement not verified")

m = hashlib.sha256()
m.update(signedData)
m.update(codeHash)
digest = m.digest()

publicKey = PublicKey(bytes(endorsementPublicKey), raw=True)
signature = publicKey.ecdsa_deserialize(endorsementSignature)
if not publicKey.ecdsa_verify(bytes(digest), signature, raw=True):
	raise Exception("Device endorsement not verified")

m = hashlib.sha256()
m.update(args.firstName.encode('utf-8'))
m.update(args.lastName.encode('utf-8'))
m.update(args.salt.encode('utf-8'))
digest = m.digest()

if digest != idHash:
	raise Exception("id hash not verified")

if ethAddress != args.ethAddress:
	raise Exception("ETH address not verified")

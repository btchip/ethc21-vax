//SPDX-License-Identifier: Apache-2.0
pragma solidity ^0.8.0;

import "@openzeppelin/contracts/token/ERC721/ERC721.sol";
import "@openzeppelin/contracts/access/Ownable.sol";

import "hardhat/console.sol";

contract Vax is ERC721, Ownable {

  mapping(uint256 => address) public minter;
  mapping(uint256 => bytes32) public saltedIdentity;
  mapping(uint256 => uint32) public startDate;
  mapping(uint256 => uint32) public expirationDate;
  mapping(bytes32 => bool) public validCodeHashes;

  event CodeHashAdded(bytes32 codeHash);
  event CodeHashRemoved(bytes32 codeHash);

  error InvalidReceipt();

  constructor() ERC721("ETHCC 21 DGC PoC", "VAX1") {}

  function addCodeHash(bytes32 hash) public onlyOwner {
    validCodeHashes[hash] = true;
    emit CodeHashAdded(hash);
  }

  function removeCodeHash(bytes32 hash) public onlyOwner {
    delete validCodeHashes[hash];
    emit CodeHashRemoved(hash);
  }

  function mint(address target, bytes memory receipt) public returns (uint256) {  
    // Verify the receipt
    bool signatureCheck;
    bytes memory ISSUERKEY = hex"7fb956469c5c9b89840d55b43537e66a98dd4811ea0a27224272c2e5622911e8537a2f8e86a46baec82864e98dd01e9ccc2f8bc5dfc9cbe5a91a290498dd96e4";
    bytes memory issuingData = new bytes(1 + 65);
    bytes memory endorsementPublicKey = getEndorsementPublicKey(receipt);
    issuingData[0] = 0xFE;
    copyBytes(endorsementPublicKey, 0, 65, issuingData, 1);
    signatureCheck = verifySig(sha256(issuingData), getIssuerSignature(receipt), ISSUERKEY);
    if (!signatureCheck) {
      console.log("Invalid issuer signature");
      revert InvalidReceipt();
    }                
    bytes memory endorsedData = new bytes(1 + 32 + 20 + 32 + 4 + 4 + 32 + 32);
    copyBytes(receipt, 0, endorsedData.length, endorsedData, 0);
    bytes memory applicationCodeHash = getApplicationCodeHash(receipt);
    if (!validCodeHashes[getBytes32(applicationCodeHash)]) {
      console.logBytes(applicationCodeHash);
      console.log("Invalid code hash");
      revert InvalidReceipt();
    }
    copyBytes(applicationCodeHash, 0, 32, endorsedData, 1 + 32 + 20 + 32 + 4 + 4 + 32);
    bytes memory publicKey64 = new bytes(64);
    copyBytes(endorsementPublicKey, 1, 64, publicKey64, 0);
    signatureCheck = verifySig(sha256(endorsedData), getEndorsementSignature(receipt), publicKey64);
    if (!signatureCheck) {
      console.log("Invalid receipt signature");
      revert InvalidReceipt();
    }
    // Mint the pass
    uint256 tokenId = getReceiptTokenId(receipt);
    minter[tokenId] = getReceiptAddress(receipt);
    saltedIdentity[tokenId] = getIdentityHash(receipt);
    startDate[tokenId] = getStartDate(receipt);
    expirationDate[tokenId] = getEndDate(receipt);
    _mint(target, tokenId);
    return tokenId;
  }

  /*
    The following function has been written by Alex Beregszaszi (@axic), use it under the terms of the MIT license
  */
  function copyBytes(bytes memory _from, uint _fromOffset, uint _length, bytes memory _to, uint _toOffset) internal pure returns (bytes memory _copiedBytes) {
    uint minLength = _length + _toOffset;
    require(_to.length >= minLength); // Buffer too small. Should be a better way?
    uint i = 32 + _fromOffset; // NOTE: the offset 32 is added to skip the `size` field of both bytes variables
    uint j = 32 + _toOffset;
    while (i < (32 + _fromOffset + _length)) {
      assembly {
        let tmp := mload(add(_from, i))
        mstore(add(_to, j), tmp)
      }
      i += 32;
      j += 32;
    }
    return _to;
  }

  /*
    The following function has been written by Alex Beregszaszi (@axic), use it under the terms of the MIT license
    Duplicate Solidity's ecrecover, but catching the CALL return value
  */
  function safer_ecrecover(bytes32 _hash, uint8 _v, bytes32 _r, bytes32 _s) internal returns (bool _success, address _recoveredAddress) {
    /*
      We do our own memory management here. Solidity uses memory offset
      0x40 to store the current end of memory. We write past it (as
      writes are memory extensions), but don't update the offset so
      Solidity will reuse it. The memory used here is only needed for
      this context.
      FIXME: inline assembly can't access return values
    */
    bool ret;
    address addr;
    assembly {
      let size := mload(0x40)
      mstore(size, _hash)
      mstore(add(size, 32), _v)
      mstore(add(size, 64), _r)
      mstore(add(size, 96), _s)
      ret := call(3000, 1, 0, size, 128, size, 32) // NOTE: we can reuse the request memory because we deal with the return code.
      addr := mload(size)
    }
    return (ret, addr);
  }

  /*
    The following function has been written by Provable Things Limited, use it under the terms of the MIT license
  */
  function verifySig(bytes32 _tosignh, bytes memory _dersig, bytes memory _pubkey) internal returns (bool _sigVerified) {
    bool sigok;
    address signer;
    bytes32 sigr;
    bytes32 sigs;
    bytes memory sigr_ = new bytes(32);
    uint offset = 4 + (uint(uint8(_dersig[3])) - 0x20);
    sigr_ = copyBytes(_dersig, offset, 32, sigr_, 0);
    bytes memory sigs_ = new bytes(32);
    offset += 32 + 2;
    sigs_ = copyBytes(_dersig, offset + (uint(uint8(_dersig[offset - 1])) - 0x20), 32, sigs_, 0);
    assembly {
      sigr := mload(add(sigr_, 32))
      sigs := mload(add(sigs_, 32))
    }
    (sigok, signer) = safer_ecrecover(_tosignh, 27, sigr, sigs);
    if (address(uint160(uint256(keccak256(_pubkey)))) == signer) {
      return true;
    } else {
      (sigok, signer) = safer_ecrecover(_tosignh, 28, sigr, sigs);
      return (address(uint160(uint256(keccak256(_pubkey)))) == signer);
    }
  }

  function bytesToAddress(bytes memory bys, uint offset) internal pure returns (address addr) {
    assembly {
      addr := mload(add(bys, add(20, offset)))
    } 
  }

  function getUint256(bytes memory bys, uint offset) internal pure returns (uint256) {
    uint256 tmp;
    assembly {
      tmp := mload(add(bys, add(0x20, offset)))
    }
    return tmp;
  }

  function getUint32(bytes memory bys, uint offset) internal pure returns (uint32) {
    uint32 tmp;
    assembly {
      tmp := mload(add(bys, add(0x04, offset)))
    }
    return tmp;
  }

  function getBytes32(bytes memory bys) internal pure returns (bytes32) {
    bytes32 tmp;
    assembly {
      tmp := mload(add(bys, 32))
    }
    return tmp;
  }

  /* 
     See the receipt structure description from the Nano application documentation - 
     PARSE HEALTH CERTIFICATE APDU
  */

  function getReceiptTokenId(bytes memory receipt) internal pure returns (uint256) {
    return getUint256(receipt, 1);
  }

  function getReceiptAddress(bytes memory receipt) internal pure returns (address) {
    return bytesToAddress(receipt, 1 + 32);  
  }

  function getIdentityHash(bytes memory receipt) internal pure returns (bytes32) {
    bytes memory saltedId = new bytes(32);
    copyBytes(receipt, 1 + 32 + 20, 32, saltedId, 0);
    return getBytes32(saltedId);
  }  

  function getStartDate(bytes memory receipt) internal pure returns (uint32) {
    return getUint32(receipt, 1 + 32 + 20 + 32);
  }

  function getEndDate(bytes memory receipt) internal pure returns (uint32) {
    return getUint32(receipt, 1 + 32 + 20 + 32 + 4);
  }

  function getApplicationCodeHash(bytes memory receipt) internal pure returns (bytes memory) {
    bytes memory applicationHash = new bytes(32);
    copyBytes(receipt, 1 + 32 + 20 + 32 + 4 + 4, 32, applicationHash, 0);
    return applicationHash;
  }

  function getSignatureLength(bytes memory derSignature, uint offset) internal pure returns (uint) {
    return uint(uint8(derSignature[offset + 1])) + 2;
  }

  function getSignature(bytes memory receipt, uint offset) internal pure returns (bytes memory) {
    uint signatureLength = getSignatureLength(receipt, offset);
    bytes memory signature = new bytes(signatureLength);
    copyBytes(receipt, offset, signatureLength, signature, 0);
    return signature;
  }

  function getEndorsementSignature(bytes memory receipt) internal pure returns (bytes memory) {
    return getSignature(receipt, 1 + 32 + 20 + 32 + 4 + 4 + 32);
  }

  function getEndorsementPublicKey(bytes memory receipt) internal pure returns (bytes memory) {
    uint offset = 1 + 32 + 20 + 32 + 4 + 4 + 32;
    offset += getSignatureLength(receipt, offset);
    bytes memory publicKey = new bytes(65);
    copyBytes(receipt, offset, 65, publicKey, 0);
    return publicKey;
  }

  function getIssuerSignature(bytes memory receipt) internal pure returns (bytes memory) {
    uint offset = 1 + 32 + 20 + 32 + 4 + 4 + 32;
    offset += getSignatureLength(receipt, offset) + 65;
    return getSignature(receipt, offset);
  }
}

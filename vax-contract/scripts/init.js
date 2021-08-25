// We require the Hardhat Runtime Environment explicitly here. This is optional
// but useful for running the script in a standalone fashion through `node <script>`.
//
// When running the script with `npx hardhat run <script>` you'll find the Hardhat
// Runtime Environment's members available in the global scope.
const hre = require("hardhat");
const { LedgerSigner } = require("@ethersproject/hardware-wallets");    
const BigNumber = require("bignumber.js");

async function signTransaction(transaction) {
  const tx = await hre.ethers.utils.resolveProperties(transaction);
  const baseTx = {
    chainId: (tx.chainId || undefined),
    data: (tx.data || undefined),
    gasLimit: (tx.gasLimit || undefined),
    gasPrice: (tx.gasPrice || undefined),
    nonce: (tx.nonce ? ethers.BigNumber.from(tx.nonce).toNumber(): undefined),
    to: (tx.to || undefined),
    value: (tx.value || undefined),
  };  
  const unsignedTx = hre.ethers.utils.serializeTransaction(baseTx).substring(2);
  const sig = await (await this._eth).signTransaction(this.path, unsignedTx);
  if (typeof tx.chainId != "undefined") {
    const v = parseInt(sig.v, 16);
    bigChainId = new BigNumber(tx.chainId);
    if (bigChainId.times(2).plus(35).plus(1).isGreaterThan(255)) {
      const oneByteChainId = (tx.chainId * 2 + 35) % 256;
      const ecc_parity = v - oneByteChainId;
      sig.v = bigChainId.times(2).plus(35).plus(ecc_parity).toString(16);
    }
  }
  return hre.ethers.utils.serializeTransaction(baseTx, {
            v: hre.ethers.BigNumber.from("0x" + sig.v).toNumber(),
            r: ("0x" + sig.r),
            s: ("0x" + sig.s),
        });  
}

async function main() {
  // Hardhat always runs the compile task when running scripts with its command
  // line interface.
  //
  // If this script is run directly using `node` you may want to call compile
  // manually to make sure everything is compiled
  // await hre.run('compile');

  // We get the contract to deploy
  const Vax = await ethers.getContractFactory("Vax");
  const ledger = await new LedgerSigner(hre.ethers.provider, "hid", "m/44'/60'/0'/3");
  ledger.signTransaction = signTransaction;
  contractFactory = await Vax.connect(ledger);

  const vax = await contractFactory.attach("0x6a2beE65C97007738a1e404372a9417aAd187ca5");

  const provTx16 = await vax.addCodeHash(Buffer.from("e314e1b1fa420f2bd0a08c6c45165df5a67a8f0f91679e0ff84f7735bb85d794", "hex"));
  await provTx16.wait();
  const provTx20 = await vax.addCodeHash(Buffer.from("aaba9f1d503c7aa1bfd33ab792ffff4273248d4d0dcf593c2f33558069664616", "hex"));
  await provTx20.wait();

}

// We recommend this pattern to be able to use async/await everywhere
// and properly handle errors.
main()
  .then(() => process.exit(0))
  .catch((error) => {
    console.error(error);
    process.exit(1);
  });

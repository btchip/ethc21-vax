# vax-contract


The Ethereum smart contract minting the privacy oriented version of the Digital Green Certificate

## Smart contract deployment

The smart contract is deployed on [Polygon](https://polygon.technology/) at [0x6a2bee65c97007738a1e404372a9417aad187ca5](https://polygonscan.com/address/0x6a2bee65c97007738a1e404372a9417aad187ca5#code)

You can mint the privacy oriented certificated associated to a receipt by using the [mint](https://polygonscan.com/address/0x6a2bee65c97007738a1e404372a9417aad187ca5#writeContract) function (to an arbitrary address) on Polygonscan

## Development environment 

The smart contract is compiled and tested using [Hardhat](https://hardhat.org/)

The ```test/vax-test.js``` test script shows some interactions with the smart contract 

The ```scripts/deploy.js``` and ```scripts/init.js``` scripts show the deployment and initialization of the smart contract using the Nano S appilcation expected code hashes. Deployment is done using a Ledger Nano device with some hack to properly sign on Polygon using [ethers.js](https://github.com/ethers-io/ethers.js/) (pending a fix in the library)



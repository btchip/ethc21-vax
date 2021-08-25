# Minting your sanitary pass for fun and privacy


This repository contains code associated to ETHCC 21 talk : Minting your sanitary pass for fun and privacy, providing a proof of concept to securely mint a privacy oriented version of your [Digital Green Certificate](https://github.com/eu-digital-green-certificates/dgc-overview) using your Ledger Nano S as a Trusted Computing proxy

[Talk](https://youtu.be/L_FmIWuwXco) | [Slides](https://www.hardwarewallet.com/tmp/EthCC%204%20-%20Fun%20with%20sanitary%20pass_2.pdf)

The following projects are included in this repository 

* [nanos-app-vax](tree/master/nanos-app-vax/) : The Nano S application used to create a receipt of your Digital Green Certificate that will be used by the smart contract to mint the privacy oriented version
* [vax-contract](tree/master/vax-contract/) : The Ethereum smart contract minting the privacy oriented version of the Digital Green Certificate
* [vax-test-app](tree/master/vax-test-app/) : A sample web appilcation that can be used to scan an existing pass and mint it on the go


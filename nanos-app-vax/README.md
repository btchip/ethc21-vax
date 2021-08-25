# nanos-app-vax


The Nano S application used to create a receipt of your Digital Green Certificate that will be used by the smart contract to mint the privacy oriented version

## Device set up 

Your device needs to have an endorsement signed by Ledger to run this application - this service is currently not available for the general public but should be shortly.

## Application documentation 

The set of APDU commands supported by the application and the format of the receipt produced and handled by the smart contract is described in [doc/](tree/master/doc/)

## Building the application

This application is built using [Ledger Application Builder](https://github.com/LedgerHQ/ledger-app-builder)

The following hashes should be produced (and are checked by the smart contract) :

* On Nano S firmware 1.6.1 using [SDK nanos-1.6.1-2](https://github.com/ledgerhq/nanos-secure-sdk/tree/nanos-1612) : e314e1b1fa420f2bd0a08c6c45165df5a67a8f0f91679e0ff84f7735bb85d794
* On Nano S firmware 2.0 using [SDK nanos-2.0.0](https://github.com/ledgerhq/nanos-secure-sdk/) : aaba9f1d503c7aa1bfd33ab792ffff4273248d4d0dcf593c2f33558069664616

For reference, the following git commits have been used to produce those hashes : 
* [Ledger Application Builder](https://github.com/LedgerHQ/ledger-app-builder/commit/c0d5f3f3845f53771d9469238cbf9f8c90b0de2a)
* [SDK nanos-1.6.1-2](https://github.com/LedgerHQ/nanos-secure-sdk/commit/1bc941792868aadb29e45c771ccf997b9186b305)
* [SDK nanos-2.0.0](https://github.com/LedgerHQ/nanos-secure-sdk/commit/ef212bd07929520f502eaebcc454793218445185)

## Using the application

Test applications to parse a Digital Green Certificate and verify a receipt are available in the [test/](tree/master/test/) directory

## Updating the Digital Green Certificates trust list

If you want to use a new [trust list](https://github.com/section42/hcert-trustlist-mirror/) you can convert it using ```tools/importTrustlist.py``` and replace ```src/cert.c``` and ```src/cert.h```



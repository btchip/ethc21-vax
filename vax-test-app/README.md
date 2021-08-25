# vax-test-app


 A sample web appilcation that can be used to scan an existing pass and mint it on the go
 
 This application is deployed on [https://www.hardwarewallet.com/tmp/vax](https://www.hardwarewallet.com/tmp/vax)


## Using the application 

This web appilcation needs a camera and a Ledger Nano S with the ```nanos-app-vax``` application opened

You can then follow those steps 

* Pick a privacy password, used as a salt when hashing your identity
* Scan an Ethereum address, used as the owner of the minted certificate
* Scan a Digital Green Certificate
* Click the ```Get Attestation``` button
* Click the ```Convert``` button
* Acknowledge the Digital Green Certificate elements on the device
* Save the generated receipt that can be provided to the smart contract


## Building the application

This application is a sample [React](https://reactjs.org/) project that can be built with ```yarn build```

## Testing the application locally

Run ```yarn start```

You'll first need to create a self signed SSL certificate in ```cert/cert.pem``` and ```cert/key.pem``` - this can be done for example with [mkcert](https://github.com/FiloSottile/mkcert)


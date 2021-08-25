require("@nomiclabs/hardhat-waffle");
require("@nomiclabs/hardhat-ethers");
require("@nomiclabs/hardhat-etherscan");
require("hardhat-preprocessor");
const removeConsoleLog = require("hardhat-preprocessor").removeConsoleLog;
const fs = require("fs");
const apiKey = fs.readFileSync(".etherscan-apiKey").toString().trim()

// This is a sample Hardhat task. To learn how to create your own go to
// https://hardhat.org/guides/create-task.html
task("accounts", "Prints the list of accounts", async (taskArgs, hre) => {
  const accounts = await hre.ethers.getSigners();

  for (const account of accounts) {
    console.log(account.address);
  }
});

// You need to export an object to set up your config
// Go to https://hardhat.org/config/ to learn more

/**
 * @type import('hardhat/config').HardhatUserConfig
 */
module.exports = {
	defaultNetwork: "hardhat",
	networks: {
		hardhat: {

		},
		matic: {
			url : "https://rpc-mainnet.matic.network"
		}
	},
  solidity: "0.8.4",
	preprocess: {
    eachLine: removeConsoleLog((hre) => hre.network.name !== 'hardhat' && hre.network.name !== 'localhost'),
  },  
  etherscan: {
  	apiKey: apiKey
  }
};

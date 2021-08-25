import "./index.css";
import React, { Component } from "react";
import ReactDOM from "react-dom";
import TransportWebUSB from "@ledgerhq/hw-transport-webusb";
import QrReader from "react-qr-reader";
import QRCode from "react-qr-code";
const base45 = require('base45');
const zlib = require('zlib');

function foreach<T, A>(
  arr: T[],
  callback: (arg0: T, arg1: number) => Promise<A>
): Promise<A[]> {
  function iterate(index, array, result) {
    if (index >= array.length) {
      return result;
    } else
      return callback(array[index], index).then(function (res) {
        result.push(res);
        return iterate(index + 1, array, result);
      });
  }

  return Promise.resolve().then(() => iterate(0, arr, []));
}

class App extends Component {
  state = {
    info: null,
    error: null,
    ethAddress: null,
    dgc: null,
    attestation: null,
    pwd: null,
    qrInfo: ""
  };

  onReset = async() => {
    this.setState({info: null});
    this.setState({error: null});
    this.setState({ethAddress: null});
    this.setState({dgc: null});
    this.setState({attestation: null});
    this.setState({pwd: null});
    this.setState({qrInfo: ""});
  }

  onAttestation = async() => {
    const transport = await TransportWebUSB.create();
    const buffer = new Buffer(0);
    transport.send(0xe0, 0x04, 0x00, 0x00, buffer)
    .then((response) => {
        response = response.slice(0, response.length - 2);
        this.setState({attestation: response});
        this.setState({info: "Attestation set"});
    }
    );
  }

  onConvert = async() => {
    if (this.state.dgc == null) {
      alert("Missing DGC");
      return;
    }
    if (this.state.attestation == null) {
      alert("Missing attestation");
      return;
    }
    if (this.state.ethAddress == null) {
      alert("Missing ETH address");
      return;
    }
    if (this.state.pwd == null) {
      alert("Missing password");
      return;
    }
    const transport = await TransportWebUSB.create();
    const toSend: Buffer[] = [];
    let offset = 0;
    let response;

    while (offset !== this.state.dgc.length) {
      const maxChunkSize = offset === 0 ? 150 - 2 - 1 - this.state.pwd.length - 20 : 150;
      let chunkSize = offset + maxChunkSize > this.state.dgc.length ? this.state.dgc.length - offset : maxChunkSize;
      const buffer = Buffer.alloc(offset === 0 ? 2 + 1 + this.state.pwd.length + 20 + chunkSize : chunkSize);      
      if (offset === 0) {
        buffer.writeUInt16BE(this.state.dgc.length, 0);
        buffer[2] = this.state.pwd.length;
        Buffer.from(this.state.pwd).copy(buffer, 3);
        this.state.ethAddress.copy(buffer, 3 + this.state.pwd.length);
        this.state.dgc.copy(buffer, 3 + this.state.pwd.length + 20, offset, offset + chunkSize);
      }
      else {
        this.state.dgc.copy(buffer, 0, offset, offset + chunkSize);
      }
      toSend.push(buffer);
      offset += chunkSize;
    }
    foreach(toSend, (data, i) =>
      transport.send(0xe0, 0x02, i === 0 ? 0x00 : 0x80, 0x00, data)
      .then((apduResponse) => {
        response = apduResponse
      })
    ).then(
      () => {
        response = response.slice(0, response.length - 2);
        if (response[0] === 0) {
          this.setState({error: response.slice(1).toString('utf-8')})
        }
        else {
          response = Buffer.concat([response, this.state.attestation]);
          this.setState({qrInfo: response.toString("hex")});
          this.setState({info: "Receipt " + response.toString("hex")});
        }
      },
      (e) => {
        this.setState({error: e});
      }

    );
  }

  handlePwd = e => {
    this.setState({pwd: e.target.value});
  }

  handleScan = data => {
    if (data) {
      var ethAddressPrefix = data.indexOf("0x");
      if (data.substring(0, 4) === "HC1:") {
        this.setState({info: "Scanning DGC"});
        data = data.substring(4);
        var decoded = base45.decode(data);
        var app = this;
        zlib.inflate(decoded, function(err, buf) {
          if (err) {
            app.setState({error: err})
          }
          app.setState({dgc: buf});
          app.setState({info: "DGC scanned"})
        });
      }
      else
      if (ethAddressPrefix >= 0) {
        data = data.substring(ethAddressPrefix + 2, ethAddressPrefix + 2 + 40);
        this.setState({ethAddress: Buffer.from(data, "hex")});
        this.setState({info: "ETH address scanned"});
      }
      else {
        this.setState({error: "Unrecognized QR format"});
      }
    }
  }

  handleError = err => {
    this.setState({error: err})
  }


  render() {
    const { info, error, pwd, qrInfo } = this.state;
    return (
      <div>
          <QrReader
            delay={300}
            facingMode={"environment"}
            onError={this.handleError}
            onScan={this.handleScan}
            style={{ width: '100%' }}
          />              
        <p>
          <input name="pwd" placeholder="Enter password" type="password" value={pwd} onChange={this.handlePwd} />
          <button onClick={this.onAttestation}>
            Get attestation
          </button>
          <button onClick={this.onConvert}>
            Convert
          </button>
          <button onClick={this.onReset}>
            Reset
          </button>
        </p>
        <p>
          {error ? (
            <code className="error">{error.toString()}</code>
          ) : (
            <code className="info">{info}</code>
          )}
        </p>
        <QRCode value={qrInfo}/>
      </div>      
    );
  }
}

ReactDOM.render(<App />, document.getElementById("root"));

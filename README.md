
# QR-Base Hardware Wallet

### Build a truly usable Hardware Wallet(not a toy) for Under $15!

**Welcome to the QR-Base Hardware Wallet Project!**  
The goal of this project is to provide a low-cost, easy-to-build hardware wallet that uses QR codes for secure transactions. With price under $15, we aim to make secure digital asset management accessible to everyone.

[Here is a video introducing how QR-Base Wallet interacts with MetaMask from AirGap Wallet.](https://youtu.be/ATfjCmIVrGg)


## Hardware Supported

The current development is based on the [**ESP32-S3 MCU**](https://www.espressif.com/en/products/socs/esp32-s3), which has enough processing power to handle real-time QR code scanning, making it ideal for a QR-based wallet client.



## Project Progress

### Hardware Progress:


- MCU
  - [x] ESP32-S3
  
- Camera
  - [ ] [OV2640 camera](https://www.arducam.com/ov2640/)
  - [ ] TBD
  
- Screen
  - [ ] TBD
  
    

### Software Progress:
- [ ] **Custom Bootloader**

- [ ] Multi qr-code input

- [ ] Multi qr-code output

- **Ethereum Support** (EVM chains)
  - MetaMask Wallet
    - [ ] Legacy Transactions
    - [ ] Access List Transactions (EIP-2930)
    - [x] Fee Market Transactions (EIP-1559)
    - [ ] Blob Transactions (EIP-4844)
  - [ ] AirGap Wallet
    - [ ] TBD

- **Bitcoin Support**
  - [ ] Bitcoin (SegWit)
    - [ ] AirGap Wallet
    - [ ] BlueWallet
    
    


## How to Build
The project is developed using the PlatformIO IDE based on the Arduino framework. Compiling the software is incredibly simple:

1. Install the [PlatformIO IDE](https://platformio.org/platformio-ide).
2. Open the project in PlatformIO IDE.
3. Click the 'PlatformIO: Build' button to compile the project.



## Special Thanks

A huge thanks to the following open-source projects, which have been instrumental in the development of this project:

- [uBitcoin](https://github.com/micro-bitcoin/uBitcoin.git)
- [tinycbor](https://github.com/intel/tinycbor)
- [airgap-vault](https://github.com/airgap-it/airgap-vault)
  (*While not directly using the library, it provided a lot of inspiration for our work.*)
- [KeystoneHQ](https://github.com/KeystoneHQ)
  (*Thanks for offering invaluable references for QR-based implementations.*)

---

Feel free to contribute, open issues, and join the journey to make secure hardware wallets affordable for everyone!

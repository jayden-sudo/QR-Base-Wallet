
# QR-Base Hardware Wallet

### Build a truly usable Hardware Wallet for Under $20

**Welcome to the QR-Base Hardware Wallet Project!**  
The goal of this project is to provide a low-cost, easy-to-build hardware wallet that uses QR codes for secure transactions. With price under $20, we aim to make secure digital asset management accessible to everyone.

[Here is a video introducing how QR-Base Wallet interacts with MetaMask from AirGap Wallet.](https://www.youtube.com/watch?v=HIKJh0h7QiU&t=55s)

## Hardware Supported

The current development is based on the [**ESP32-S3 MCU**](https://www.espressif.com/en/products/socs/esp32-s3), which has enough processing power to handle real-time QR code scanning, making it ideal for a QR-based wallet client.



- MCU

  | MCU      | Support | Test |
  | -------- | ------- | ---- |
  | ESP32-S3 | ✅       | ✅    |
  | ESP32-S2 | ✅       |      |

  

- Camera

  | Camera | Support | Test |
  | ------ | ------- | ---- |
  | OV2640 | ✅       | ✅    |
  | OV3660 | ✅       |      |
  | GC0308 |         |      |

  

- Screen

  | Screen | Support | Test |
  | ------ | ------- | ---- |
  | TBD    |         |      |

  

## Project Progress

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

The project is developed using the VSCode IDE based on the ESP-IDF framework. Compiling the software is incredibly simple:

1. Install the [ESP-IDF (Espressif IoT Development Framework)](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html#ide).
2. Open the project in VSCode IDE.
3. Click the 'ESP-IDF: Build' button to compile the project.

## Thanks

A huge thanks to the following open-source projects, which have been instrumental in the development of this project:

[uBitcoin](https://github.com/micro-bitcoin/uBitcoin.git) [LVGL](https://github.com/lvgl/lvgl) [tinycbor](https://github.com/intel/tinycbor) [airgap-vault](https://github.com/airgap-it/airgap-vault) [KeystoneHQ](https://github.com/KeystoneHQ) [esp32-camera](https://github.com/espressif/esp32-camera) [esp-code-scanner](https://github.com/espressif/)

---

Feel free to contribute, open issues, and join the journey to make secure hardware wallets affordable for everyone!

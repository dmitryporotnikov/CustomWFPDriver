# Customized WFP Callout Driver for Azure VM Communication Inspection/Blocking

This project is a customized implementation of a Windows Filtering Platform driver, originally based on the [WFPStarterKit](https://github.com/JaredWright/WFPStarterKit/tree/master) by Jared Wright. 
It's designed as a training tool to inspect/filter outbound TCP traffic, specifically to troubleshoot issues related to VM communication with Azure platform resources. 
The customization involves modifying the `ClassifyFn` function to block packets destined for IP address 168.63.129.16, which is a virtual public IP address used by Azure for various platform-level operations.

## Background

IP address 168.63.129.16 is utilized within Azure to facilitate a communication channel to platform resources. 
It is a virtual public IP address that enables operations crucial for Azure services, as documented in Microsoft's [official documentation](https://learn.microsoft.com/en-us/azure/virtual-network/what-is-ip-address-168-63-129-16). 
This project aims to provide a hands-on example for learning and troubleshooting packet filtering in the context of Azure VMs and network security.

## Features

- **Selective Packet Blocking**: The driver blocks outbound packets destined for the Azure platform's virtual public IP address (168.63.129.16).
- **Outbound Traffic Inspection**: It logs the TCP 4-tuple of outbound packets, aiding in understanding traffic flow and for educational purposes.
- **Extendable for Educational Use**: While focused on a specific Azure IP, can be used as a base for further exploration and customization of packet filtering using WFP.

## Getting Started

### Prerequisites

- Windows 10 or later with WDK
- Visual Studio 2019 or later with C++ development tools

### Installation

1. Clone this repository.
2. Open the solution in Visual Studio and build the project to generate the driver file.
3. Install the driver using `sc create` or through the provided PowerShell script.

### Usage

This driver automatically starts inspecting outbound TCP traffic upon installation. It specifically blocks packets destined for 168.63.129.16 and logs other outbound traffic details for educational and troubleshooting purposes.

## Contributing

This project is intended as an educational tool, and contributions are welcome to enhance its functionality or documentation for learning purposes.

## Acknowledgments

- Jared Wright, for the original WFPStarterKit which served as the foundation for this project.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

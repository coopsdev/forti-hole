# forti-hole

## Build and Installation Instructions

### 👯 Step 1: Clone the Repository

First, clone the **forti-hole** repository:

```bash
git clone https://github.com/coopsdev/forti-hole.git
cd forti-hole
```

### 🛠 Step 2: Build the Program

Run the `build.sh` script to compile the program and create the `config.yaml` file.

```bash
./bin/build.sh
```

This script will check if `config.yaml` exists and create it from `config.example.yaml` if necessary.

### 👷 Step 3: Configure `config.yaml`

Before running the program, make sure you properly configure the `config.yaml` file located in the build directory. Adjust the relevant settings to match your environment (e.g., API keys, IP addresses, and certificate paths).

### 🚀 Step 4: Run the Program

Once `config.yaml` is properly configured, you can run the program:

```bash
./bin/run.sh
```

This script will build the program if necessary and execute it using the configurations in `config.yaml`.

### 🔃 Step 5: Install as a Systemd Service (Linux Only)

To install **forti-hole** as a systemd service on Linux with a default 5am everyday run-timer, use the `install.sh` script:

```bash
./bin/install.sh
```

This script will install the program as a service, allowing it to run automatically on boot and at 5am (easily customizable) everyday.

### 🪬 Step 6: Update the Program

To update the repository, remove the build directory, and rebuild the program, run the `update.sh` script:

```bash
./bin/update.sh
```

This script pulls the latest changes from the repository, removes the existing build directory, and triggers a fresh build.

## Overview

**forti-hole** is a cutting-edge DNS Threat Feed management tool, engineered for FortiGate users who need to manage DNS blocklists at an unprecedented scale. Inspired by Pi-hole, forti-hole is designed to tackle the challenges of handling massive domain lists, overcoming issues like buffer overflow and network traffic interruptions that plague FortiGate’s non-API managed threat feeds.

**forti-hole** delivers a tested payload of 16+ files, encompassing over 2 million unique DNS entries, without causing any network interruption or delay to users. It directly addresses the challenges associated with managing 'pull' update method threat feeds on FortiGate, where a simultaneous refresh can overwhelm the buffers in an alarmingly short timespan, forcing a manual reset of the FortiGate and impacting all network users.

The default configuration includes a blocklist 'recipe' with these domains, organized into a 2-level security system, and scores up to 94+% on the [d3ward ad-block test](https://d3ward.github.io/toolz/adblock).

While forti-hole isn't a Pi-hole replacement, it excels in simplicity and scale. If you need YAML-configured control and the capability to efficiently manage vast DNS blocklists on your FortiGate, **forti-hole** is your solution.

## Key Features

- **Efficient DNS Blocklist Management**: Manage blocklists containing over 2 million unique DNS entries across multiple files without causing network interruptions.
- **YAML Configuration**: Simplified management through a YAML config file, allowing for easy customization and deployment.
- **Automated Security Level Assignment**: Future releases will automate DNS filter assignments to reduce manual management.
- **[forti-api](https://github.com/coopsdev/forti-api) Integration**: Provides deep control over your FortiGate setup via a type-safe, nlohmann-based syntax.
- **Buffer Overflow Protection**: Prevents buffer overflow issues caused by dynamic feed updates in FortiGate, specifically addressing issues with the 'pull' update method that can force a manual reset.
- **Future forti2ban Integration**: Planned integration with [forti2ban](https://github.com/coopsdev/forti2ban) for synchronized category management.

## Why forti-hole?

FortiGate devices often struggle with large DNS blocklists due to memory limitations and lack of overflow protections. **forti-hole** manages threat feeds via the API, preventing the buffer overflow issues that can occur when refreshing massive domain lists through the 'pull' update method. This tool is for network security professionals who need a reliable way to manage DNS blocklists on FortiGate devices without risking network downtime.

With upcoming features like automatic security level management and forti2ban integration, **forti-hole** is set to become an indispensable tool for anyone serious about network security on FortiGate.

## Default 'Advanced' Settings - Results from [d3ward.github.io/toolz/adblock](https://d3ward.github.io/toolz/adblock)

![Ad-block Test Results](https://cooperhlarson.com/media/Screenshot%202024-08-31%20at%205.07.39%C3%A2%C2%AFPM.png)

## Contributing

Contributions are welcome! As **forti-hole** transitions from Python to a more robust C++ implementation, there are many opportunities to get involved. **Owning a FortiGate firewall is required** for testing, as I cannot provide access to my personal FortiGate 60F. The project is licensed under the MIT License, so your contributions will benefit the broader community.

## Status

**forti-hole** is in active production. Many features may be broken as the project undergoes a significant refactor from its original Python version. However, the core functionality is rapidly improving, with a major release planned for September 10, 2024, including a multi-platform precompiled executable with configuration and `.env` files.

Stay tuned for updates and help make **forti-hole** the go-to tool for managing DNS blocklists on FortiGate!

# Sample forti-hole output

```markdown
Starting blocklist scraping process...

Processing config file...

Scraping blocklists...

Fetching URL: https://cdn.jsdelivr.net/gh/hagezi/dns-blocklists@latest/adblock/pro.plus.txt
Fetching URL: https://cdn.jsdelivr.net/gh/hagezi/dns-blocklists@latest/adblock/tif.txt
Fetching URL: https://cdn.jsdelivr.net/gh/hagezi/dns-blocklists@latest/adblock/ultimate.txt
Fetching URL: https://raw.githubusercontent.com/blocklistproject/Lists/master/adguard/ads-ags.txt
Fetching URL: https://raw.githubusercontent.com/blocklistproject/Lists/master/adguard/tracking-ags.txt
Fetching URL: https://raw.githubusercontent.com/blocklistproject/Lists/master/adguard/malware-ags.txt
Fetching URL: https://raw.githubusercontent.com/blocklistproject/Lists/master/adguard/phishing-ags.txt
Fetching URL: https://raw.githubusercontent.com/blocklistproject/Lists/master/adguard/ransomware-ags.txt
Fetching URL: https://raw.githubusercontent.com/blocklistproject/Lists/master/adguard/scam-ags.txt
Fetching URL: https://raw.githubusercontent.com/blocklistproject/Lists/master/adguard/abuse-ags.txt
Fetching URL: https://raw.githubusercontent.com/blocklistproject/Lists/master/adguard/fraud-ags.txt
Fetching URL: https://raw.githubusercontent.com/StevenBlack/hosts/master/hosts/default.txt

Parsing response data...

Parsing response data: https://cdn.jsdelivr.net/gh/hagezi/dns-blocklists@latest/adblock/pro.plus.txt
Parsing response data: https://cdn.jsdelivr.net/gh/hagezi/dns-blocklists@latest/adblock/tif.txt
Parsing response data: https://cdn.jsdelivr.net/gh/hagezi/dns-blocklists@latest/adblock/ultimate.txt
Parsing response data: https://raw.githubusercontent.com/blocklistproject/Lists/master/adguard/ads-ags.txt
Parsing response data: https://raw.githubusercontent.com/blocklistproject/Lists/master/adguard/tracking-ags.txt
Parsing response data: https://raw.githubusercontent.com/blocklistproject/Lists/master/adguard/malware-ags.txt
Parsing response data: https://raw.githubusercontent.com/blocklistproject/Lists/master/adguard/phishing-ags.txt
Parsing response data: https://raw.githubusercontent.com/blocklistproject/Lists/master/adguard/ransomware-ags.txt
Parsing response data: https://raw.githubusercontent.com/blocklistproject/Lists/master/adguard/scam-ags.txt
Parsing response data: https://raw.githubusercontent.com/blocklistproject/Lists/master/adguard/abuse-ags.txt
Parsing response data: https://raw.githubusercontent.com/blocklistproject/Lists/master/adguard/fraud-ags.txt
Parsing response data: https://raw.githubusercontent.com/StevenBlack/hosts/master/hosts/default.txt

Blocklist processing successfully completed...

Consolidating data...
Gathering threat feed information...
Creating threat feed containers...
Updating firewall policies...
Constructing files and pushing threat feeds...

Security Level 0: { Files: 14, LPF: 128423 }

Built file: forti-hole_security_level-0_part-1 with 128424 lines.
Successfully uploaded to FortiGate: forti-hole_security_level-0_part-1

Built file: forti-hole_security_level-0_part-2 with 128424 lines.
Successfully uploaded to FortiGate: forti-hole_security_level-0_part-2

Built file: forti-hole_security_level-0_part-3 with 128424 lines.
Successfully uploaded to FortiGate: forti-hole_security_level-0_part-3

Built file: forti-hole_security_level-0_part-4 with 128424 lines.
Successfully uploaded to FortiGate: forti-hole_security_level-0_part-4

Built file: forti-hole_security_level-0_part-5 with 128424 lines.
Successfully uploaded to FortiGate: forti-hole_security_level-0_part-5

Built file: forti-hole_security_level-0_part-6 with 128424 lines.
Successfully uploaded to FortiGate: forti-hole_security_level-0_part-6

Built file: forti-hole_security_level-0_part-7 with 128424 lines.
Successfully uploaded to FortiGate: forti-hole_security_level-0_part-7

Built file: forti-hole_security_level-0_part-8 with 128424 lines.
Successfully uploaded to FortiGate: forti-hole_security_level-0_part-8

Built file: forti-hole_security_level-0_part-9 with 128424 lines.
Successfully uploaded to FortiGate: forti-hole_security_level-0_part-9

Built file: forti-hole_security_level-0_part-10 with 128424 lines.
Successfully uploaded to FortiGate: forti-hole_security_level-0_part-10

Built file: forti-hole_security_level-0_part-11 with 128424 lines.
Successfully uploaded to FortiGate: forti-hole_security_level-0_part-11

Built file: forti-hole_security_level-0_part-12 with 128424 lines.
Successfully uploaded to FortiGate: forti-hole_security_level-0_part-12

Built file: forti-hole_security_level-0_part-13 with 128424 lines.
Successfully uploaded to FortiGate: forti-hole_security_level-0_part-13

Built file: forti-hole_security_level-0_part-14 with 128423 lines.
Successfully uploaded to FortiGate: forti-hole_security_level-0_part-14
Security Level 1: { Files: 2, LPF: 100084 }

Built file: forti-hole_security_level-1_part-1 with 100085 lines.
Successfully uploaded to FortiGate: forti-hole_security_level-1_part-1

Built file: forti-hole_security_level-1_part-2 with 100084 lines.
Successfully uploaded to FortiGate: forti-hole_security_level-1_part-2

forti-hole finished successfully in 33s
```

---

# forti-hole

## Overview

**forti-hole** is a robust DNS Threat Feed management tool inspired by Pi-hole, designed specifically for FortiGate users who need to manage DNS blocklists exceeding 1 million domains. While forti-hole is not a Pi-hole replacement, it addresses key limitations in FortiGate's non-API managed threat feeds—particularly the issues related to buffer overflow and network traffic interruptions when managing large-scale domain lists.

If you're looking for a solution with a flashy dashboard and extensive statistics, Pi-hole is still your best bet. However, for those who need streamlined control via a YAML config file and the ability to handle massive DNS blocklists efficiently, **forti-hole** is the tool for you.

## Key Features

- **Efficient DNS Blocklist Management**: Capable of managing blocklists with over 1 million domains without causing buffer overflows or network interruptions.
- **YAML Configuration**: Simplified management through a YAML config file, allowing for easy customization and deployment.
- **Automated Security Level Assignment**: Future releases will include automatic assignment of DNS filters to the appropriate security policies, significantly reducing manual management time.
- **[forti-api](https://github.com/coopsdev/forti-api) Integration**: Utilizes a powerful, rapidly expanding API wrapper for FortiGate, providing deep control over your FortiGate setup via a type-safe, nlohmann-based syntax.
- **Buffer Overflow Protection**: Directly addresses the buffer overflow issues caused by dynamic feed updates in FortiGate, ensuring stable and reliable operation even with large threat feeds.
- **Future forti2ban Integration**: Planned integration with [forti2ban](https://github.com/coopsdev/forti2ban) to synchronize categories, though both tools will continue to run separately via cron or systemd.

## Why forti-hole?

FortiGate devices often struggle with managing large DNS blocklists due to memory limitations and lack of overflow protections. **forti-hole** solves this by managing the threat feeds via API, preventing the buffer overflow issues that occur when attempting to refresh massive domain lists. This tool is designed to be a direct solution for network security professionals who require a more reliable method of managing DNS blocklists on their FortiGate devices.

With future features like automatic security level management and forti2ban integration, **forti-hole** is set to become an indispensable tool for anyone serious about network security on FortiGate.

## Default 'Advanced' Settings - results from [d3ward.github.io/toolz/adblock](https://d3ward.github.io/toolz/adblock)
<img src="https://cooperhlarson.com/media/Screenshot%202024-08-31%20at%205.07.39%C3%A2%C2%AFPM.png" />

## Contributing

* Contributors are welcome! As **forti-hole** is refactored from its initial Python version into a more robust C++ implementation, there are many opportunities to get involved.
* **Owning a FortiGate firewall is required**, my testing environment is my own FortiGate 60f which protects my home lan, access to this will not be provided for obvious reasons. The project is licensed under the MIT License, ensuring that your contributions will benefit the broader community.

## Status

**forti-hole** is currently in active production. Many features may be broken as the project undergoes a significant refactor from its original Python version. However, the core functionality is being rapidly improved, with a major release planned in the next few days.

Stay tuned for updates and join us in making **forti-hole** the go-to tool for managing DNS blocklists on FortiGate!

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

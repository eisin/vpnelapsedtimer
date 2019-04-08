@echo off
if "%1" == "stats" (
 echo Cisco AnyConnect Secure Mobility Client ^(version 4.5.03040^) .
 echo.
 echo Copyright ^(c^) 2004 - 2017 Cisco Systems, Inc.  All Rights Reserved.
 echo.
 echo.
 echo   ^>^> state: Connected
 echo   ^>^> state: Connected
 echo   ^>^> registered with local VPN subsystem.
 echo   ^>^> state: Connected
 echo   ^>^> notice: Connected to example.com.
 echo VPN^>
 echo.
 echo [ Connection Information ]
 echo.
 echo     Tunnel Mode ^(IPv4^):        Tunnel All Traffic
 echo     Tunnel Mode ^(IPv6^):        Drop All Traffic
 echo     Dynamic Tunnel Exclusion:  None
 echo     Time Connected:            00:00:09
 echo.
 echo [ Address Information ]
 echo.
 echo     Client Address ^(IPv4^):     000.000.000.000
 echo     Client Address ^(IPv6^):     Not Available
 echo     Server Address:            000.000.000.000
 echo.
 echo [ Bytes ]
 echo.
 echo     Bytes Sent:                21451
 echo     Bytes Received:            64065
 echo.
 echo [ Frames ]
 echo.
 echo     Packets Sent:              186
 echo     Packets Received:          101
 echo.
 echo [ Control Frames ]
 echo.
 echo     Control Packets Sent:      13
 echo     Control Packets Received:  29
 echo.
 echo [ Client Management ]
 echo.
 echo     Administrative Domain:     Undefined
 echo     Profile Name:              Not Available
 echo.
 echo [ Transport Information ]
 echo.
 echo     Protocol:                  IKEv2/IPsec NAT-T
 echo     Cipher:                    AES_128_SHA1
 echo     Compression:               None
 echo     Proxy Address:             Not Available
 echo     FIPS Mode:                 Disabled
 echo.
 echo [ Feature Configuration ]
 echo.
 echo     FIPS Mode:                 Disabled
 echo     Trusted Network Detection: Disabled
 echo     Always On:                 Disabled
 echo.
 echo [ Secure Mobility Solution ]
 echo.
 echo     Network Status:            Network Access: Restricted
 echo     Appliance:                 Not Available
 echo     SMS Status:                Unconfirmed
 echo.
 echo [ Secured Routes ^(IPv4^) ]
 echo.
 echo     Network                                Subnet
 echo     0.0.0.0                                0
 echo.
 echo [ Secured Routes ^(IPv6^) ]
 echo.
 echo     Network                                Subnet
 echo.
 echo VPN^>
 echo.
 echo.
) else (
  echo Usage: with argument "stats"
)
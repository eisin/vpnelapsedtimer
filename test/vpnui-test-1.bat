@echo off
if "%1" == "stats" (
  echo [Tunnel information]
  echo.
  echo Time Connected: 01:17:33
  echo Client Address: 192.168.23.45
  echo Server Address: 209.165.200.224
  echo.
  echo [Tunnel Details]
  echo.
  echo Tunneling Mode: All traffic
  echo Protocol: DTLS
  echo Protocol Cipher: RSA_AES_256_SHA1
  echo Protocol Compression: None
  echo.
  echo [Data Transfer]
  echo.
  echo Bytes ^(sent/received^): 1950410/23861719
  echo Packets ^(sent/received^): 18346/28851
  echo Bypassed ^(outbound/inbound^): 0/0
  echo Discarded ^(outbound/inbound^): 0/0
  echo.
  echo [Secure Routes]
  echo.
  echo Network Subnet
  echo 0.0.0.0 0.0.0.0
  echo VPN^>
) else (
  echo "Usage %0 stats"
)

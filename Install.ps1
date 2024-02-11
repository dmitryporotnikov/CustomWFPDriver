# This script must be run as Administrator
# Run in the folder with driver binaries
# Reboot the OS after installation to see the changes

if (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    throw "Please run the PowerShell as an Administrator."
}

# Set variables for the service and driver
$driverName = "WFPDriver.sys"
$serviceName = "WFPService"
$certPath = ".\WFPDriver.cer"
$storeLocation = "Cert:\LocalMachine\Root"

# Install the certificate to Trusted Root
if (Test-Path $certPath) {
    try {
        Import-Certificate -FilePath $certPath -CertStoreLocation $storeLocation -ErrorAction Stop
        Write-Host "Certificate installed successfully."
    } catch {
        Write-Error "Failed to install the certificate. Error: $_"
    }
} else {
    Write-Error "Certificate file not found at the path: $certPath"
}

# Install Driver
$driverPath = Join-Path -Path (Get-Location) -ChildPath $driverName
Invoke-Expression -Command "sc.exe create `$serviceName binPath= `"$driverPath`" type= kernel"

# Set the service to start automatically on boot
Invoke-Expression -Command "sc.exe config `$serviceName start= auto"

Invoke-Expression -Command "sc.exe start `$serviceName"

# Enable test signing for driver development
#Start-Process -FilePath "bcdedit" -ArgumentList "/set", "testsigning", "on" -Verb RunAs

#Reboot

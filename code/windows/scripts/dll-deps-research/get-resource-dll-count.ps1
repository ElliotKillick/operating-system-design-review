param(
    [string]$directoryPath
)

# Read a file into a byte array
function Read-FileBytes {
    param([string]$filePath, [int]$length = 1024)
    $buffer = New-Object byte[] $length
    $fs = [System.IO.File]::OpenRead($filePath)
    $fs.Read($buffer, 0, $buffer.Length) | Out-Null
    $fs.Close()
    return $buffer
}

# Verify if file has a valid MZ (DOS) header
function Verify-MZHeader {
    param([byte[]]$buffer)
    return [System.Text.Encoding]::ASCII.GetString($buffer, 0, 2) -eq "MZ"
}

# Get PE header offset from DOS header
function Get-PEHeaderOffset {
    param([byte[]]$buffer)
    return [System.BitConverter]::ToInt32($buffer, 0x3C)
}

# Verify if file has a valid PE signature
function Verify-PESignature {
    param([byte[]]$buffer, [int]$peHeaderOffset)
    return [System.Text.Encoding]::ASCII.GetString($buffer, $peHeaderOffset, 4) -eq "PE`0`0"
}

# Extract the AddressOfEntryPoint
function Get-EntryPoint {
    param([byte[]]$buffer, [int]$peHeaderOffset)
    $entryPointOffset = $peHeaderOffset + 0x28
    return [System.BitConverter]::ToInt32($buffer, $entryPointOffset)
}

# Check if a DLL is a resource-only DLL
# Resource-only DLLs never have an entry point: https://learn.microsoft.com/en-us/cpp/build/reference/noentry-no-entry-point
# In my testing, it is possible to build a code DLL with the /NOENTRY option, which could create a false-positive
# Although, MSVC does block a /NOENTRY DLL from importing any symbols from other DLLs, which fails with a link error:
# dll-test.obj : error LNK2019: unresolved external symbol __imp_puts referenced in function DummyExport
# We could also check for a .rsrc section, but that is still only another heuristic since there does not appear to exist a distinct flag for telling a code DLL apart from a resource-only DLL
# The best we could do is copy the exact heuristic that the Windows loader uses to detect a resource-only DLL on an ambiguous call to LoadLibrary
# For our practical purposes, I think simply checking for an entry point is good enough
function Is-ResourceOnlyDLL {
    param([string]$filePath)

    $buffer = Read-FileBytes -filePath $filePath

    # Verify MZ header
    if (-not (Verify-MZHeader -buffer $buffer)) {
        return $false
    }

    $peHeaderOffset = Get-PEHeaderOffset -buffer $buffer

    # Verify PE signature
    if (-not (Verify-PESignature -buffer $buffer -peHeaderOffset $peHeaderOffset)) {
        return $false
    }

    $entryPoint = Get-EntryPoint -buffer $buffer -peHeaderOffset $peHeaderOffset

    return $entryPoint -eq 0
}

# Scan all DLLs in the directory (not including subdirectories)
function Scan-Directory {
    param([string]$directoryPath)

    $dllFiles = Get-ChildItem -Path $directoryPath -Filter "*.dll"
    $resourceOnlyDllCount = 0

    foreach ($dll in $dllFiles) {
        if (Is-ResourceOnlyDLL -filePath $dll.FullName) {
            $resourceOnlyDllCount++
        }
    }

    Write-Host "Code DLLs: $($dllFiles.Count - $resourceOnlyDllCount)"
    Write-Host "Resource-only DLLs: $resourceOnlyDllCount"
	Write-Host "Total DLLs scanned: $($dllFiles.Count)"
}

# Run the scan
Scan-Directory -directoryPath $directoryPath

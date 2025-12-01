Get-ChildItem -File | Where-Object { $_.Extension -in '.exe', '.dll' } | ForEach-Object {
    $output = dumpbin /dependents $_
    if ($output | Select-String -Pattern "^  Image has the following delay load dependencies:") {
        Write-Host "$($_.Name)"
    }
}

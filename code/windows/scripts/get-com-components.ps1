# Get registered COM components
# Each COM class is a component
#
# Run as administrator to ensure all results are collected.
# Make sure your PowerShell window is maximized when you run this command so entires are not split between lines even though the output is to a file (apparently isatty() does not exist on Windows?)
#
# PowerShell sucks, so clean up the output before commiting to Git repo:
# 1. dos2unix com-components.txt
#   - UTF-16LE with BOM -> UTF-8 and CRLF -> LF
# 2. sed -i 's/[[:space:]]*$//' com-components.txt
#   - Remove trailing spaces before each end of line
#   - Believe it or not, PowerShell includes a box of whitespace the size of your PowerShell window in the output. As a result, file size grows in large linear increments based on window size.
# 3. Manually remove any extraneous newlines puts at the top and bottom of the output

# We use HKCR to include the system-wide and current user components
Get-ChildItem Registry::HKEY_CLASSES_ROOT\CLSID | Get-ItemProperty | Select-Object PsChildName, "(default)" | Sort-Object "(default)" | Out-File -FilePath "com-components.txt"

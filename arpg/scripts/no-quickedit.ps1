<#
.SYNOPSIS
  Turns off QuickEdit (mouse text selection) for ONE process's console window.
.DESCRIPTION
  With QuickEdit on, a click into a server console starts a text selection and Windows pauses the program at its next
  console write - the world server freezes (seen 2026-11-22: 28 minutes, tests hung on SOAP). This changes only that
  window's mode, nothing global. restart-world.ps1 runs it (hidden) for authserver and worldserver after starting them.
.EXAMPLE
  pwsh -NoProfile -File arpg\scripts\no-quickedit.ps1 -ProcessId 12345
#>
param([Parameter(Mandatory)][int]$ProcessId)
$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class ConsoleQuickEdit {
  [DllImport("kernel32.dll", SetLastError = true)] static extern bool FreeConsole();
  [DllImport("kernel32.dll", SetLastError = true)] static extern bool AttachConsole(uint pid);
  [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
  static extern IntPtr CreateFile(string name, uint access, uint share, IntPtr sec, uint disp, uint flags, IntPtr tmpl);
  [DllImport("kernel32.dll", SetLastError = true)] static extern bool GetConsoleMode(IntPtr h, out uint mode);
  [DllImport("kernel32.dll", SetLastError = true)] static extern bool SetConsoleMode(IntPtr h, uint mode);
  [DllImport("kernel32.dll", SetLastError = true)] static extern bool CloseHandle(IntPtr h);
  public static string Disable(uint pid) {
    FreeConsole();
    if (!AttachConsole(pid)) return "attach failed (" + Marshal.GetLastWin32Error() + ")";
    try {
      IntPtr h = CreateFile("CONIN$", 0xC0000000, 3, IntPtr.Zero, 3, 0, IntPtr.Zero);   // GENERIC_READ|WRITE, share R|W, OPEN_EXISTING
      if (h == new IntPtr(-1)) return "CONIN$ failed (" + Marshal.GetLastWin32Error() + ")";
      uint mode;
      GetConsoleMode(h, out mode);
      uint newMode = (mode & ~0x40u) | 0x80u;   // clear ENABLE_QUICK_EDIT_MODE, keep ENABLE_EXTENDED_FLAGS
      bool ok = SetConsoleMode(h, newMode);
      CloseHandle(h);
      return ok ? "quickedit off (mode " + mode + " -> " + newMode + ")" : "SetConsoleMode failed";
    } finally { FreeConsole(); }
  }
}
'@
[ConsoleQuickEdit]::Disable([uint32]$ProcessId) | Out-File (Join-Path $env:TEMP "no-quickedit-$ProcessId.txt")

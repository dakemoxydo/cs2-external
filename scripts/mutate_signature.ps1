# mutate_signature.ps1
# Post-build signature mutation: writes 256 random bytes into the .mutate section
# and patches the PE TimeDateStamp so every build has a unique hash.
# Usage: powershell -File mutate_signature.ps1 -ExePath "path\to\cs2overlay.exe"

param([string]$ExePath)

if (-not $ExePath -or -not (Test-Path $ExePath)) {
    Write-Host "[MUTATE] ERROR: .exe not found: $ExePath"
    exit 1
}

$bytes  = [System.IO.File]::ReadAllBytes($ExePath)
$rng    = [System.Security.Cryptography.RandomNumberGenerator]::Create()

# ── 1. Patch TimeDateStamp in COFF header ──────────────────────────────────
# PE header: 'MZ' at 0, e_lfanew at 0x3C (int32 LE)
$e_lfanew = [BitConverter]::ToInt32($bytes, 0x3C)
# COFF header TimeDateStamp is at e_lfanew + 4 + 4 = e_lfanew + 8
$coffTimestampOffset = $e_lfanew + 8
$randTs = [byte[]]::new(4)
$rng.GetBytes($randTs)
[Array]::Copy($randTs, 0, $bytes, $coffTimestampOffset, 4)

# ── 2. Find .mutate section and fill with random bytes ────────────────────
# Section table starts at: e_lfanew + 4 (sig) + 20 (COFF header) + SizeOfOptionalHeader
$sizeOfOptHeader = [BitConverter]::ToInt16($bytes, $e_lfanew + 4 + 16)
$numberOfSections = [BitConverter]::ToInt16($bytes, $e_lfanew + 4 + 2)
$sectionTableOffset = $e_lfanew + 4 + 20 + $sizeOfOptHeader

$targetName = [System.Text.Encoding]::ASCII.GetBytes(".mutate")

for ($i = 0; $i -lt $numberOfSections; $i++) {
    $sOff  = $sectionTableOffset + $i * 40
    $sName = $bytes[$sOff..($sOff+7)]

    # Compare 7 bytes of section name
    $match = $true
    for ($c = 0; $c -lt $targetName.Length; $c++) {
        if ($sName[$c] -ne $targetName[$c]) { $match = $false; break }
    }
    if (-not $match) { continue }

    $rawOffset = [BitConverter]::ToInt32($bytes, $sOff + 20)
    $rawSize   = [BitConverter]::ToInt32($bytes, $sOff + 16)

    if ($rawOffset -gt 0 -and $rawSize -gt 0) {
        $randData = [byte[]]::new($rawSize)
        $rng.GetBytes($randData)
        [Array]::Copy($randData, 0, $bytes, $rawOffset, $rawSize)
        Write-Host "[MUTATE] Filled .mutate section ($rawSize bytes) at offset 0x$($rawOffset.ToString('X'))"
    }
    break
}

# ── 3. No .mutate section? Append seed to PE overlay ──────────────────────
# (Bytes appended past the last section — ignored by loader, changes the hash)
$seed = [byte[]]::new(256)
$rng.GetBytes($seed)
$allBytes = $bytes + $seed
Write-Host "[MUTATE] Appended 256-byte random seed to PE overlay"

# ── 4. Write result ───────────────────────────────────────────────────────
[System.IO.File]::WriteAllBytes($ExePath, $allBytes)
Write-Host "[MUTATE] Done! New SHA256: $((Get-FileHash $ExePath -Algorithm SHA256).Hash)"

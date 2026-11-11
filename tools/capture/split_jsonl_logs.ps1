param(
    [switch]$Join,
    [string]$RepositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
)

$ErrorActionPreference = 'Stop'
$PartLimit = 95000000
$ManifestPath = Join-Path $RepositoryRoot 'data\captures\split-log-manifest.json'
$BufferSize = 1MB

function Find-NextLineBoundary([System.IO.FileStream]$Stream, [long]$Position) {
    $Stream.Position = $Position
    $buffer = [byte[]]::new(65536)
    while (($read = $Stream.Read($buffer, 0, $buffer.Length)) -gt 0) {
        $index = [Array]::IndexOf($buffer, [byte]10, 0, $read)
        if ($index -ge 0) { return ($Stream.Position - $read + $index + 1) }
    }
    return $Stream.Length
}

function Find-PreviousLineBoundary([System.IO.FileStream]$Stream, [long]$Position, [long]$Minimum) {
    $buffer = [byte[]]::new(65536)
    $cursor = $Position
    while ($cursor -gt $Minimum) {
        $readLength = [int][Math]::Min($buffer.Length, $cursor - $Minimum)
        $cursor -= $readLength
        $Stream.Position = $cursor
        $read = $Stream.Read($buffer, 0, $readLength)
        if ($read -gt 0) {
            $index = [Array]::LastIndexOf($buffer, [byte]10, $read - 1)
            if ($index -ge 0) { return ($cursor + $index + 1) }
        }
    }
    return -1L
}

function Get-Sha256([string]$Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Write-Manifest($Manifest) {
    $temp = "$ManifestPath.tmp"
    $json = $Manifest | ConvertTo-Json -Depth 8
    [System.IO.File]::WriteAllText($temp, $json + "`n", [System.Text.UTF8Encoding]::new($false))
    Move-Item -LiteralPath $temp -Destination $ManifestPath -Force
}

if ($Join) {
    if (-not (Test-Path -LiteralPath $ManifestPath)) { throw "Missing manifest: $ManifestPath" }
    $manifest = Get-Content -LiteralPath $ManifestPath -Raw | ConvertFrom-Json
    $restored = 0
    foreach ($entry in $manifest.files) {
        $destination = Join-Path $RepositoryRoot ($entry.source -replace '/', '\')
        if (Test-Path -LiteralPath $destination) {
            if ((Get-Sha256 $destination) -ne $entry.sha256) { throw "Source path already exists with different content: $destination" }
            Write-Output "Already restored: $($entry.source)"
            continue
        }
        $temp = "$destination.join-tmp"
        if (Test-Path -LiteralPath $temp) { throw "Temporary output already exists: $temp" }
        $hash = [System.Security.Cryptography.IncrementalHash]::CreateHash([System.Security.Cryptography.HashAlgorithmName]::SHA256)
        $total = [long]0
        try {
            $output = [System.IO.File]::Open($temp, [System.IO.FileMode]::CreateNew, [System.IO.FileAccess]::Write, [System.IO.FileShare]::None)
            try {
                foreach ($part in $entry.parts) {
                    $partPath = Join-Path $RepositoryRoot ($part.path -replace '/', '\')
                    if (-not (Test-Path -LiteralPath $partPath)) { throw "Missing part: $partPath" }
                    if ((Get-Item -LiteralPath $partPath).Length -ne [long]$part.bytes -or (Get-Sha256 $partPath) -ne $part.sha256) {
                        throw "Part size/hash mismatch: $partPath"
                    }
                    $input = [System.IO.File]::OpenRead($partPath)
                    try {
                        $buffer = [byte[]]::new($BufferSize)
                        while (($read = $input.Read($buffer, 0, $buffer.Length)) -gt 0) {
                            $output.Write($buffer, 0, $read)
                            $hash.AppendData($buffer, 0, $read)
                            $total += $read
                        }
                    } finally { $input.Dispose() }
                }
            } finally { $output.Dispose() }
            $digest = [Convert]::ToHexString($hash.GetHashAndReset()).ToLowerInvariant()
            if ($total -ne [long]$entry.bytes -or $digest -ne $entry.sha256) { throw "Reassembled file size/hash mismatch: $($entry.source)" }
            Move-Item -LiteralPath $temp -Destination $destination
            $restored++
            Write-Output "Restored: $($entry.source)"
        } catch {
            if (Test-Path -LiteralPath $temp) { Remove-Item -LiteralPath $temp -Force }
            throw
        } finally { $hash.Dispose() }
    }
    Write-Output "Restored $restored source log(s). Parts were retained."
    exit 0
}

$scanRoots = @('data', 'bin') | ForEach-Object { Join-Path $RepositoryRoot $_ } | Where-Object { Test-Path -LiteralPath $_ }
$candidates = @($scanRoots | ForEach-Object {
    Get-ChildItem -LiteralPath $_ -Recurse -File -Filter '*.jsonl' -ErrorAction Stop
} | Where-Object { $_.Length -ge 100000000 } | Sort-Object FullName)

if ($candidates.Count -eq 0) { Write-Output 'No JSONL logs at or above 100,000,000 bytes.'; exit 0 }
$recent = @($candidates | Where-Object { $_.LastWriteTimeUtc -gt [DateTime]::UtcNow.AddHours(-1) })
if ($recent.Count -gt 0) { throw "Refusing to split recently modified logs: $($recent[0].FullName)" }

if (Test-Path -LiteralPath $ManifestPath) {
    $manifest = Get-Content -LiteralPath $ManifestPath -Raw | ConvertFrom-Json -AsHashtable
    if ($manifest.schema -ne 1 -or $manifest.part_limit_bytes -ne $PartLimit) { throw "Unsupported or mismatched split manifest: $ManifestPath" }
} else {
    $manifest = [ordered]@{ schema = 1; part_limit_bytes = $PartLimit; line_boundary = 'LF'; files = @() }
    Write-Manifest $manifest
}
$alreadyRecorded = @{}; foreach ($entry in $manifest.files) { $alreadyRecorded[$entry.source] = $entry }
$index = 0
foreach ($file in $candidates) {
    $source = $file.FullName
    $relative = [System.IO.Path]::GetRelativePath($RepositoryRoot, $source).Replace('\', '/')
    $index++
    if ($alreadyRecorded.ContainsKey($relative)) {
        $record = $alreadyRecorded[$relative]
        if ((Get-Item -LiteralPath $source).Length -ne [long]$record.bytes -or (Get-Sha256 $source) -ne $record.sha256) {
            throw "Manifest entry exists but source differs; refusing to replace: $source"
        }
        Remove-Item -LiteralPath $source -Force
        Write-Output "[$index/$($candidates.Count)] Removed already verified original $relative."
        continue
    }
    if (Get-ChildItem -LiteralPath $file.DirectoryName -File -Filter ($file.Name + '.part*') -ErrorAction SilentlyContinue) {
        throw "Split parts already exist beside source; refusing to overwrite: $source"
    }
    $sourceLength = [long]$file.Length
    $sourceStream = [System.IO.File]::Open($source, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::Read)
    $sourceHash = [System.Security.Cryptography.IncrementalHash]::CreateHash([System.Security.Cryptography.HashAlgorithmName]::SHA256)
    $parts = [System.Collections.Generic.List[object]]::new()
    $tempPaths = [System.Collections.Generic.List[string]]::new()
    $start = [long]0
    try {
        while ($start -lt $sourceLength) {
            $target = [Math]::Min($sourceLength, $start + $PartLimit - 5000000)
            if ($target -eq $sourceLength) { $end = $sourceLength }
            else {
                $next = Find-NextLineBoundary $sourceStream $target
                if (($next - $start) -le $PartLimit) { $end = $next }
                else {
                    $previous = Find-PreviousLineBoundary $sourceStream $target $start
                    if ($previous -le $start) { throw "One JSONL record exceeds the safe chunk size: $source" }
                    $end = $previous
                }
            }
            if ($end -le $start -or ($end - $start) -ge 100000000) { throw "Invalid or oversized split boundary in: $source" }
            $partNumber = $parts.Count + 1
            $suffix = '.part' + $partNumber.ToString('D3')
            $tempPath = $source + '.split-tmp' + $suffix
            $finalPath = $source + $suffix
            if ((Test-Path -LiteralPath $tempPath) -or (Test-Path -LiteralPath $finalPath)) { throw "Part path already exists: $finalPath" }
            $tempPaths.Add($tempPath)
            $partHash = [System.Security.Cryptography.IncrementalHash]::CreateHash([System.Security.Cryptography.HashAlgorithmName]::SHA256)
            $partStream = [System.IO.File]::Open($tempPath, [System.IO.FileMode]::CreateNew, [System.IO.FileAccess]::Write, [System.IO.FileShare]::None)
            try {
                $sourceStream.Position = $start
                $remaining = $end - $start
                $buffer = [byte[]]::new($BufferSize)
                while ($remaining -gt 0) {
                    $read = $sourceStream.Read($buffer, 0, [int][Math]::Min($buffer.Length, $remaining))
                    if ($read -le 0) { throw "Unexpected end of source file: $source" }
                    $partStream.Write($buffer, 0, $read)
                    $partHash.AppendData($buffer, 0, $read)
                    $sourceHash.AppendData($buffer, 0, $read)
                    $remaining -= $read
                }
            } finally { $partStream.Dispose() }
            $parts.Add([ordered]@{ path = [System.IO.Path]::GetRelativePath($RepositoryRoot, $finalPath).Replace('\', '/'); bytes = ($end - $start); sha256 = [Convert]::ToHexString($partHash.GetHashAndReset()).ToLowerInvariant() })
            $partHash.Dispose()
            $start = $end
        }
    } catch {
        $sourceStream.Dispose(); $sourceHash.Dispose()
        foreach ($tempPath in $tempPaths) { if (Test-Path -LiteralPath $tempPath) { Remove-Item -LiteralPath $tempPath -Force } }
        throw
    }
    $sourceStream.Dispose()
    $sourceDigest = [Convert]::ToHexString($sourceHash.GetHashAndReset()).ToLowerInvariant()
    $sourceHash.Dispose()

    $combinedHash = [System.Security.Cryptography.IncrementalHash]::CreateHash([System.Security.Cryptography.HashAlgorithmName]::SHA256)
    try {
        for ($partIndex = 0; $partIndex -lt $parts.Count; $partIndex++) {
            $part = $parts[$partIndex]
            $tempPath = $tempPaths[$partIndex]
            $partCheck = [System.Security.Cryptography.IncrementalHash]::CreateHash([System.Security.Cryptography.HashAlgorithmName]::SHA256)
            $partSize = [long]0
            $input = [System.IO.File]::OpenRead($tempPath)
            try {
                $buffer = [byte[]]::new($BufferSize)
                while (($read = $input.Read($buffer, 0, $buffer.Length)) -gt 0) {
                    $partCheck.AppendData($buffer, 0, $read)
                    $combinedHash.AppendData($buffer, 0, $read)
                    $partSize += $read
                }
            } finally { $input.Dispose() }
            $partDigest = [Convert]::ToHexString($partCheck.GetHashAndReset()).ToLowerInvariant()
            $partCheck.Dispose()
            if ($partSize -ne [long]$part.bytes -or $partDigest -ne $part.sha256) { throw "Part verification failed; original retained: $source" }
        }
        $reassembledDigest = [Convert]::ToHexString($combinedHash.GetHashAndReset()).ToLowerInvariant()
        if ($reassembledDigest -ne $sourceDigest) { throw "Combined parts do not reconstruct the source; original retained: $source" }
    } catch {
        foreach ($tempPath in $tempPaths) { if (Test-Path -LiteralPath $tempPath) { Remove-Item -LiteralPath $tempPath -Force } }
        throw
    } finally { $combinedHash.Dispose() }

    for ($partIndex = 0; $partIndex -lt $parts.Count; $partIndex++) {
        Move-Item -LiteralPath $tempPaths[$partIndex] -Destination (Join-Path $RepositoryRoot ($parts[$partIndex].path -replace '/', '\'))
    }
    $manifest.files += [ordered]@{
        source = $relative
        bytes = $sourceLength
        sha256 = $sourceDigest
        parts = @($parts.ToArray() | ForEach-Object { [pscustomobject]$_ })
    }
    Write-Manifest $manifest
    Remove-Item -LiteralPath $source -Force
    Write-Output "[$index/$($candidates.Count)] Split $relative into $($parts.Count) verified parts."
}

Write-Output "Completed: $($manifest.files.Count) logs split; manifest: $ManifestPath"

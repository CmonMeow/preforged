param([string]$PackageDirectory,[string]$ArtifactDirectory,[string]$ArchivePath)
$ErrorActionPreference='Stop'
$taskRoot=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$taskPackage=if($PackageDirectory){[System.IO.Path]::GetFullPath($PackageDirectory)}else{Join-Path $taskRoot 'bin/GuildWarsLogger'}
$taskArchive=if($ArchivePath){[System.IO.Path]::GetFullPath($ArchivePath)}elseif($PackageDirectory){$taskPackage+'.zip'}else{Join-Path $taskRoot 'bin/GuildWarsLogger.zip'}
$taskArtifacts=if($ArtifactDirectory){[System.IO.Path]::GetFullPath($ArtifactDirectory)}else{
    $taskCache=Join-Path $taskRoot 'build/capture/CMakeCache.txt'
    $taskOutput=@(Get-Content -LiteralPath $taskCache | Where-Object {$_ -cmatch '^CAPTURE_OUTPUT_DIRECTORY:PATH='})
    if($taskOutput.Count -ne 1){throw 'Configured capture output unavailable; build the collector or supply ArtifactDirectory'}
    [System.IO.Path]::GetFullPath($taskOutput[0].Substring('CAPTURE_OUTPUT_DIRECTORY:PATH='.Length))
}
# Controller and DLL must come from the configured build together. A separate
# staged controller silently attaching an old adjacent DLL is not a live test.
foreach($taskBinary in @('capture_control.exe','world_capture.dll','gwca.dll')){
    if(-not (Test-Path -LiteralPath (Join-Path $taskArtifacts $taskBinary) -PathType Leaf)){throw "Missing configured capture artifact: $taskBinary"}
}
$taskProvenance=Join-Path $taskRoot 'third_party/gwca-current/gwca-provenance.json'
$taskProvenanceData=Get-Content -LiteralPath $taskProvenance -Raw | ConvertFrom-Json
$taskDllRecords=@($taskProvenanceData.files | Where-Object {$_.path -eq 'bin/gwca.dll'})
if($taskDllRecords.Count -ne 1){throw 'Current GWCA provenance must identify exactly one runtime DLL'}
$taskGwcaSource=Join-Path $taskRoot 'third_party/gwca-current/bin/gwca.dll'
foreach($taskGwcaFile in @($taskGwcaSource,(Join-Path $taskArtifacts 'gwca.dll'))){
    if((Get-FileHash -LiteralPath $taskGwcaFile -Algorithm SHA256).Hash -ine $taskDllRecords[0].sha256){throw "GWCA provenance mismatch: $taskGwcaFile"}
}
New-Item -ItemType Directory -Force $taskPackage | Out-Null
Copy-Item -LiteralPath (Join-Path $taskArtifacts 'capture_control.exe') -Destination $taskPackage
Copy-Item -LiteralPath (Join-Path $taskArtifacts 'world_capture.dll') -Destination $taskPackage
Copy-Item -LiteralPath (Join-Path $taskRoot 'third_party/gwca-current/bin/gwca.dll') -Destination $taskPackage
Copy-Item -LiteralPath $taskProvenance -Destination (Join-Path $taskPackage 'gwca-provenance.json')
$taskNames=@('README.txt','START LOGGER.cmd','STOP LOGGER.cmd','STATUS.cmd','FIND CLIENT.cmd','AUTO TP.cmd','auto_tp.py','GWToolboxpp-LICENSE.txt')
foreach($taskName in $taskNames){Copy-Item -LiteralPath (Join-Path $PSScriptRoot ('package/'+$taskName)) -Destination $taskPackage}
$taskNames+=@('capture_control.exe','world_capture.dll','gwca.dll','gwca-provenance.json')
$taskManifest=foreach($taskName in $taskNames){$taskHash=Get-FileHash -LiteralPath (Join-Path $taskPackage $taskName) -Algorithm SHA256; [pscustomobject]@{file=$taskName;sha256=$taskHash.Hash.ToLowerInvariant()}}
$taskManifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskPackage 'manifest.json')
$taskNames+='manifest.json'
# Explicit allowlist prevents accidentally distributing logs produced here.
$taskFiles=$taskNames | ForEach-Object {Join-Path $taskPackage $_}
Compress-Archive -LiteralPath $taskFiles -DestinationPath $taskArchive -Force
Write-Output $taskArchive

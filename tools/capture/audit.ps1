param(
    [Parameter(Mandatory=$true)][string]$Capture,
    [string]$Output = 'build/capture-audit.json'
)
$ErrorActionPreference='Stop'
$source=(Resolve-Path -LiteralPath $Capture).Path
$destination=[IO.Path]::GetFullPath($Output)
if($source -eq $destination){throw 'Audit output must not overwrite the capture'}
$maps=@{}
$lineNumber=0
$invalid=0
$events=@{}
foreach($line in [IO.File]::ReadLines($source)){
    ++$lineNumber
    try {$row=$line | ConvertFrom-Json -ErrorAction Stop} catch {++$invalid;continue}
    $event=[string]$row.event
    if(!$events.ContainsKey($event)){$events[$event]=0}
    ++$events[$event]
    if($event -notin @('npc_observations','npc_definition_observed')){continue}
    $map=[string]$row.map
    if(!$maps.ContainsKey($map)){
        $maps[$map]=@{observations=0;encoded=0;decoded=0;models=@{};definitions=@{}}
    }
    $state=$maps[$map]
    # Table IDs are session-local: only match definitions in the same epoch.
    if($event -eq 'npc_definition_observed'){
        $key="$($row.instance_epoch):$($row.npc_table_id)"
        $state.definitions[$key]=$true
        continue
    }
    foreach($npc in $row.npcs){
        ++$state.observations
        if($npc.name_encoded_utf16){++$state.encoded}
        if($npc.name_status -eq 'decoded' -and $npc.name){++$state.decoded}
        $state.models["$($row.instance_epoch):$($npc.model)"]=$true
    }
}
$reportMaps=@(foreach($map in ($maps.Keys|Sort-Object {[int]$_})){
    $state=$maps[$map]
    $missing=@($state.models.Keys|Where-Object {!$state.definitions.ContainsKey($_)})
    [ordered]@{map=[int]$map;observation_count=$state.observations;
        observations_with_encoded_name=$state.encoded;observations_with_decoded_name=$state.decoded;
        epoch_model_count=$state.models.Count;epoch_models_missing_definitions=$missing.Count;
        missing_epoch_models=@($missing|Sort-Object)}
})
$report=[ordered]@{schema=1;source=$source;sha256=(Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash;
    lines=$lineNumber;invalid_json_lines=$invalid;events=$events;maps=$reportMaps;
    limitations='Counts are observations, not NPC populations. Definition presence does not validate fields, 2022 assets, permanence, or spawn rules. No game or capture files are modified.'}
[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination))|Out-Null
$report|ConvertTo-Json -Depth 8|Set-Content -LiteralPath $destination -Encoding utf8
Write-Output "Audited $lineNumber lines, $invalid invalid JSON lines, $($maps.Count) maps. Report: $destination"

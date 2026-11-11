param([Parameter(Mandatory=$true)][string[]]$Path)
$ErrorActionPreference='Stop'
# Directory metadata can stay at zero while an injected logger holds its file.
# Query a shared read handle; never stop recording merely to refresh metadata.
foreach($capturePath in $Path){
    $resolved=(Resolve-Path -LiteralPath $capturePath).Path
    $stream=$null
    try{
        $stream=[IO.FileStream]::new($resolved,[IO.FileMode]::Open,[IO.FileAccess]::Read,
            [IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete)
        $length=$stream.Length
        $sample=[byte[]]::new(512)
        $read=$stream.Read($sample,0,$sample.Length)
        [pscustomobject]@{Path=$resolved;HandleBytes=$length;ReadableSampleBytes=$read;
            HasData=($length -gt 0 -and $read -gt 0);Error=$null}
    }catch{
        [pscustomobject]@{Path=$resolved;HandleBytes=$null;ReadableSampleBytes=0;
            HasData=$false;Error=$_.Exception.Message}
    }finally{if($null -ne $stream){$stream.Dispose()}}
}

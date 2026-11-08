$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '../tools/observer/host_aliases.ps1')
function Check($condition,$message){if(!$condition){throw $message}}
function Recording($opponent,$tags){$guilds=@{};$i=0;foreach($tag in $tags){$guilds[[string]++$i]=@{tag=$tag}};return [pscustomobject]@{folder="2026-01-01_Map_01.00_[123456]vs[$opponent]";guilds=[pscustomobject]$guilds}}
$tag=[string][char]0x6226
$records=@((Recording 'A' @($tag,'A')),(Recording 'B' @($tag,'B')),(Recording 'C' @($tag,'C')))
$found=Resolve-ObserverHostAliases $records
Check ($found['123456'].Tag -ceq $tag) 'unique cross-record alias lost'
Check ($found['123456'].Provenance -eq 'cache-tag-intersection:records=3:opponents=3') 'alias provenance lost'
Check ((Resolve-ObserverHostAliases @($records[0],$records[0],$records[0])).Count -eq 0) 'duplicate recording counted as independent evidence'
Check ((Resolve-ObserverHostAliases $records[0..1]).Count -eq 0) 'insufficient evidence accepted'
$ambiguous=@();foreach($opponent in @('A','B','C')){$ambiguous+=Recording $opponent @($tag,([string][char]0x6230),$opponent)}
Check ((Resolve-ObserverHostAliases $ambiguous).Count -eq 0) 'ambiguous alias guessed'
$conflicting=$records+@(Recording 'D' @('D',([string][char]0x6230)))
Check ((Resolve-ObserverHostAliases $conflicting).Count -eq 0) 'conflicting evidence ignored'
'PASS observer host alias independence, ambiguity and conflict checks'

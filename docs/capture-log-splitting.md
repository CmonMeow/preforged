# Capture log splitting

GitHub rejects individual files at 100 MB. To make archived JSONL captures
uploadable, run `tools/capture/split_jsonl_logs.ps1` from PowerShell 7. It scans
`data` and `bin`, splits JSONL files at or above 100,000,000 bytes into
95,000,000-byte maximum parts, and keeps every JSON record intact at an LF
boundary. The bytes are preserved exactly.

Each source remains until the parts pass per-part SHA-256 checks and their
combined SHA-256 matches the source. The verified source is then removed, and
`data/captures/split-log-manifest.json` records the original relative path,
size, hash, and ordered part hashes. The script can resume from the manifest.

Restore original logs with:

```powershell
pwsh -NoProfile -File tools/capture/split_jsonl_logs.ps1 -Join
```

Reconstruction verifies all part hashes and the original file hash. It leaves
the split parts in place.

# Contributing

Thanks for helping with Preforged. It is an unfinished private-server research and development project. Contributions should make a specific, testable improvement and describe its limits clearly.

## Development

- Runtime code belongs in `src/`, public headers in `include/`, tests in `tests/`, and developer tools in `tools/`.
- Use native C++ for runtime behavior. Python and PowerShell are suitable for development utilities and validation scripts.
- Build with the commands in [Sharing and local setup](docs/sharing-and-setup.md).
- Keep generated files, binaries, databases, client assets, and captures out of commits.
- Do not add secrets, account credentials, personal account exports, or raw private capture logs.
- Preserve capture provenance and distinguish packet evidence, wiki references, simulation tests, and rendered client verification.
- Include a focused regression test for a mechanics change where practical. State what remains untested.

## Pull requests

Explain the observed problem, the change, and the validation performed. Keep unrelated formatting or generated artifacts out of the diff. Do not claim full client compatibility from unit tests alone.

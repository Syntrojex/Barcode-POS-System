# Follow-up review results - 19 November 2026

Initial baseline: branch `ali`, commit `1b16a70`; final source recheck at
`883425c`. This is a follow-up to the
[original verification](../verification.md), with an additional source review
of backend, GUI, scanner, active targets, and legacy paths.

## Observed checks

| Check | Result |
| --- | --- |
| `git fetch origin` | Completed successfully. |
| `git pull --ff-only origin ali` | Already up to date. |
| Branch compared with fetched `origin/main` | 33 commits ahead, 0 behind at review baseline. |
| Open pull requests through public GitHub API | Empty list returned; no open PR to inspect or take. |
| Backend Debug incremental build | Passed; Ninja reported no work required. |
| Backend CTest | All five suites passed, 0 failures; total 2.20 seconds. |
| Initial Qt GUI rebuild, camera disabled, at `1b16a70` | Failed again at missing `RecordOrderPage` members. |
| Final Qt GUI rebuild, camera disabled, at `883425c` | Passed; `BarcodeHub.exe` linked after the separately committed header repair. |

The backend rerun reused the configured build directory from the earlier fresh
build. It is not a second clean-build claim. No camera runtime, GUI checkout,
portable package, or physical receipt printer was tested.

## Reproduction commands

Run from the repository root using the matching toolchain. See the
[build guide](../build.md) for configure commands if these directories do not exist.

```powershell
cmake --build build/increment2-audit-backend --parallel 4
ctest --test-dir build/increment2-audit-backend --output-on-failure
$env:PATH = 'C:/Qt/Tools/mingw1310_64/bin;C:/Qt/6.11.0/mingw_64/bin;' + $env:PATH
cmake --build build/increment2-audit-gui --parallel 4
```

The initial GUI compiler report at
[RecordOrderPage.cpp:61](../../../Frontend/src/ui/RecordOrderPage.cpp#L61):

```text
error: 'm_feed' was not declared in this scope
```

Further diagnostics included `m_customer`, `m_contact`, `m_manual`, `m_sessionId`,
`m_checkout`, and total labels. Compare the
[header](../../../Frontend/include/ui/RecordOrderPage.h) with its implementation.
During this review another process committed `46f1a4b` (a project text dump),
then `883425c` (the header repair). Rebuilding after those commits completed
successfully, including linking `BarcodeHub.exe`. This documentation work did
not author either commit or change application source. Camera-enabled compilation
and interactive behavior remain unverified.

## Additional source findings

- History loading clears cached history before reading all three record files;
  a malformed file can make history appear empty. See [diagnostics](history-diagnostics.md).
- Saved bill lines are paired with order details by position during history
  reconstruction. Preserve item order across the files.
- External file edits have no automatic refresh UI. See [data lifecycle](data-lifecycle.md).
- Quantity serialization has three decimal places, while backend weight
  validation accepts smaller positives. Billing checks finite inputs without a
  final finite-result check. See [precision boundaries](identifiers-precision.md).

These findings are source-inspected boundaries, not newly executed failure tests.
The [screen](screen-rehearsal.md), [payment](payment-cases.md), and
[scanner](scanner-rehearsal.md) cases are an explicit plan for further checking.

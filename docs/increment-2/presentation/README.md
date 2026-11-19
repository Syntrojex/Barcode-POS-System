# Increment 2 presentation preparation

This supplement turns the [implementation inspection](../README.md) into
requirements evidence, viva answers, and manual rehearsal cases. Reviewed on
19 November 2026 from `ali` at `1b16a70` for the 20 November presentation.

**Current readiness:** backend tests pass (5/5). The current GUI build fails in
`RecordOrderPage.cpp` because its header does not declare members used by the
implementation. Screen and camera cases below are rehearsal instructions, not
claims of a successful runtime test. See [review results](review-results.md).

## Suggested reading order

1. Read the [requirements matrix](requirements-matrix.md) to explain what each
   Increment 2 module delivers and which claims need qualification.
2. Rehearse the [backend viva](viva-backend.md) and [GUI viva](viva-gui.md).
3. Explain the [EAN-8 worked example](ean8-worked-example.md) and the
   [identifier/precision examples](identifiers-precision.md).
4. Use the [screen cases](screen-rehearsal.md), [shortcut cases](shortcut-cases.md),
   and [camera cases](scanner-rehearsal.md) after the GUI build blocker is resolved.
5. Keep [history diagnostics](history-diagnostics.md) and
   [persistence failure cases](persistence-failure-cases.md) available for questions.

## Topic map

| Question | Evidence or rehearsal aid |
| --- | --- |
| Which requirements are implemented? | [Requirement evidence](requirements-matrix.md) |
| What actually passed on this machine? | [Review results](review-results.md) |
| How do the backend modules cooperate? | [Backend viva](viva-backend.md) |
| What do the GUI buttons really do? | [GUI viva](viva-gui.md) |
| What inputs should be accepted or rejected? | [Validation cases](validation-cases.md) |
| What counts as a payment? | [Payment cases](payment-cases.md) |
| What happens when saving fails? | [Persistence failures](persistence-failure-cases.md) |
| How do IDs and decimal precision work? | [Identifiers and precision](identifiers-precision.md) |
| How should each page be rehearsed? | [Screen cases](screen-rehearsal.md) |
| Why is history empty or different after restart? | [History diagnostics](history-diagnostics.md) |
| What survives restart? | [Data lifecycle](data-lifecycle.md) |
| Which shortcut depends on the current page? | [Shortcut cases](shortcut-cases.md) |
| How does the sample barcode checksum work? | [EAN-8 example](ean8-worked-example.md) |
| What can be tested with a camera? | [Scanner rehearsal](scanner-rehearsal.md) |
| Which source files and targets are active? | [Build targets](build-targets.md) |
| What belongs in a portable demo folder? | [Deployment rehearsal](deployment-rehearsal.md) |

The original [worked cart example](../demo.md) gives the PKR 938 milk-and-bread
transaction. These supplements add questions and acceptance cases around that
walkthrough. Application code, tests, configuration, and business data are not
modified by this documentation work.

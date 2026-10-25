---
hide:
  - toc
---

# A map of the game, from intent to evidence

SDL-Stars reconstructs Stars! as a portable desktop game and an independent
command-line host. This handbook connects what a player or host wants to do
with the workflows, rules, file contracts, and evidence behind it.

**Start with a journey. Follow a module. Check its evidence.**

| Your starting point | Read next |
| --- | --- |
| Understand how the game fits together | [Game lifecycle and implementation layers](overview.md) |
| Follow a module's decisions, state, failure and cancellation paths | [Module diagram atlas](diagrams.md) |
| Create a race or start a game | [Before entering a game](../spec/README.md#before-entering-a-game) |
| Play and exchange a turn | [Inside a player turn](../spec/README.md#inside-a-player-turn) |
| Run a host or understand simulation | [Hosting and simulation](../spec/README.md#hosting-and-simulation) |
| Implement persistence or platform services | [Shared support](../spec/README.md#shared-support) |
| Find a particular story or acceptance criterion | [Requirement index](requirements.md) |

## Read with the right expectations

The collection has detailed entries for all first-level modules. Logical module
ownership remains proposed; formal rule coverage and implementation qualification
remain incomplete. A checked Dafny property establishes a property of that model,
within its stated assumptions. It does not establish that the C implementation
refines the model.

Read [status and evidence](reading.md) before interpreting a requirement as a
delivered capability. The [coverage guide](evidence.md) leads to the current
completion backlog and dated verification records.

## One collection, several ways in

The sidebar follows the module map: setup, player activity, hosting, and shared
contracts. Search accepts module IDs, requirement IDs, function names, and terms
from the [glossary](../GLOSSARY.md).

The module pages are built from the repository's existing Markdown. Formal
reference pages are built from its Dafny files. [Maintaining the handbook](maintaining.md)
explains how changes reach the site.

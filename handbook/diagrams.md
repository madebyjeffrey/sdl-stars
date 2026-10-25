# Module workflow and state diagrams

Start with a module's workflow to follow its main decisions, failure paths and
caller handoffs. Its state diagram shows the illustrated draft, live, pending,
published or recovery milestones. Both diagrams live in the owning specification
and link back to its existing prose and canonical contracts.

**Cancel has an entry-specific meaning.** Some dialogs discard copied work;
others commit on close or retain earlier writes. Host generation and synchronous
platform operations have no generic mid-operation Cancel. A diagram marks an
undefined or uncovered boundary without inventing a native error return.

These states are reading projections, not new persisted enums or evidence of
complete C/model refinement. Follow each diagram's scope note, inspected
revision, and coverage inventory. The table below is regenerated from the module
map and actual Mermaid blocks whenever the handbook builds.

Use **Fit to width** for an overview and **Actual size** to read labels. At
actual size, scroll horizontally when a diagram is wider than the page. The
diagram region accepts keyboard focus for scrolling as well as touch gestures.

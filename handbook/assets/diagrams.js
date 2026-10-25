// Mermaid renders inside closed shadow roots. Size its host rather than touching
// renderer internals, so the same source can offer an overview and readable text.
(() => {
  function enhance() {
    for (const diagram of document.querySelectorAll("div.mermaid:not([data-diagram-view])")) {
      diagram.dataset.diagramView = "";
      const frame = document.createElement("section");
      frame.className = "handbook-diagram";
      const toolbar = document.createElement("div");
      toolbar.className = "handbook-diagram-controls";
      toolbar.setAttribute("role", "group");
      toolbar.setAttribute("aria-label", "Diagram display");
      const fit = document.createElement("button");
      const actual = document.createElement("button");
      const hint = document.createElement("span");
      fit.type = actual.type = "button";
      fit.textContent = "Fit to width";
      actual.textContent = "Actual size";
      toolbar.append(fit, actual, hint);
      diagram.before(frame);
      frame.append(toolbar, diagram);
      diagram.tabIndex = 0;
      diagram.setAttribute("role", "region");
      diagram.setAttribute("aria-label", "Diagram; scroll horizontally at actual size");
      diagram.style.width = "1px";
      const intrinsicWidth = diagram.scrollWidth;
      diagram.style.width = "";
      const naturalWidth = intrinsicWidth > 1 ? intrinsicWidth : Math.max(1, diagram.scrollWidth);
      let fitting = true;
      function display(shouldFit) {
        fitting = shouldFit;
        fit.setAttribute("aria-pressed", String(fitting));
        actual.setAttribute("aria-pressed", String(!fitting));
        diagram.style.width = fitting ? `${naturalWidth}px` : "";
        diagram.style.zoom = fitting ? Math.min(1, frame.clientWidth / naturalWidth) : 1;
        diagram.style.overflowX = fitting ? "hidden" : "auto";
        hint.textContent = fitting ? "Choose Actual size to read labels" : "Scroll sideways for details";
        if (!fitting) {
          diagram.focus({ preventScroll: true });
          diagram.scrollLeft = (diagram.scrollWidth - diagram.clientWidth) / 2;
        }
      }
      fit.addEventListener("click", () => display(true));
      actual.addEventListener("click", () => display(false));
      new ResizeObserver(() => { if (fitting) display(true); }).observe(frame);
      display(true);
    }
  }
  new MutationObserver(enhance).observe(document.body, { childList: true, subtree: true });
  enhance();
})();

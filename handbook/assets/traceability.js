// Progressive filtering of static, source-linked trace views. No evidence is
// fetched or classified here; the unfiltered content is rendered at build time.
(() => {
  function enhance() {
    for (const view of document.querySelectorAll("[data-trace-view]:not([data-trace-ready])")) {
      view.dataset.traceReady = "";
      const controls = view.parentElement.querySelector("[data-trace-controls]");
      if (!controls) continue;
      const story = controls.querySelector("[data-trace-story]");
      const query = controls.querySelector("[data-trace-query]");
      const count = controls.querySelector("[data-trace-count]");
      const cards = [...view.querySelectorAll(".trace-criterion")];
      const stories = [...view.querySelectorAll(".trace-story")];
      const storyText = new Map(stories.map(section => [section.dataset.story, section.textContent]));
      const params = new URL(location.href).searchParams;
      if ([...story.options].some(option => option.value === params.get("story"))) story.value = params.get("story");
      query.value = params.get("q") || "";
      function apply(updateUrl = true) {
        const needle = query.value.trim().toLocaleLowerCase();
        let visible = 0;
        const visibleStories = new Set();
        for (const card of cards) {
          const parents = card.dataset.stories.split(" ");
          const text = card.textContent + parents.map(parent => storyText.get(parent) || "").join(" ");
          card.hidden = Boolean((story.value && !parents.includes(story.value)) ||
            (needle && !text.toLocaleLowerCase().includes(needle)));
          if (!card.hidden) {
            visible++;
            parents.forEach(parent => visibleStories.add(parent));
          }
        }
        for (const section of stories) {
          section.hidden = Boolean((story.value && section.dataset.story !== story.value) ||
            (needle && !visibleStories.has(section.dataset.story)));
        }
        count.textContent = `Showing ${visible} of ${cards.length} acceptance criteria.`;
        if (updateUrl) {
          const url = new URL(location.href);
          for (const [key, value] of [["story", story.value], ["q", query.value]]) {
            if (value) url.searchParams.set(key, value);
            else url.searchParams.delete(key);
          }
          history.replaceState(null, "", url);
        }
      }
      view.revealTraceAnchor = () => {
        const target = document.getElementById(location.hash.slice(1));
        if (target && view.contains(target) && target.hidden) {
          story.value = query.value = "";
          apply();
          target.scrollIntoView();
        }
      };
      story.addEventListener("change", () => apply());
      query.addEventListener("input", () => apply());
      controls.querySelector("[data-trace-reset]").addEventListener("click", () => {
        story.value = query.value = "";
        apply();
      });
      controls.hidden = false;
      apply(false);
      view.revealTraceAnchor();
    }
  }
  window.addEventListener("hashchange", () => {
    document.querySelector("[data-trace-view]")?.revealTraceAnchor?.();
  });
  new MutationObserver(enhance).observe(document.body, { childList: true, subtree: true });
  enhance();
})();

'use strict';
const guide = JSON.parse(document.getElementById('guide-data').textContent);
document.getElementById('guide-style').textContent = guide.css;
const popup = document.getElementById('popup');
const frame = popup.querySelector('iframe');
function layoutTabs(doc) {
    for (const paragraph of doc.querySelectorAll('.help-paragraph')) {
        const stops = paragraph.dataset.tabs.split(',').filter(Boolean).map(value => Number(value) * 96 / 72);
        const origin = paragraph.getBoundingClientRect().left - parseFloat(doc.defaultView.getComputedStyle(paragraph).marginLeft);
        for (const tab of paragraph.querySelectorAll('.tab')) {
            tab.style.width = '0px';
            const position = tab.getBoundingClientRect().left - origin;
            let next = stops.find(value => value > position + 0.5);
            if (next === undefined) {
                const last = stops.length ? stops[stops.length - 1] : 0;
                next = last + Math.max(1, Math.floor((position - last) / 48) + 1) * 48;
            }
            tab.style.width = Math.max(0, next - position) + 'px';
        }
    }
}
function bindLinks(doc) {
    doc.addEventListener('click', event => {
        const link = event.target.closest('a[href]');
        if (!link) return;
        const key = link.getAttribute('href').slice(1);
        if (!guide.pages[key]) return;
        event.preventDefault();
        if (link.hasAttribute('data-popup')) {
            frame.title = guide.pages[key].title;
            frame.onload = () => {
                bindLinks(frame.contentDocument);
                frame.contentDocument.fonts.ready.then(() => layoutTabs(frame.contentDocument));
            };
            frame.srcdoc = '<!doctype html><html lang="en"><head><meta charset="utf-8"><style>' +
                guide.css + '</style></head><body class="popup"><main>' + guide.pages[key].body + '</main></body></html>';
            if (!popup.open) popup.showModal();
        } else {
            if (popup.open) popup.close();
            if (location.hash === '#' + key) render(); else location.hash = key;
        }
    });
}
function render() {
    let key = location.hash.slice(1) || document.querySelector('meta[name="stars-help-start"]').content;
    if (!guide.pages[key]) key = guide.contents;
    const page = guide.pages[key];
    document.title = page.title;
    const main = document.querySelector('main');
    main.innerHTML = page.body;
    main.dataset.page = key;
    main.setAttribute('aria-label', page.title);
    const filter = document.getElementById('topic-filter');
    if (filter) filter.addEventListener('input', () => {
        const query = filter.value.toLocaleLowerCase();
        for (const item of document.querySelectorAll('#topics li')) item.hidden = !item.textContent.toLocaleLowerCase().includes(query);
    });
    document.fonts.ready.then(() => layoutTabs(document));
    window.scrollTo(0, 0);
}
bindLinks(document);
document.getElementById('back').addEventListener('click', () => history.back());
document.getElementById('close-popup').addEventListener('click', () => popup.close());
popup.addEventListener('close', () => frame.removeAttribute('srcdoc'));
window.addEventListener('hashchange', render);
window.addEventListener('resize', () => layoutTabs(document));
render();

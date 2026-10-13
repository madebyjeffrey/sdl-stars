'use strict';
if (new URLSearchParams(location.search).get('popup') === '1') document.body.classList.add('popup');
document.getElementById('back').addEventListener('click', () => history.back());
const popup = document.getElementById('popup');
document.getElementById('close-popup').addEventListener('click', () => popup.close());
popup.addEventListener('close', () => popup.querySelector('iframe').removeAttribute('src'));
document.addEventListener('click', event => {
    const link = event.target.closest('a[data-popup]');
    if (!link) return;
    event.preventDefault();
    const url = new URL(link.href);
    url.searchParams.set('popup', '1');
    popup.querySelector('iframe').src = url.href;
    popup.querySelector('iframe').title = link.textContent || 'Help definition';
    popup.showModal();
});
const filter = document.getElementById('topic-filter');
if (filter) filter.addEventListener('input', () => {
    const query = filter.value.toLocaleLowerCase();
    for (const item of document.querySelectorAll('#topics li')) item.hidden = !item.textContent.toLocaleLowerCase().includes(query);
});
function layoutTabs() {
    for (const paragraph of document.querySelectorAll('.help-paragraph')) {
        const stops = paragraph.dataset.tabs.split(',').filter(Boolean).map(value => Number(value) * 96 / 72);
        const origin = paragraph.getBoundingClientRect().left - parseFloat(getComputedStyle(paragraph).marginLeft);
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
document.fonts.ready.then(layoutTabs);
window.addEventListener('resize', layoutTabs);

// Injected into ESPHome's web_server v3 entities dashboard at /.
// Prepends an iframe that loads the /preview SVG above the entity cards.
(function () {
    function inject() {
        if (document.getElementById('cal-preview-wrap')) return;
        const wrap = document.createElement('div');
        wrap.id = 'cal-preview-wrap';
        wrap.style.cssText =
            'display:flex;justify-content:center;padding:14px 0 4px;';
        const f = document.createElement('iframe');
        f.src = '/preview';
        f.title = 'Calendar preview';
        f.style.cssText =
            'width:832px;height:600px;border:0;background:#1a1a1a;' +
            'border-radius:8px;box-shadow:0 4px 20px rgba(0,0,0,.4);';
        wrap.appendChild(f);
        // Insert as the first child of <body> so it sits above the title.
        document.body.insertBefore(wrap, document.body.firstChild);
    }
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', inject);
    } else {
        inject();
    }
})();

(function () {
  'use strict';

  var statusEl = document.getElementById('status');
  var buttons = Array.prototype.slice.call(document.querySelectorAll('.btn'));
  var busy = false;

  function setStatus(text, isError) {
    statusEl.textContent = text;
    statusEl.className = isError ? 'error' : '';
  }

  function send(dir) {
    var path = dir === 'stop' ? '/api/motor/stop' : '/api/motor/move';
    var payload = dir === 'stop' ? {} : { direction: dir };

    return fetch(path, {
      method: 'POST',
      cache: 'no-store',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    }).then(function (res) {
      return res.text().then(function (text) {
        if (!res.ok) throw new Error('HTTP ' + res.status + ' ' + text);
        return text;
      });
    });
  }

  function markActive(dir) {
    buttons.forEach(function (b) {
      b.classList.toggle('active', b.getAttribute('data-dir') === dir);
    });
  }

  buttons.forEach(function (btn) {
    var dir = btn.getAttribute('data-dir');

    function trigger(event) {
      event.preventDefault();
      if (busy) return;
      busy = true;
      markActive(dir);
      setStatus('enviando ' + dir + '...');
      send(dir).then(function () {
        setStatus(dir === 'stop' ? 'parado' : 'movendo: ' + dir);
      }).catch(function (err) {
        markActive(null);
        setStatus('falha: ' + err.message, true);
      }).then(function () {
        busy = false;
      });
    }

    btn.addEventListener('click', trigger);
  });

  setStatus('toque em um bot\u00e3o');
})();

$(document).ready(function() {
  There.init({
    onReady: function() {
      There.guiCommand('settings');
      There.fsCommand('closeWindow');
    },
  });
});
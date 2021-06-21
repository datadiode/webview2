There.init({
  onReady: function() {
    There.fsCommand('setStageWidthHeight', {
      width: 200,
      height: 548,
    });

    There.fsCommand('setTextureBitDepth', {
      depth: 32,
    });

    new ResizeObserver(function(entries) {
      const rect = entries[0].contentRect;
      There.fsCommand('setWidthHeight', {
        width: rect.width,
        height: rect.height,
      });
    }).observe($('.changeme')[0]);
  },

  onVariable: function(name, value) {
    if (name == 'there_teleporting') {
      $('.changeme').attr(name.replace('there_', 'data-'), value);
    }
  },
});

$(document).ready(function() {
  $('.titlebar .buttons .button[data-id="bar"]').on('click', function() {
    $('.changeme').attr('data-state', 'bar');
  }).on('mousedown', function(event) {
    event.stopPropagation();
  });

  $('.titlebar .buttons .button[data-id="full"]').on('click', function() {
    $('.changeme').attr('data-state', 'full');
  }).on('mousedown', function(event) {
    event.stopPropagation();
  });

  $('.titlebar .buttons .button[data-id="help"]').on('click', function() {
  }).on('mousedown', function(event) {
    event.stopPropagation();
  });

  $('.titlebar .buttons .button[data-id="close"]').on('click', function() {
  }).on('mousedown', function(event) {
    event.stopPropagation();
  });

  $('.sections .section .tab').on('click', function() {
    const section = $(this).parent().data('section');
    $('.changeme').attr('data-section', section);
    if (section == 'wardrobe') {
      $('.areas .area[data-section="wardrobe"][data-area="hairstyles"]').trigger('click');
    }
    if (section == 'body') {
      $('.areas .area[data-section="body"][data-area="head"]').trigger('click');
    }
  });

  $('.areas .area').on('click', function() {
    const area = $(this).data('area');
    $('.changeme').attr('data-area', area);
    $('.sections .section .panel .title').text($(this).data('title'));
    if (area == 'tops') {
      $('.sections .section .panel .items').attr('data-count', '2');
    } else if (area == 'outfits' || area == 'looksets') {
      $('.sections .section .panel .items').attr('data-count', '1');
    } else {
      $('.sections .section .panel .items').attr('data-count', '0');
    }
    if (area == 'face') {
      $('.changeme').attr('data-subarea', 'eyes-ears');
    }
  });

  $('.subareas').on('mousedown', function(event) {
    $('.subareas').attr('data-active', '1');
    event.stopPropagation();
  });

  $('.subareas .subarea').on('click', function(event) {
    const subarea = $(this).data('subarea');
    $('.changeme').attr('data-subarea', subarea);
    $('.subareas').attr('data-active', '0');
    event.stopPropagation();
  });

  $('.changeme').on('mousedown', function() {
    $('.subareas').attr('data-active', '0');
  });

  $('.sections .section[data-section="body"] .panel .editor[data-area="skin"] .item').on('click', function() {
    $(this).parent().find('.item').attr('data-selected', '0');
    $(this).attr('data-selected', '1');
  });

  $('.footer .button[data-id="save"]').on('click', function() {
  });

  $('.footer .button[data-id="shop"]').on('click', function() {
  });

  $('.footer .button[data-id="organize"]').on('click', function() {
  });

  $('.titlebar').on('mousedown', function(event) {
    There.fsCommand('beginDragWindow');
    event.preventDefault();
    event.stopPropagation();
  });

  $('.sections .section[data-section="wardrobe"] .tab').trigger('click');
});

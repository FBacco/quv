$(document).ready(function () {
    $selector = $('#frequency-selector');

    $selector.find('a[data-frequency]').click(function (e) {
        $.post($selector.data('endpoint'), {'frequency': $(this).data('frequency')});

        e.preventDefault();
    });
});

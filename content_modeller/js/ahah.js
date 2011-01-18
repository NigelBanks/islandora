$(document).ready(function() {
    $("body").ajaxComplete(function(event, request, settings) {
        Drupal.attachBehaviors();
    });
});
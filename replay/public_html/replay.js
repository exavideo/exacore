function loadJson(aUrl, callback) {
    jQuery.ajax(aUrl, {
        success: function(data, jqXHR, textStatus) {
            callback(data);
        },
        dataType: "json"
    });
}

function loadHtml(aUrl, callback) {
    jQuery.ajax(aUrl, {
        success: function(data, jqXHR, textStatus) {
            callback(data);
        },
        dataType: "html"
    });
}

function postJson(data, aUrl, callback) {
    jQuery.ajax(aUrl, {
        type: 'POST',
        success: function(data, jqXHR, textStatus) {
            callback(data);
        },
        data: JSON.stringify(data),
        dataType: "json"
    });
}

function putJson(data, aUrl, callback) {
    jQuery.ajax(aUrl, {
        type: 'PUT',
        success: function(data, jqXHR, textStatus) {
            callback(data);
        },
        data: JSON.stringify(data),
        dataType: "json"
    });
}

function deleteJson(data, aUrl, callback) {
    jQuery.ajax(aUrl, {
        type: 'DELETE',
        success: function(data, jqXHR, textStatus) {
            callback(data);
        },
        data: JSON.stringify(data),
        dataType: "json"
    });
}

function previewShot(shot) {
    putJson(shot, '/preview_shot.json', function() {});
}

function rollShot(shot) {
    putJson(shot, '/roll_shot.json', function() {});
}

// Populate the shot list with an initial data set.
function populateShots(data) {
    $.each(data, function(i, shot) {
        var new_div = $("#shotPrototype").clone( );

        new_div.find('#shotPreview img').attr('src', '/shots/'+shot.id+'/preview.jpg');
        new_div.find('#preview').click(function() { previewShot(shot) });
        new_div.find('#roll').click(function() { rollShot(shot) });
        new_div.find('#data').html('<table><tr><td>Source</td><td>' + shot.source_name + '</td></tr><tr><td>Start Time</td><td>xx:xx:xx:xx</td></tr></table>');
        
        $('#savedShots').append(new_div);
        new_div.show();
    });
}

$(function() {
    loadJson('/shots.json', populateShots);
})

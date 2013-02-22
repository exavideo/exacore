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

function queueShot(shot) {
    var div = makeDivForShot(jQuery.extend(true, {}, shot));

    //div.find('#queue').text('Copy'); 
    div.find('#queue').hide();
    div.find('#copy').show();
    div.find('#delete').show();
    div.find('#delete').removeAttr('disabled');
    $('#queuedShots').append(div);

    div.show( );
    selectShot(div);
}

function rollQueue() {
    var shots = $('#queuedShots div').map(function(i, shotDiv) {
        return $(shotDiv).data('shot');
    }).get( );

    putJson(shots, '/roll_queue.json', function() {});
}

function emptyQueue() {
    $('#queuedShots div').remove( );
}

function twoDigits(value) { 
    value = Math.floor(value)
    if (value < 10) {
        return '0' + value;
    } else {
        return value;
    }
}

function formatTimecode(tc_value) {
    var frames = tc_value % 30; /* FIXME NTSC hardcoded frames per sec */
    var seconds = (tc_value / 30) % 60; 
    var minutes = (tc_value / (30 * 60)) % 60;
    var hours = (tc_value / (30 * 60 * 60));

    return twoDigits(hours) + ':' + twoDigits(minutes) + ':' + twoDigits(seconds) + ':' + twoDigits(frames);
}

function selectShot(div) {
    $('.shotViewSelected').removeClass('shotViewSelected').addClass('shotViewUnselected');
    div.addClass('shotViewSelected').removeClass('shotViewUnselected');
}

function makeDivForShot(shot) {
    var new_div = $("#shotPrototype").clone(true);
    new_div.attr('id','');

    new_div.data('shot', shot);
    new_div.data('offset', 0);

    //new_div.find('#shotPreview img').attr('src', '/shots/'+shot.id+'/preview.jpg');
    new_div.find('#shotPreview img').attr('src', '/sources/'+shot.source+'/'+shot.start+'/preview.jpg')
    new_div.find('#preview').click(function() { previewShot( $(this).parent().data('shot')); return false; });
    new_div.find('#roll').click(function() { rollShot( $(this).parent().data('shot')); return false; });
    new_div.find('#queue').click(function() { queueShot( $(this).parent().data('shot')); return false; });
    new_div.find('#copy').click(function() { queueShot( $(this).parent().data('shot')); return false; });
    new_div.find('#delete').click(function() { $(this).parent().remove(); return false; });

    new_div.find('#back').click(seekBackButton);
    new_div.find('#forward').click(seekForwardButton);
    new_div.find('#in').click(markIn);
    new_div.find('#out').click(markOut);
    new_div.find("#dlvideo").click(downloadVideo);
    new_div.find("#dlaudio").click(downloadAudio);

    new_div.find('#queue').show( );
    new_div.find('#copy').hide( );
    new_div.find('#delete').hide( );

    new_div.click(function() { selectShot($(this)) } );

    updateShotData(new_div);

    return new_div;
}

function updateShotData(shotDiv) {
    var shot = shotDiv.data('shot');
    var offset = shotDiv.data('offset');

    shotDiv.find('#data').html(
        '<table><tr><td>Source</td><td>' + shot.source_name 
        + '</td></tr><tr><td>Preview</td><td>' + formatTimecode(shot.start + offset)
        + '</td></tr><tr><td>Start Time</td><td>' + formatTimecode(shot.start) 
        + '</td></tr><tr><td>Duration</td><td>' + formatTimecode(shot.length) 
        + '</td></tr></table>'
    );
}

function markIn() {
    var shotDiv = $(this).parent();

    var shot = shotDiv.data('shot');
    var oldOut = shot.start + shot.length;
    
    shot.start = shot.start + shotDiv.data('offset');
    if (shot.start < 0) {
        shot.start = 0;
    }

    if (oldOut < shot.start) {
        shot.length = 0;
    } else {
        shot.length = oldOut - shot.start;
    }

    shotDiv.data('shot', shot);
    shotDiv.data('offset', 0);

    updateShotData(shotDiv);
}

function markOut() {
    var shotDiv = $(this).parent();

    // if we're before the "in" mark... mark in here as well.
    // That makes length always positive.
    if (shotDiv.data('offset') < 0) {
        markIn(shotDiv);
    }

    var shot = shotDiv.data('shot');
    shot.length = shotDiv.data('offset');
    shotDiv.data('shot', shot);

    updateShotData(shotDiv); 
}

function downloadVideo() {
    var shotDiv = $(this).parent();
    var shot = shotDiv.data('shot');

    url = '/sources/'+shot.source+'/'+shot.start+'/'+shot.length+'/video/shot.mjpg';
    window.open(url, 'Video Download');
}

function downloadAudio() {
    var shotDiv = $(this).parent();
    var shot = shotDiv.data('shot');

    url = '/sources/'+shot.source+'/'+shot.start+'/'+shot.length+'/audio/2ch_48khz/shot.raw';
    window.open(url, 'Audio Download');
}

function seekBackButton() {
    var shotDiv = $(this).parent();
    doSeekBack(shotDiv);
}

function doSeekBack(shotDiv) {
    var shot = shotDiv.data('shot');

    var new_offset = shotDiv.data('offset') - 15;
    var new_start = shot.start + new_offset;

    // clamp start of shot to zero
    if (new_start < 0) {
        new_offset = new_offset - new_start;
    }
    
    shotDiv.data('offset', new_offset);
    updateShotPreview(shotDiv);
    updateShotData(shotDiv);
}

function seekForwardButton() {
    var shotDiv = $(this).parent();
    doSeekForward(shotDiv);
}

function doSeekForward(shotDiv) {
    shotDiv.data('offset', shotDiv.data('offset') + 15);
    updateShotPreview(shotDiv);
    updateShotData(shotDiv);
}

function updateShotPreview(shotDiv) {
    var shot = shotDiv.data('shot');
    var offset = shotDiv.data('offset');

    shotDiv.find('#shotPreview img').attr('src', '/sources/'+shot.source+'/' 
        + (shot.start + offset) + '/preview.jpg');
}

// Populate the shot list with an initial data set.
function populateShots(data) {
    $.each(data, function(i, shot) {
        var div = makeDivForShot(shot);
        div.find('#copy').hide();
        div.find('#delete').hide();
        div.find('#delete').attr('disabled','disabled');
        $('#recordedShots').append(div);
        div.show();
    });
}

function doRefreshMaybe(data) {
    var currentCount = $('#recordedShots div.shotView').size();
    if (currentCount != data.length) {
        $("#recordedShots").empty();
        populateShots(data);
    }
}

function refreshMaybe() {
    loadJson('/shots.json', doRefreshMaybe);
}

function handleKeyboard(evt) {
    var selectedShot = $('.shotViewSelected');
    var shotData = selectedShot.data('shot');

    if (evt.which == 38) {    // 38 = up
        if (selectedShot.prev( ).size( ) > 0) {
            selectShot(selectedShot.prev( ));
            selectedShot.prev( ).scrollintoview( );
        }
    } else if (evt.which == 40) { // 40 = down
        if (selectedShot.next( ).size( ) > 0) {
            selectShot(selectedShot.next( )); 
            selectedShot.next( ).scrollintoview( );
        }
    } else if (evt.which == 37) { // 37 = left
        doSeekBack(selectedShot);
    } else if (evt.which == 39) { // 39 = right
        doSeekForward(selectedShot);
    } else if (evt.which == 73) {
        selectedShot.find("#in").click();
    } else if (evt.which == 79) {
        selectedShot.find("#out").click();
    } else if (evt.which == 81) { // 81 = q
        queueShot(shotData);
    } else if (evt.which == 80) { // 80 = p
        previewShot(shotData);
    } else if (evt.which == 82) { // 82 = r
        rollShot(shotData);
    }
}

function loadFiles( ) {
    loadJson('/files.json', function(files) {
        $.each(files, function(i, x) {
            $('#filepicker').append('<option value="'+x+'">'+x+'</option>');
        });
    });
}

function rolloutFile( ) {
    var file = $('#filepicker').val( );

    if (file.length > 0) {
        putJson({filename : file }, '/ffmpeg_rollout.json', function(){});
    }
}

function resumeEncode( ) {
    putJson({ }, '/resume_encode.json');
}

$(function() {
    //loadJson('/shots.json', populateShots);
    //refreshTimeoutLoop();
    setInterval(refreshMaybe, 500);
    $('#rollQueue').click(function() { rollQueue( ); });
    $('#emptyQueue').click(function() { emptyQueue( ); });
    $('#queuedShots').sortable();
    $('#helpLink').click(function() {
        $('#help').dialog('open');
    });

    $('#updateList').click(function() {
        $('#recordedShots').empty();
        loadJson('/shots.json', populateShots);
    });

    $('#clearList').click(function() {
        if (confirm("Really clear shot list?")) {
            putJson({ }, '/clear_events', function() { $('#recordedShots').empty(); }); 
        }
    });


    $('#help').dialog({ autoOpen: false });
    $(document).keydown(handleKeyboard);

    loadFiles( );
    $("#rollout_file").click(rolloutFile);
    $("#resume_encode").click(resumeEncode);
});

